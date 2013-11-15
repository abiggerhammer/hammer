require 'hammer/hammer_ext'
require 'hammer/internal'
require 'hammer/parser'
require 'hammer/parser_builder'

# TODO:
# Probably need to rename this file to 'hammer-parser.rb', so
# people can use "require 'hammer-parser'" in their code.



# TODO: Put tests in test/ directory.

parser = Hammer::Parser.build do
  token 'blah'
  ch 'a'
  choice {
    sequence {
      token 'abc'
    }
    token 'def'
  }
end

p parser

if parser
  p parser.parse 'blahaabcd'
  p parser.parse 'blahadefd'
  p parser.parse 'blahablad'
  p parser.parse 'blaha'
  p parser.parse 'blah'
end

parser = Hammer::Parser.build {
  token 'Hello '
  choice {
    token 'Mom'
    token 'Dad'
  }
  token '!'
}
p parser.parse 'Hello Mom!'

parser = Hammer::ParserBuilder.new
  .token('Hello ')
  .choice(Hammer::Parser.token('Mom'), Hammer::Parser.token('Dad'))
  .token('!')
  .build
p parser.parse 'Hello Mom!'

h = Hammer::Parser
parser = h.sequence(h.token('Hello '), h.choice(h.token('Mom'), h.token('Dad')), h.token('!'))
p parser.parse 'Hello Mom!'

s = 'blah'
parser = h.token(s)
p parser.parse 'BLAH' # => false
s.upcase!
p parser.parse 'BLAH' # => false
