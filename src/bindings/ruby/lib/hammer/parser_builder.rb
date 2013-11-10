# TODO: Find a way to make docile an optional dependency
# (autoload for this file? and throw some informative error when docile isn't available.
# should also check gem version with a 'gem' call and appropriate version specifier.)
require 'docile'

module Hammer

  class Parser
    def self.build(&block)
      ParserBuilder.new.sequence(&block).build
    end
  end

  # TODO: Is this even useful for "real" usage?
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
        Hammer::Parser::Sequence.new(*@parsers)
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
      #@h_parsers << Hammer::Internal.h_token(str, str.length)
      @parsers << Hammer::Parser::Token.new(str)
      return self
    end

    def ch(char)
      #@h_parsers << Hammer::Internal.h_ch(char.ord)
      @parsers << Hammer::Parser::Ch.new(char)
      return self
    end

    # can call it either as ParserBuiler.new.sequence(parser1, parser2, parser3)
    # or as Parser.build { sequence { call parser1; call parser2; call parser3 } }
    def sequence(*parsers, &block)
      @parsers += parsers
      @parsers << Docile.dsl_eval(ParserBuilder.new, &block).build if block_given?
      return self
      #builder = Hammer::ParserBuilder.new
      #builder.instance_eval &block
      #@parsers << Hammer::Parser::Sequence.new(*builder.parsers)
      ## TODO: Save original receiver and redirect missing methods!
    end

    def choice(*parsers, &block)
      if block_given?
        parsers += Docile.dsl_eval(ParserBuilder.new, &block).parsers
      end
      @parsers << Hammer::Parser::Choice.new(*parsers)
      return self
    end
  end

end
