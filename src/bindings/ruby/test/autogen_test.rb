require 'bundler/setup'
require 'minitest/autorun'
require 'hammer'
class TestToken < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.token("95\xa2")
  end

  def test_1
    assert_parse_ok @parser_1, "95\xa2", "95\xa2"
  end

  def test_2
    refute_parse_ok @parser_1, "95\xa2"
  end
end

class TestCh < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.ch(0xa2)
  end

  def test_1
    assert_parse_ok @parser_1, "\xa2", '\xa2'
  end

  def test_2
    refute_parse_ok @parser_1, "\xa3"
  end
end

class TestChRange < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.ch_range(0x61, 0x63)
  end

  def test_1
    assert_parse_ok @parser_1, "b", 'b'
  end

  def test_2
    refute_parse_ok @parser_1, "d"
  end
end

class TestInt64 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.int64
  end

  def test_1
    assert_parse_ok @parser_1, "\xff\xff\xff\xfe\x00\x00\x00\x00", -0x200000000
  end

  def test_2
    refute_parse_ok @parser_1, "\xff\xff\xff\xfe\x00\x00\x00"
  end
end

class TestInt32 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.int32
  end

  def test_1
    assert_parse_ok @parser_1, "\xff\xfe\x00\x00", -0x20000
  end

  def test_2
    refute_parse_ok @parser_1, "\xff\xfe\x00"
  end

  def test_3
    assert_parse_ok @parser_1, "\x00\x02\x00\x00", 0x20000
  end

  def test_4
    refute_parse_ok @parser_1, "\x00\x02\x00"
  end
end

class TestInt16 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.int16
  end

  def test_1
    assert_parse_ok @parser_1, "\xfe\x00", -0x200
  end

  def test_2
    refute_parse_ok @parser_1, "\xfe"
  end

  def test_3
    assert_parse_ok @parser_1, "\x02\x00", 0x200
  end

  def test_4
    refute_parse_ok @parser_1, "\x02"
  end
end

class TestInt8 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.int8
  end

  def test_1
    assert_parse_ok @parser_1, "\x88", -0x78
  end

  def test_2
    refute_parse_ok @parser_1, ""
  end
end

class TestUint64 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.uint64
  end

  def test_1
    assert_parse_ok @parser_1, "\x00\x00\x00\x02\x00\x00\x00\x00", 0x200000000
  end

  def test_2
    refute_parse_ok @parser_1, "\x00\x00\x00\x02\x00\x00\x00"
  end
end

class TestUint32 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.uint32
  end

  def test_1
    assert_parse_ok @parser_1, "\x00\x02\x00\x00", 0x20000
  end

  def test_2
    refute_parse_ok @parser_1, "\x00\x02\x00"
  end
end

class TestUint16 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.uint16
  end

  def test_1
    assert_parse_ok @parser_1, "\x02\x00", 0x200
  end

  def test_2
    refute_parse_ok @parser_1, "\x02"
  end
end

class TestUint8 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.uint8
  end

  def test_1
    assert_parse_ok @parser_1, "x", 0x78
  end

  def test_2
    refute_parse_ok @parser_1, ""
  end
end

class TestIntRange < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.int_range(h.uint8, 0x3, 0x10)
  end

  def test_1
    assert_parse_ok @parser_1, "\x05", 0x5
  end

  def test_2
    refute_parse_ok @parser_1, "\x0b"
  end
end

class TestWhitespace < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.whitespace(h.ch(0x61))
    @parser_2 = h.whitespace(h.end_p)
  end

  def test_1
    assert_parse_ok @parser_1, "a", 'a'
  end

  def test_2
    assert_parse_ok @parser_1, " a", 'a'
  end

  def test_3
    assert_parse_ok @parser_1, "  a", 'a'
  end

  def test_4
    assert_parse_ok @parser_1, "\x09a", 'a'
  end

  def test_5
    refute_parse_ok @parser_1, "_a"
  end

  def test_6
    assert_parse_ok @parser_2, "", null
  end

  def test_7
    assert_parse_ok @parser_2, "  ", null
  end

  def test_8
    refute_parse_ok @parser_2, "  x"
  end
end

class TestLeft < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.left(h.ch(0x61), h.ch(0x20))
  end

  def test_1
    assert_parse_ok @parser_1, "a ", 'a'
  end

  def test_2
    refute_parse_ok @parser_1, "a"
  end

  def test_3
    refute_parse_ok @parser_1, " "
  end

  def test_4
    refute_parse_ok @parser_1, "ba"
  end
end

