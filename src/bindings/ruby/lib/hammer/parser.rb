module Hammer
  class Parser

    # Don't create new instances with Hammer::Parser.new,
    # use the constructor methods instead (i.e. Hammer::Parser.int64 etc.)
    def initialize
    end

    def parse(data)
      raise RuntimeError, '@h_parser is nil' if @h_parser.nil?
      raise ArgumentError, 'expecting a String' unless data.is_a? String # TODO: Not needed, FFI checks that.
      result = Hammer::Internal.h_parse(@h_parser, data, data.length);
      # TODO: Do something with the data
      !result.null?
    end

    class Token < Parser
      def initialize(string)
        @h_parser = Hammer::Internal.h_token(string, string.length)
      end
    end

    class Ch < Parser
      def initialize(char)
        # TODO: Really? Should probably accept Fixnum in appropriate range
        # Also, char.ord gives unexptected results if you pass e.g. Japanese characters: '今'.ord == 20170; Hammer::Parser::Ch.new('今').parse(202.chr) == true
        # Not really unexpected though, since 20170 & 255 == 202.
        # But probably it's better to use Ch for Fixnum in 0..255 only, and only Token for strings.
        raise ArgumentError, 'expecting a one-character String' unless char.is_a?(String) && char.length == 1
        @h_parser = Hammer::Internal.h_ch(char.ord)
      end
    end

    class Sequence < Parser
      def initialize(*parsers)
        #args = []
        #parsers.each { |p| args += [:pointer, p.h_parser] }
        args = parsers.flat_map { |p| [:pointer, p.h_parser] }
        @h_parser = Hammer::Internal.h_sequence(*args, :pointer, nil)
        @sub_parsers = parsers # store them so they don't get garbage-collected (probably not needed, though)
        # TODO: Use (managed?) FFI struct instead of void pointers
      end
    end

    class Choice < Parser
      def initialize(*parsers)
        #args = []
        #parsers.each { |p| args += [:pointer, p.h_parser] }
        args = parsers.flat_map { |p| [:pointer, p.h_parser] }
        @h_parser = Hammer::Internal.h_choice(*args, :pointer, nil)
        @sub_parsers = parsers # store them so they don't get garbage-collected (probably not needed, though)
        # TODO: Use (managed?) FFI struct instead of void pointers
      end
    end

    # Define parsers that take some number of other parsers
    # TODO: Maybe use -1 for variable number, and use this for Sequence and Choice too
    # TODO: Refactor this code as a method? And call it like: define_parser :Int64, :h_int64, 0
    [
      [:Int64,      :h_int64,       0],
      [:Int32,      :h_int32,       0],
      [:Int16,      :h_int16,       0],
      [:Int8,       :h_int8,        0],
      [:UInt64,     :h_uint64,      0],
      [:UInt32,     :h_uint32,      0],
      [:UInt16,     :h_uint16,      0],
      [:UInt8,      :h_uint8,       0],
      [:Whitespace, :h_whitespace,  1],
      [:Left,       :h_left,        2],
      [:Right,      :h_right,       2],
      [:Middle,     :h_middle,      3],
      [:End,        :h_end_p,       0],
      [:Nothing,    :h_nothing_p,   0],
      [:ButNot,     :h_butnot,      2],
      [:Difference, :h_difference,  2],
      [:Xor,        :h_xor,         2],
      [:Many,       :h_many,        1],
      [:Many1,      :h_many1,       1]
    ].each do |class_name, h_function_name, parameter_count|
      # Create new subclass of Hammer::Parser
      klass = Class.new(Hammer::Parser) do
        # Need to use define_method instead of def to be able to access h_function_name in the method's body
        define_method :initialize do |*parsers|
          # Checking parameter_count is not really needed, since the h_* methods will complain anyways
          @h_parser = Hammer::Internal.send(h_function_name, *parsers.map(&:h_parser))
          # TODO: Do we need to store sub-parsers to prevent them from getting garbage-collected?
        end
      end
      # Register class with name Hammer::Parser::ClassName
      Hammer::Parser.const_set class_name, klass
    end

    # TODO:
    # Hammer::Parser::Token.new('...') is a bit too long. Find a shorter way to use the parsers.
    # Maybe:
    #   class Hammer::Parser
    #     def self.token(*args)
    #       Hammer::Parser::Token.new(*args)
    #     end
    #   end
    # Can create functions like that automatically. Usage:
    #   h = Hammer::Parser
    #   parser = h.sequence(h.token('blah'), h.token('other_token'))
    # Looks almost like hammer in C!

    # Defines a parser constructor with the given name.
    # Options:
    #   hammer_function: name of the hammer function to call (default: 'h_'+name)
    def self.define_parser(name, options = {})
      hammer_function = options[:hammer_function] || ('h_' + name.to_s)

      # Define a new class method
      define_singleton_method name do |*parsers|
        #args = parsers.map { |p| p.instance_variable_get :@h_parser }
        h_parser = Hammer::Internal.send hammer_function, *parsers.map(&:h_parser)

        parser = Hammer::Parser.new
        parser.instance_variable_set :@h_parser, h_parser
        return parser
      end
    end
    private_class_method :define_parser

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
    define_parser :end
    define_parser :nothing
    define_parser :butnot
    define_parser :difference
    define_parser :xor
    define_parser :many
    define_parser :many1

    attr_reader :h_parser
  end
end
