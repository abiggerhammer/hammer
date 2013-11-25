import unittest
import hammer as h

class TestTokenParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_token("95\xa2")
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "95\xa2"), "95\xa2")
    def test_partial_fails(self):
        self.assertEqual(h.h_parse(self.parser, "95"), None)

class TestChParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser_int = h.h_ch(0xa2)
        cls.parser_chr = h.h_ch("\xa2")
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser_int, "\xa2"), 0xa2)
        self.assertEqual(h.h_parse(self.parser_chr, "\xa2"), "\xa2")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser_int, "\xa3"), None)
        self.assertEqual(h.h_parse(self.parser_chr, "\xa3"), None)

class TestChRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_ch_range("a", "c")
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "b"), "b")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "d"), None)

class TestInt64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_int64()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\xff\xff\xff\xfe\x00\x00\x00\x00"), -0x200000000)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\xff\xff\xff\xfe\x00\x00\x00"), None)

class TestInt32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_int32()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\xff\xfe\x00\x00"), -0x20000)
        self.assertEqual(h.h_parse(self.parser, "\x00\x02\x00\x00"), 0x20000)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\xff\xfe\x00"), None)
        self.assertEqual(h.h_parse(self.parser, "\x00\x02\x00"), None)

class TestInt16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_int16()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\xfe\x00"), -0x200)
        self.assertEqual(h.h_parse(self.parser, "\x02\x00"), 0x200)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\xfe"), None)
        self.assertEqual(h.h_parse(self.parser, "\x02"), None)

class TestInt8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_int8()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x88"), -0x78)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, ""), None)

class TestUint64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_uint64()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x00\x00\x00\x02\x00\x00\x00\x00"), 0x200000000)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\x00\x00\x00\x02\x00\x00\x00"), None)

class TestUint32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_uint32()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x00\x02\x00\x00"), 0x20000)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\x00\x02\x00"), None)

class TestUint16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_uint16()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x02\x00"), 0x200)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\x02"), None)

class TestUint8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_uint8()
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x78"), 0x78)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, ""), None)
        
class TestIntRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_int_range(h.h_uint8(), 3, 10)
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "\x05"), 5)
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "\x0b"), None)

class TestWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_whitespace(h.h_ch("a"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), "a")
        self.assertEqual(h.h_parse(self.parser, " a"), "a")
        self.assertEqual(h.h_parse(self.parser, "  a"), "a")
        self.assertEqual(h.h_parse(self.parser, "\ta"), "a")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "_a"), None)

class TestWhitespaceEnd(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_whitespace(h.h_end_p())
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, ""), None) # empty string
        self.assertEqual(h.h_parse(self.parser, "  "), None) # empty string
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "  x"), None)

class TestLeft(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_left(h.h_ch("a"), h.h_ch(" "))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a "), "a")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)
        self.assertEqual(h.h_parse(self.parser, " "), None)
        self.assertEqual(h.h_parse(self.parser, "ab"), None)

class TestRight(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_right(h.h_ch(" "), h.h_ch("a"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, " a"), "a")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)
        self.assertEqual(h.h_parse(self.parser, " "), None)
        self.assertEqual(h.h_parse(self.parser, "ba"), None)

class TestMiddle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_middle(h.h_ch(" "), h.h_ch("a"), h.h_ch(" "))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, " a "), "a")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)
        self.assertEqual(h.h_parse(self.parser, " "), None)
        self.assertEqual(h.h_parse(self.parser, " a"), None)
        self.assertEqual(h.h_parse(self.parser, "a "), None)
        self.assertEqual(h.h_parse(self.parser, " b "), None)
        self.assertEqual(h.h_parse(self.parser, "ba "), None)
        self.assertEqual(h.h_parse(self.parser, " ab"), None)

class TestAction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_action(h.h_sequence__a([h.h_choice__a([h.h_ch("a"), h.h_ch("A")]),
                                                 h.h_choice__a([h.h_ch("b"), h.h_ch("B")])]),
                                lambda x: [y.upper() for y in x])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), ["A", "B"])
        self.assertEqual(h.h_parse(self.parser, "AB"), ["A", "B"])
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "XX"), None)

class TestIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_in("abc")
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "b"), "b") 
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "d"), None)

class TestNotIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_not_in("abc")
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "d"), "d")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)

class TestEndP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_end_p()])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), ("a",))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "aa"), None)

class TestNothingP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_nothing_p()
    def test_success(self):
        pass
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)

class TestSequence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_ch("b")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), ('a','b'))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)
        self.assertEqual(h.h_parse(self.parser, "b"), None)

class TestSequenceWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_whitespace(h.h_ch("b"))])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), ('a','b'))
        self.assertEqual(h.h_parse(self.parser, "a b"), ('a','b'))
        self.assertEqual(h.h_parse(self.parser, "a  b"), ('a','b'))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a  c"), None)

class TestChoice(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_choice__a([h.h_ch("a"), h.h_ch("b")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), "a")
        self.assertEqual(h.h_parse(self.parser, "b"), "b")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "c"), None)

class TestButNot(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_butnot(h.h_ch("a"), h.h_token("ab"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), "a")
        self.assertEqual(h.h_parse(self.parser, "aa"), "a")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), None)

class TestButNotRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_butnot(h.h_ch_range("0", "9"), h.h_ch("6"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "4"), "4")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "6"), None)

class TestDifference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_difference(h.h_token("ab"), h.h_ch("a"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), "ab")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a"), None)

class TestXor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_xor(h.h_ch_range("0", "6"), h.h_ch_range("5", "9"))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "0"), "0")
        self.assertEqual(h.h_parse(self.parser, "9"), "9")
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "5"), None)
        self.assertEqual(h.h_parse(self.parser, "a"), None)

class TestMany(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_many(h.h_choice__a([h.h_ch("a"), h.h_ch("b")]))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, ""), ())
        self.assertEqual(h.h_parse(self.parser, "a"), ('a',))
        self.assertEqual(h.h_parse(self.parser, "b"), ('b',))
        self.assertEqual(h.h_parse(self.parser, "aabbaba"), ('a','a','b','b','a','b','a'))
    def test_failure(self):
        pass

class TestMany1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_many1(h.h_choice__a([h.h_ch("a"), h.h_ch("b")]))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), ("a",))
        self.assertEqual(h.h_parse(self.parser, "b"), ("b",))
        self.assertEqual(h.h_parse(self.parser, "aabbaba"), ("a", "a", "b", "b", "a", "b", "a"))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, ""), None)
        self.assertEqual(h.h_parse(self.parser, "daabbabadef"), None)

class TestRepeatN(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_repeat_n(h.h_choice__a([h.h_ch("a"), h.h_ch("b")]), 2)
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "abdef"), ('a', 'b'))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "adef"), None)
        self.assertEqual(h.h_parse(self.parser, "dabdef"), None)

class TestOptional(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_optional(h.h_choice__a([h.h_ch("b"), h.h_ch("c")])), h.h_ch("d")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "abd"), ('a','b','d'))
        self.assertEqual(h.h_parse(self.parser, "acd"), ('a','c','d'))
        self.assertEqual(h.h_parse(self.parser, "ad"), ('a',h.Placeholder(), 'd'))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "aed"), None)
        self.assertEqual(h.h_parse(self.parser, "ab"), None)
        self.assertEqual(h.h_parse(self.parser, "ac"), None)

class TestIgnore(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_ignore(h.h_ch("b")), h.h_ch("c")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "abc"), ("a","c"))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "ac"), None)

class TestSepBy(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sepBy(h.h_choice__a([h.h_ch("1"), h.h_ch("2"), h.h_ch("3")]), h.h_ch(","))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "1,2,3"), ('1','2','3'))
        self.assertEqual(h.h_parse(self.parser, "1,3,2"), ('1','3','2'))
        self.assertEqual(h.h_parse(self.parser, "1,3"), ('1','3'))
        self.assertEqual(h.h_parse(self.parser, "3"), ('3',))
        self.assertEqual(h.h_parse(self.parser, ""), ())
    def test_failure(self):
        pass