class TestMiddle < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.middle(h.ch(' '), h.ch('a'), h.ch(' '))
  end

  def test_1
    assert_parse_ok @parser_1, " a ", 'a'
  end

  def test_2
    refute_parse_ok @parser_1, "a"
  end

  def test_3
    refute_parse_ok @parser_1, " a"
  end

  def test_4
    refute_parse_ok @parser_1, "a "
  end

  def test_5
    refute_parse_ok @parser_1, " b "
  end

  def test_6
    refute_parse_ok @parser_1, "ba "
  end

  def test_7
    refute_parse_ok @parser_1, " ab"
  end
end

class TestIn < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.in("abc")
  end

  def test_1
    assert_parse_ok @parser_1, "b", 'b'
  end

  def test_2
    refute_parse_ok @parser_1, "d"
  end
end

class TestNotIn < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.not_in("abc")
  end

  def test_1
    assert_parse_ok @parser_1, "d", 'd'
  end

  def test_2
    refute_parse_ok @parser_1, "a"
  end
end

class TestEndP < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.ch('a'), h.end_p)
  end

  def test_1
    assert_parse_ok @parser_1, "a", ['a']
  end

  def test_2
    refute_parse_ok @parser_1, "aa"
  end
end

class TestNothingP < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.nothing_p
  end

  def test_1
    refute_parse_ok @parser_1, "a"
  end
end

class TestSequence < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.ch('a'), h.ch('b'))
    @parser_2 = h.sequence(h.ch('a'), h.whitespace(h.ch('b')))
  end

  def test_1
    assert_parse_ok @parser_1, "ab", ['a', 'b']
  end

  def test_2
    refute_parse_ok @parser_1, "a"
  end

  def test_3
    refute_parse_ok @parser_1, "b"
  end

  def test_4
    assert_parse_ok @parser_2, "ab", ['a', 'b']
  end

  def test_5
    assert_parse_ok @parser_2, "a b", ['a', 'b']
  end

  def test_6
    assert_parse_ok @parser_2, "a  b", ['a', 'b']
  end
end

class TestChoice < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.choice(h.ch('a'), h.ch('b'))
  end

  def test_1
    assert_parse_ok @parser_1, "a", 'a'
  end

  def test_2
    assert_parse_ok @parser_1, "b", 'b'
  end

  def test_3
    assert_parse_ok @parser_1, "ab", 'a'
  end

  def test_4
    refute_parse_ok @parser_1, "c"
  end
end

class TestButnot < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.butnot(h.ch('a'), h.token("ab"))
    @parser_2 = h.butnot(h.ch_range('0', '9'), h.ch('6'))
  end

  def test_1
    assert_parse_ok @parser_1, "a", 'a'
  end

  def test_2
    refute_parse_ok @parser_1, "ab"
  end

  def test_3
    assert_parse_ok @parser_1, "aa", 'a'
  end

  def test_4
    assert_parse_ok @parser_2, "5", '5'
  end

  def test_5
    refute_parse_ok @parser_2, "6"
  end
end

class TestDifference < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.difference(h.token("ab"), h.ch('a'))
  end

  def test_1
    assert_parse_ok @parser_1, "ab", "ab"
  end

  def test_2
    refute_parse_ok @parser_1, "a"
  end
end

class TestXor < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.xor(h.ch_range('0', '6'), h.ch_range('5', '9'))
  end

  def test_1
    assert_parse_ok @parser_1, "0", '0'
  end

  def test_2
    assert_parse_ok @parser_1, "9", '9'
  end

  def test_3
    refute_parse_ok @parser_1, "5"
  end

  def test_4
    refute_parse_ok @parser_1, "a"
  end
end

class TestMany < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.many(h.choice(h.ch('a'), h.ch('b')))
  end

  def test_1
    assert_parse_ok @parser_1, "", []
  end

  def test_2
    assert_parse_ok @parser_1, "a", ['a']
  end

  def test_3
    assert_parse_ok @parser_1, "b", ['b']
  end

  def test_4
    assert_parse_ok @parser_1, "aabbaba", ['a', 'a', 'b', 'b', 'a', 'b', 'a']
  end
end

class TestMany1 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.many1(h.choice(h.ch('a'), h.ch('b')))
  end

  def test_1
    refute_parse_ok @parser_1, ""
  end

  def test_2
    assert_parse_ok @parser_1, "a", ['a']
  end

  def test_3
    assert_parse_ok @parser_1, "b", ['b']
  end

  def test_4
    assert_parse_ok @parser_1, "aabbaba", ['a', 'a', 'b', 'b', 'a', 'b', 'a']
  end

  def test_5
    refute_parse_ok @parser_1, "daabbabadef"
  end
end

class TestRepeatN < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.repeat_n(h.choice(h.ch('a'), h.ch('b')), 0x2)
  end

  def test_1
    refute_parse_ok @parser_1, "adef"
  end

  def test_2
    assert_parse_ok @parser_1, "abdef", ['a', 'b']
  end

  def test_3
    refute_parse_ok @parser_1, "dabdef"
  end
end

