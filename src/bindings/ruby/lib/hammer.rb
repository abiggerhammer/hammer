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

#$p = parser
$r = parser.parse 'abcabd'

#p $r[:ast][:data][:seq].elements.map {|e| e[:data][:bytes].token }


h = Hammer::Parser
parser =
  h.many(
    h.action(h.uint8) { |r|
      #p "TT=#{r[:ast][:token_type]}, value=#{r[:ast][:data][:uint]}"
      r.data * 2
    })

#parser = Hammer::Parser.build {
#  many {
#    uint8
#    action { |r|
#      p r
#      r[:ast]
#    }
#  }
#}

$r = parser.parse 'abcdefgh'

#p $r[:ast][:data][:seq].elements.map {|e| e[:data][:uint]}
# or:
#p $r.ast.data.map(&:data)


h = Hammer::Parser
parser = h.many(h.attr_bool(h.uint8) { |r| r.data <= 100 })
#p parser.parse('abcdefgh').ast.data.map(&:data)
