# TODO: Find a way to make docile an optional dependency
# (autoload for this file? and throw some informative error when docile isn't available.
# should also check gem version with a 'gem' call and appropriate version specifier.)
require 'docile'

module Hammer

  class Parser
    def self.build(&block)
      ParserBuilder.new.sequence(&block).build
    end

    def self.build_choice(&block)
      ParserBuilder.new.choice(&block).build
    end
  end # class Parser

  class ParserBuilder
    attr_reader :parsers

    def initialize
      @parsers = []
    end

    def build
      if @parsers.length > 1
        Hammer::Parser.sequence(*@parsers)
      else
        @parsers.first
      end
    end


    # can call it either as ParserBuiler.new.sequence(parser1, parser2, parser3)
    # or as Parser.build { sequence { call parser1; call parser2; call parser3 } }
    def sequence(*parsers, &block)
      @parsers += parsers
      @parsers << Docile.dsl_eval(ParserBuilder.new, &block).build if block_given?
      return self
    end

    def choice(*parsers, &block)
      if block_given?
        parsers += Docile.dsl_eval(ParserBuilder.new, &block).parsers
      end
      @parsers << Hammer::Parser.choice(*parsers)
      return self
    end

    def call(parser)
      @parsers << parser
      return self
    end

    # modifies previous parser
    def action(&block)
      parser = @parsers.last
      raise RuntimeError, 'need a parser before action' if parser.nil?
      @parsers << Hammer::Parser.action(parser, &block)
      return self
    end

    # Defines a parser constructor with the given name.
    def self.define_parser(name, options = {})
      define_method name do |*args|
        # TODO: This is wrong!! Needs to accept a block for nested parsers!
        @parsers << Hammer::Parser.send(name, *args)
        return self
      end
    end
    private_class_method :define_parser

    define_parser :token
    define_parser :ch
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

    # At least indirect must return the parser instead of the builder, so it can be stored in a variable.
    # Other possible solution:
    #   Make indirect take a name parameter, and use the name to bind it later.
    #   Example:
    #     p = Hammer::Parser.build { indirect(:the_name) }
    #     p.bind(:the_name, inner_parser)
    #   (store names and parsers in hash in the builder,
    #    when building merge hashes from sub builders and store everything in the resulting sequence or choice.
    #    make Parser#bind take and optional symbol. if it is given, the name is looked up in the table.)
    # TODO:
    #   Think about this more.
    #   Do we need to be able to build parsers by chaining function calls? DSL should be sufficient.
    #   If yes, the parser methods in this class should not return "self", but the Hammer::Parser object they create.
    def indirect
      parser = Hammer::Parser.indirect
      @parsers << parser
      return parser
    end

  end # class ParserBuilder

end # module Hammer