class TestOptional < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.ch('a'), h.optional(h.choice(h.ch('b'), h.ch('c'))), h.ch('d'))
  end

  def test_1
    assert_parse_ok @parser_1, "abd", ['a', 'b', 'd']
  end

  def test_2
    assert_parse_ok @parser_1, "acd", ['a', 'c', 'd']
  end

  def test_3
    assert_parse_ok @parser_1, "ad", ['a', null, 'd']
  end

  def test_4
    refute_parse_ok @parser_1, "aed"
  end

  def test_5
    refute_parse_ok @parser_1, "ab"
  end

  def test_6
    refute_parse_ok @parser_1, "ac"
  end
end

class TestIgnore < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.ch('a'), h.ignore(h.ch('b')), h.ch('c'))
  end

  def test_1
    assert_parse_ok @parser_1, "abc", ['a', 'c']
  end

  def test_2
    refute_parse_ok @parser_1, "ac"
  end
end

class TestSepBy < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sepBy(h.choice(h.ch('1'), h.ch('2'), h.ch('3')), h.ch(','))
  end

  def test_1
    assert_parse_ok @parser_1, "1,2,3", ['1', '2', '3']
  end

  def test_2
    assert_parse_ok @parser_1, "1,3,2", ['1', '3', '2']
  end

  def test_3
    assert_parse_ok @parser_1, "1,3", ['1', '3']
  end

  def test_4
    assert_parse_ok @parser_1, "3", ['3']
  end

  def test_5
    assert_parse_ok @parser_1, "", []
  end
end

class TestSepBy1 < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sepBy1(h.choice(h.ch('1'), h.ch('2'), h.ch('3')), h.ch(','))
  end

  def test_1
    assert_parse_ok @parser_1, "1,2,3", ['1', '2', '3']
  end

  def test_2
    assert_parse_ok @parser_1, "1,3,2", ['1', '3', '2']
  end

  def test_3
    assert_parse_ok @parser_1, "1,3", ['1', '3']
  end

  def test_4
    assert_parse_ok @parser_1, "3", ['3']
  end

  def test_5
    refute_parse_ok @parser_1, ""
  end
end

class TestAnd < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.and(h.ch('0')), h.ch('0'))
    @parser_2 = h.sequence(h.and(h.ch('0')), h.ch('1'))
    @parser_3 = h.sequence(h.ch('1'), h.and(h.ch('2')))
  end

  def test_1
    assert_parse_ok @parser_1, "0", ['0']
  end

  def test_2
    refute_parse_ok @parser_1, "1"
  end

  def test_3
    refute_parse_ok @parser_2, "0"
  end

  def test_4
    refute_parse_ok @parser_2, "1"
  end

  def test_5
    assert_parse_ok @parser_3, "12", ['1']
  end

  def test_6
    refute_parse_ok @parser_3, "13"
  end
end

class TestNot < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @parser_1 = h.sequence(h.ch('a'), h.choice(h.token("+"), h.token("++")), h.ch('b'))
    @parser_2 = h.sequence(h.ch('a'), h.choice(h.sequence(h.token("+"), h.not(h.ch('+'))), h.token("++")), h.ch('b'))
  end

  def test_1
    assert_parse_ok @parser_1, "a+b", ['a', "+", 'b']
  end

  def test_2
    refute_parse_ok @parser_1, "a++b"
  end

  def test_3
    assert_parse_ok @parser_2, "a+b", ['a', ["+"], 'b']
  end

  def test_4
    assert_parse_ok @parser_2, "a++b", ['a', "++", 'b']
  end
end

class TestRightrec < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @sp_rr = h.indirect
    @sp_rr.bind h.choice(h.sequence(h.ch('a'), @sp_rr), h.epsilon_p)
    @parser_1 = @sp_rr
  end

  def test_1
    assert_parse_ok @parser_1, "a", ['a']
  end

  def test_2
    assert_parse_ok @parser_1, "aa", ['a', ['a']]
  end

  def test_3
    assert_parse_ok @parser_1, "aaa", ['a', ['a', ['a']]]
  end
end

class TestAmbiguous < Minitest::Test
  def setup
    super
    h = Hammer::Parser
    @sp_d = h.indirect
    @sp_p = h.indirect
    @sp_e = h.indirect
    @sp_d.bind h.ch('d')
    @sp_p.bind h.ch('+')
    @sp_e.bind h.choice(h.sequence(@sp_e, @sp_p, @sp_e), @sp_d)
    @parser_1 = @sp_e
  end

  def test_1
    assert_parse_ok @parser_1, "d", 'd'
  end

  def test_2
    assert_parse_ok @parser_1, "d+d", ['d', '+', 'd']
  end

  def test_3
    assert_parse_ok @parser_1, "d+d+d", [['d', '+', 'd'], '+', 'd']
  end
end

