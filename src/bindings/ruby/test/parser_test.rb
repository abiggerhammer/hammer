require 'bundler/setup'
require 'hammer'
require 'minitest/autorun'

class ParserTest < Minitest::Test
  def test_builder_1
    parser = Hammer::Parser.build {
      token 'blah'
      ch 'a'.ord
      choice {
        sequence {
          token 'abc'
        }
        token 'def'
      }
    }

    refute_nil parser

    refute_nil parser.parse('blahaabcd')
    refute_nil parser.parse('blahadefd')
    assert_nil parser.parse('blahablad')
    assert_nil parser.parse('blaha')
    assert_nil parser.parse('blah')
  end

  def test_builder_2
    parser = Hammer::ParserBuilder.new
      .token('Hello ')
      .choice(Hammer::Parser.token('Mom'), Hammer::Parser.token('Dad'))
      .token('!')
      .build

    refute_nil parser
    refute_nil parser.parse('Hello Mom!')
  end

  def test_builder_3
    h = Hammer::Parser
    parser = h.sequence(h.token('Hello '), h.choice(h.token('Mom'), h.token('Dad')), h.token('!'))

    refute_nil parser
    refute_nil parser.parse('Hello Mom!')
  end

  def test_string_copied
    s = 'blah'
    parser = Hammer::Parser.token(s)

    refute_equal s, 'BLAH'
    assert_nil parser.parse('BLAH')

    # parser still shouldn't match, even if we modify the string in-place
    s.upcase!
    assert_equal s, 'BLAH'
    assert_nil parser.parse('BLAH')
  end

  def test_indirect
    x = nil
    parser = Hammer::Parser.build {
      token 'abc'
      x = indirect
      end_p
    }
    x.bind(Hammer::Parser.token('abd'))

    assert_nil parser.parse('abcabdabd')
    refute_nil parser.parse('abcabd')
    assert_nil parser.parse('abdabd')
    assert_nil parser.parse('abc')
  end

  def test_multibyte_token
    parser = Hammer::Parser.build {
      token '今日'
      end_p
    }

    refute_nil parser.parse('今日')
  end
end
