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
      # TODO: Store an aggregator, e.g.:
      # @aggregator = Hammer::Parser::Sequence
      # Sequence is the default, set to Hammer::Parser::Choice for choice() calls
      # In the build method, use @aggregator.new(*@parsers) to build the final parser.
    end

    def build
      if @parsers.length > 1
        Hammer::Parser.sequence(*@parsers)
      else
        @parsers.first
      end
    end


    # TODO: Need to check if that's really needed
    def call(parser)
      @parsers << parser
      return self
    end


    def token(str)
      @parsers << Hammer::Parser.token(str)
      return self
    end

    def ch(char)
      @parsers << Hammer::Parser.ch(char)
      return self
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
  end

end
