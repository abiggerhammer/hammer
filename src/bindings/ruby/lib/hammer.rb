require 'hammer/internal'
require 'hammer/parser'
require 'hammer/parser_builder'

# TODO:
# Probably need to rename this file to 'hammer-parser.rb', so
# people can use "require 'hammer-parser'" in their code.


# Leave this in for now to be able to play around with HParseResult in irb.
x = nil
parser = Hammer::Parser.build {
  token 'abc'
  x = indirect
  end_p
}
x.bind(Hammer::Parser.token('abd'))

$r = parser.parse 'abcabd'
