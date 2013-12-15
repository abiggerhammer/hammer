module Hammer
  class Parser

    # Don't create new instances with Hammer::Parser.new,
    # use the constructor methods instead (i.e. Hammer::Parser.int64 etc.)
    #
    # name: Name of the parser. Should be a symbol.
    # h_parser: The pointer to the parser as returned by hammer.
    # dont_gc: Pass additional data that's used by the parser and needs to be saved from the garbage collector.
    def initialize(name, h_parser, dont_gc)
      @name = name
      @h_parser = h_parser
      @dont_gc = dont_gc
    end

    attr_reader :name
    attr_reader :h_parser

    # Parse the given data. Returns the parse result if successful, nil otherwise.
    #
    # data: A string containing the data to parse.
    def parse(data)
      raise RuntimeError, '@h_parser is nil' if @h_parser.nil?
      raise ArgumentError, 'expecting a String' unless data.is_a? String # TODO: Not needed, FFI checks that.

      result = Hammer::Internal.h_parse(@h_parser, data, data.length)
      return result unless result.null?
    end

    # Binds an indirect parser.
    def bind(other_parser)
      raise RuntimeError, 'can only bind indirect parsers' unless self.name == :indirect
      Hammer::Internal.h_bind_indirect(self.h_parser, other_parser.h_parser)
    end

    def self.token(string)
      # TODO:
      # This might fail in JRuby.
      # See "String Memory Allocation" at https://github.com/ffi/ffi/wiki/Core-Concepts
      h_string = string.dup
      h_parser = Hammer::Internal.h_token(h_string, h_string.length)

      return Hammer::Parser.new(:token, h_parser, h_string)
    end

    def self.ch(num)
      raise ArgumentError, 'expecting a Fixnum in 0..255' unless num.is_a?(Fixnum) and num.between?(0, 255)
      h_parser = Hammer::Internal.h_ch(num)

      return Hammer::Parser.new(:ch, h_parser, nil)
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
