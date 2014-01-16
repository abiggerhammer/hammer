require 'hammer/internal'

module Hammer
  class Parser

    @@saved_objects = Hammer::Internal::DynamicVariable.new nil, "Hammer parse-time pins"

    # Don't create new instances with Hammer::Parser.new,
    # use the constructor methods instead (i.e. Hammer::Parser.int64 etc.)
    #
    # name: Name of the parser. Should be a symbol.
    # h_parser: The pointer to the parser as returned by hammer.
    # dont_gc: Pass additional data that's used by the parser and needs to be saved from the garbage collector (at least as long this object lives).
    def initialize(name, h_parser, dont_gc=[])
      @name = name
      @h_parser = h_parser
      # Always store as array, so we can easily add stuff later on
      dont_gc = [dont_gc] unless dont_gc.is_a? Array
      @dont_gc = dont_gc.dup
    end

    attr_reader :name
    attr_reader :h_parser

    # Parse the given data. Returns the parse result if successful, nil otherwise.
    #
    # data: A string containing the data to parse.
    def parse(data)
      raise RuntimeError, '@h_parser is nil' if @h_parser.nil?
      raise ArgumentError, 'expecting a String' unless data.is_a? String # TODO: Not needed, FFI checks that.

      ibuf = FFI::MemoryPointer.from_string(data)
      @@saved_objects.with([]) do
          result = Hammer::Internal.h_parse(@h_parser, ibuf, data.bytesize) # Don't include the trailing null
          if result.null?
            return nil
          else
            # NOTE:
            # The parse result *must* hold a reference to the parser that created it!
            # Otherwise, the parser might get garbage-collected while the result is still valid.
            # Any pointers to token strings will then be invalid.
            result.instance_variable_set :@parser, self
            result.instance_variable_set :@pins, @@saved_objects.value
            return result
          end
        end
    end

    # Binds an indirect parser.
    def bind(other_parser)
      raise RuntimeError, 'can only bind indirect parsers' unless self.name == :indirect
      Hammer::Internal.h_bind_indirect(self.h_parser, other_parser.h_parser)
      @dont_gc << other_parser
    end

    # Can pass the action either as a Proc in second parameter, or as block.
    def self.action(parser, action=nil, &block)
      action = block if action.nil?
      raise ArgumentError, 'no action' if action.nil?

      real_action = Proc.new {|hpr|
                          ret = action.call(hpr.ast)
                          # Pin the result
                          @@saved_objects.value << ret
                          hpt = hpr.arena_alloc(Hammer::Internal::HParsedToken)
                          unless hpr.ast.nil?
                            hpt[:index] = hpr[:ast][:index]
                            hpt[:bit_offset] = hpr[:ast][:bit_offset]
                          end
                          hpt[:token_type] = :"com.upstandinghackers.hammer.ruby.object"
                          hpt[:data][:uint] = ret.object_id
                          hpt
                        }

      h_parser = Hammer::Internal.h_action(parser.h_parser, real_action)
      return Hammer::Parser.new(:action, h_parser, [parser, action, real_action])
    end

    # Can pass the predicate either as a Proc in second parameter, or as block.
    def self.attr_bool(parser, predicate=nil, &block)
      predicate = block if predicate.nil?
      raise ArgumentError, 'no predicate' if predicate.nil?

      real_pred = Proc.new {|hpr| predicate.call hpr.ast}

      h_parser = Hammer::Internal.h_attr_bool(parser.h_parser, real_pred)
      return Hammer::Parser.new(:attr_bool, h_parser, [parser, predicate, real_pred])
    end

    def self.token(string)
      # Need to copy string to a memory buffer (not just string.dup)
      # * Original string might be modified, this must not affect existing tokens
      # * We need a constant memory address (Ruby string might be moved around by the Ruby VM)
      buffer = FFI::MemoryPointer.from_string(string)
      h_parser = Hammer::Internal.h_token(buffer, buffer.size-1) # buffer.size includes the null byte at the end
      encoding = string.encoding

      wrapping_action = Proc.new {|hpr|
                              hstr = hpr.arena_alloc(Hammer::Internal::HString)
                              hstr[:content] = hpr[:ast][:data][:bytes]
                              hstr[:encoding] = encoding.object_id

                              hpt = hpr.arena_alloc(Hammer::Internal::HParsedToken)
                              hpt[:token_type] = :"com.upstandinghackers.hammer.ruby.encodedStr"
                              hpt[:data][:user] = hstr.to_ptr
                              hpt[:bit_offset] = hpr[:ast][:bit_offset]
                              hpt[:index] = hpr[:ast][:index]
                              hpt
                            }
      wrapped_parser = Hammer::Internal.h_action(h_parser, wrapping_action)
      return Hammer::Parser.new(:token, wrapped_parser, [buffer, string, encoding, wrapping_action, h_parser])
    end

    def self.marshal_ch_arg(num)
      if num.is_a?(String)
        raise ArgumentError, "Expecting either a fixnum in 0..255 or a single-byte String" unless num.bytesize == 1
        num = num.bytes.first
      end
      raise ArgumentError, 'Expecting a Fixnum in 0..255 or a single-byte String' unless num.is_a?(Fixnum) and num.between?(0, 255)
      return num
    end
    private_class_method :marshal_ch_arg

    def self.ch_parser_wrapper(parser)
      return Hammer::Parser.action(parser) {|x| x.data.chr}
    end

    def self.ch(ch)
      num = marshal_ch_arg(ch)
      h_parser = Hammer::Internal.h_ch(num)

      return ch_parser_wrapper(Hammer::Parser.new(:ch, h_parser, nil))
    end

    def self.ch_range(ch1, ch2)
      ch1 = marshal_ch_arg(ch1)
      ch2 = marshal_ch_arg(ch2)
      h_parser = Hammer::Internal.h_ch_range(ch1, ch2)
      return ch_parser_wrapper(Hammer::Parser.new(:ch_range, h_parser, nil))
    end

    def self.int_range(parser, i1, i2)
      h_parser = Hammer::Internal.h_int_range(parser.h_parser, i1, i2)
      return Hammer::Parser.new(:int_range, h_parser, [parser])
    end

    def self.in(charset)
      raise ArgumentError, "Expected a String" unless charset.is_a?(String)
      ibuf = FFI::MemoryPointer.from_string(charset)
      h_parser = Hammer::Internal.h_in(ibuf, charset.bytesize)
      return ch_parser_wrapper(Hammer::Parser.new(:in, h_parser, nil))
    end

    def self.repeat_n(parser, count)
      h_parser = Hammer::Internal.h_repeat_n(parser.h_parser, count)
      return Hammer::Parser.new(:repeat_n, h_parser, nil)
    end

    def self.not_in(charset)
      raise ArgumentError, "Expected a String" unless charset.is_a?(String)
      ibuf = FFI::MemoryPointer.from_string(charset)
      h_parser = Hammer::Internal.h_not_in(ibuf, charset.bytesize)
      return ch_parser_wrapper(Hammer::Parser.new(:not_in, h_parser, nil))
    end

    # Defines a parser constructor with the given name.
    # Options:
    #   hammer_function: name of the hammer function to call (default: 'h_'+name)
    #   varargs: Whether the function is taking a variable number of arguments (default: false)
    def self.define_parser(name, options = {})
      hammer_function = options[:hammer_function] || ('h_' + name.to_s).to_sym
      varargs = options[:varargs] || false

      # Define a new class method
      define_singleton_method name do |*parsers|
        if varargs
          args = parsers.flat_map { |p| [:pointer, p.h_parser] }
          args += [:pointer, nil]
        else
          args = parsers.map(&:h_parser)
        end
        h_parser = Hammer::Internal.send hammer_function, *args

        return Hammer::Parser.new(name, h_parser, parsers)
      end
    end
    private_class_method :define_parser

    define_parser :sequence, varargs: true
    define_parser :choice, varargs: true

    define_parser :int64
    define_parser :int32
    define_parser :int16
    define_parser :int8
    define_parser :uint64
    define_parser :uint32
    define_parser :uint16
    define_parser :uint8
    define_parser :whitespace
    define_parser :left
    define_parser :right
    define_parser :middle
    define_parser :end_p
    define_parser :nothing_p
    define_parser :butnot
    define_parser :difference
    define_parser :xor
    define_parser :many
    define_parser :many1
    define_parser :optional
    define_parser :ignore
    define_parser :sepBy
    define_parser :sepBy1
    define_parser :epsilon_p
    define_parser :length_value
    define_parser :and
    define_parser :not
    define_parser :indirect

  end
end