class TestSepBy1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sepBy1(h.h_choice__a([h.h_ch("1"), h.h_ch("2"), h.h_ch("3")]), h.h_ch(","))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "1,2,3"), ('1','2','3'))
        self.assertEqual(h.h_parse(self.parser, "1,3,2"), ('1','3','2'))
        self.assertEqual(h.h_parse(self.parser, "1,3"), ('1','3'))
        self.assertEqual(h.h_parse(self.parser, "3"), ('3',))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, ""), None)

### segfaults
class TestEpsilonP1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_epsilon_p(), h.h_ch("b")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), ("a", "b"))
    def test_failure(self):
        pass

class TestEpsilonP2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_epsilon_p(), h.h_ch("a")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), ("a",))
    def test_failure(self):
        pass

class TestEpsilonP3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_epsilon_p()])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), ("a",))
    def test_failure(self):
        pass

class TestAttrBool(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_attr_bool(h.h_many1(h.h_choice__a([h.h_ch("a"), h.h_ch("b")])), lambda x: x[0] == x[1])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "aa"), ("a", "a"))
        self.assertEqual(h.h_parse(self.parser, "bb"), ("b", "b"))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "ab"), None)

class TestAnd1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_and(h.h_ch("0")), h.h_ch("0")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "0"), ("0",))
    def test_failure(self):
        pass

class TestAnd2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_and(h.h_ch("0")), h.h_ch("1")])
    def test_success(self):
        pass
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "0"), None)

class TestAnd3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("1"), h.h_and(h.h_ch("2"))])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "12"), ('1',))
    def test_failure(self):
        pass

class TestNot1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_choice__a([h.h_ch("+"), h.h_token("++")]), h.h_ch("b")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a+b"), ("a", "+", "b"))
    def test_failure(self):
        self.assertEqual(h.h_parse(self.parser, "a++b"), None)

class TestNot2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_sequence__a([h.h_ch("a"), h.h_choice__a([h.h_sequence__a([h.h_ch("+"), h.h_not(h.h_ch("+"))]), h.h_token("++")]), h.h_ch("b")])
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a+b"), ('a', ('+',), 'b'))
        self.assertEqual(h.h_parse(self.parser, "a++b"), ('a', "++", 'b'))
    def test_failure(self):
        pass

# ### this is commented out for packrat in C ...
# #class TestLeftrec(unittest.TestCase):
# #    @classmethod
# #    def setUpClass(cls):
# #        cls.parser = h.h_indirect()
# #        a = h.h_ch("a")
# #        h.h_bind_indirect(cls.parser, h.h_choice(h.h_sequence(cls.parser, a), a))
# #    def test_success(self):
# #        self.assertEqual(h.h_parse(self.parser, "a"), "a")
# #        self.assertEqual(h.h_parse(self.parser, "aa"), ["a", "a"])
# #        self.assertEqual(h.h_parse(self.parser, "aaa"), ["a", "a", "a"])
# #    def test_failure(self):
# #        pass

class TestRightrec(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.h_indirect()
        a = h.h_ch("a")
        h.h_bind_indirect(cls.parser, h.h_choice__a([h.h_sequence__a([a, cls.parser]), h.h_epsilon_p()]))
    def test_success(self):
        self.assertEqual(h.h_parse(self.parser, "a"), ('a',))
        self.assertEqual(h.h_parse(self.parser, "aa"), ('a', ('a',)))
        self.assertEqual(h.h_parse(self.parser, "aaa"), ('a', ('a', ('a',))))
    def test_failure(self):
        pass

# ### this is just for GLR
# #class TestAmbiguous(unittest.TestCase):
# #    @classmethod
# #    def setUpClass(cls):
# #        cls.parser = h.h_indirect()
# #        d = h.h_ch("d")
# #        p = h.h_ch("+")
# #        h.h_bind_indirect(cls.parser, h.h_choice(h.h_sequence(cls.parser, p, cls.parser), d))
# #        # this is supposed to be flattened
# #    def test_success(self):
# #        self.assertEqual(h.h_parse(self.parser, "d"), ["d"])
# #        self.assertEqual(h.h_parse(self.parser, "d+d"), ["d", "+", "d"])
# #        self.assertEqual(h.h_parse(self.parser, "d+d+d"), ["d", "+", "d", "+", "d"])
# #    def test_failure(self):
# #        self.assertEqual(h.h_parse(self.parser, "d+"), None)

