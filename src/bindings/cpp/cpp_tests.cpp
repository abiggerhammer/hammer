#include <gtest/gtest.h>
#include <hammer/hammer.hpp>
#include <hammer/hammer_test.hpp>

namespace {
  using namespace ::hammer;
  TEST(ParserTypes, Token) {
    Parser p = Token("95\xA2");
    EXPECT_TRUE(ParsesTo(p, "95\xA2", "<39.35.a2>"));
    EXPECT_TRUE(ParseFails(p, "95"));
  }

  TEST(ParserTypes, Ch) {
    Parser p = Ch(0xA2);
    EXPECT_TRUE(ParsesTo(p, "\xA2", "u0xa2"));
    EXPECT_TRUE(ParseFails(p, "\xA3"));
  }

  TEST(ParserTypes, ChRange) {
    Parser p = ChRange('a', 'c');
    EXPECT_TRUE(ParsesTo(p, "b", "u0x62"));
    EXPECT_TRUE(ParseFails(p, "d"));
  }

  TEST(ParserTypes, Int64) {
    Parser p = Int64();
    EXPECT_TRUE(ParsesTo(p, "\xff\xff\xff\xfe\x00\x00\x00\x00", "s-0x200000000"));
    EXPECT_TRUE(ParseFails(p, "\xff\xff\xff\xfe\x00\x00\x00"));
  }

  TEST(ParserTypes, Int32) {
    Parser p = Int32();
    EXPECT_TRUE(ParsesTo(p, "\xff\xfe\x00\x00", "s-0x20000"));
    EXPECT_TRUE(ParseFails(p, "\xff\xfe\x00"));
    EXPECT_TRUE(ParsesTo(p, "\x00\x02\x00\x00","s0x20000"));
    EXPECT_TRUE(ParseFails(p, "\x00\x02\x00"));
  }

  TEST(ParserTypes, Int16) {
    Parser p = Int16();
    EXPECT_TRUE(ParsesTo(p, "\xfe\x00", "s-0x200"));
    EXPECT_TRUE(ParseFails(p, "\xfe"));
    EXPECT_TRUE(ParsesTo(p, "\x02\x00", "s0x200"));
    EXPECT_TRUE(ParseFails(p, "\x01"));
  }

  TEST(ParserTypes, Int8) {
    Parser p = Int8();
    EXPECT_TRUE(ParsesTo(p, "\x88", "s-0x78"));
    EXPECT_TRUE(ParseFails(p, ""));
  }

  TEST(ParserTypes, Uint64) {
    Parser p = Uint64();
    EXPECT_TRUE(ParsesTo(p, "\x00\x00\x00\x02\x00\x00\x00\x00", "u0x200000000"));
    EXPECT_TRUE(ParseFails(p, "\x00\x00\x00\x02\x00\x00\x00"));
  }

  TEST(ParserTypes, Uint32) {
    Parser p = Uint32();
    EXPECT_TRUE(ParsesTo(p, "\x00\x02\x00\x00", "u0x20000"));
    EXPECT_TRUE(ParseFails(p, "\x00\x02\x00"));
  }

  TEST(ParserTypes, Uint16) {
    Parser p = Uint16();
    EXPECT_TRUE(ParsesTo(p, "\x02\x00", "u0x200"));
    EXPECT_TRUE(ParseFails(p, "\x02"));
  }

  TEST(ParserTypes, Uint8) {
    Parser p = Uint8();
    EXPECT_TRUE(ParsesTo(p, "\x78", "u0x78"));
    EXPECT_TRUE(ParseFails(p, ""));
  }

  TEST(ParserTypes, IntRange) {
    Parser p = IntRange(Uint8(), 3, 10);
    EXPECT_TRUE(ParsesTo(p, "\x05", "u0x5"));
    EXPECT_TRUE(ParseFails(p, "\xb"));
  }

  TEST(ParserTypes, Whitespace) {
    Parser p = Whitespace(Ch('a'));
    Parser q = Whitespace(EndP());
    EXPECT_TRUE(ParsesTo(p, "a", "u0x61"));
    EXPECT_TRUE(ParsesTo(p, " a", "u0x61"));
    EXPECT_TRUE(ParsesTo(p, "  a", "u0x61"));
    EXPECT_TRUE(ParsesTo(p, "\ta", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "_a"));

    EXPECT_TRUE(ParsesTo(q, "", "NULL"));
    EXPECT_TRUE(ParsesTo(q, "  ", "NULL"));
    EXPECT_TRUE(ParseFails(p, "  x"));
  }

  TEST(ParserTypes, Left) {
    Parser p = Left(Ch('a'), Ch(' '));
    EXPECT_TRUE(ParsesTo(p, "a ", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "a"));
    EXPECT_TRUE(ParseFails(p, " "));
    EXPECT_TRUE(ParseFails(p, "ab"));
  }

  TEST(ParserTypes, Right) {
    Parser p = Right(Ch(' '), Ch('a'));
    EXPECT_TRUE(ParsesTo(p, " a", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "a"));
    EXPECT_TRUE(ParseFails(p, " "));
    EXPECT_TRUE(ParseFails(p, "ba"));
  }

  TEST(ParserTypes, Middle) {
    Parser p = Middle(Ch(' '), Ch('a'), Ch(' '));
    EXPECT_TRUE(ParsesTo(p, " a ", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "a"));
    EXPECT_TRUE(ParseFails(p, " "));
    EXPECT_TRUE(ParseFails(p, " a"));
    EXPECT_TRUE(ParseFails(p, " b "));
    EXPECT_TRUE(ParseFails(p, "ba "));
    EXPECT_TRUE(ParseFails(p, " ab"));
  }

  // TODO action function

  TEST(ParserTypes, Action) {
    Parser p = Action(Sequence(Choice(Ch('a'), Ch('A'), NULL),
			       Choice(Ch('b'), Ch('B'), NULL),
			       NULL),
		      toupper); // this won't compile
    EXPECT_TRUE(ParsesTo(p, "ab", "(u0x41 u0x42)"));
    EXPECT_TRUE(ParsesTo(p, "AB", "(u0x41 u0x42)"));
    EXPECT_TRUE(ParseFails(p, "XX"));
  }

  TEST(ParserTypes, In) {
    Parser p = In("abc");
    EXPECT_TRUE(ParsesTo(p, "b", "u0x62"));
    EXPECT_TRUE(ParseFails(p, "d"));
  }

  TEST(ParserTypes, NotIn) {
    Parser p = NotIn("abc");
    EXPECT_TRUE(ParsesTo(p, "d", "u0x64"));
    EXPECT_TRUE(ParseFails(p, "a"));
  }

  TEST(ParserTypes, EndP) {
    Parser p = Sequence(Ch('a'), EndP(), NULL);
    EXPECT_TRUE(ParsesTo(p, "a", "(u0x61)"));
    EXPECT_TRUE(ParseFails(p, "aa"));
  }

  TEST(ParserTypes, NothingP) {
    Parser p = NothingP();
    EXPECT_TRUE(ParseFails(p, "a"));
  }

  TEST(ParserTypes, Sequence) {
    Parser p = Sequence(Ch('a'), Ch('b'), NULL);
    Parser q = Sequence(Ch('a'), Whitespace(Ch('b')), NULL);
    EXPECT_TRUE(ParsesTo(p, "ab", "(u0x61 u0x62)"));
    EXPECT_TRUE(ParseFails(p, "a"));
    EXPECT_TRUE(ParseFails(p, "b"));
    EXPECT_TRUE(ParsesTo(q, "ab", "(u0x61 u0x62)"));
    EXPECT_TRUE(ParsesTo(q, "a b", "(u0x61 u0x62)"));
    EXPECT_TRUE(ParsesTo(q, "a  b", "(u0x61 u0x62)"));
  }

  TEST(ParserTypes, Choice) {
    Parser p = Choice(Ch('a'), Ch('b'), NULL);
    EXPECT_TRUE(ParsesTo(p, "a", "u0x61"));
    EXPECT_TRUE(ParsesTo(p, "b", "u0x62"));
    EXPECT_TRUE(ParseFails(p, "c"));
  }

  TEST(ParserTypes, ButNot) {
    Parser p = ButNot(Ch('a'), Token("ab"));
    Parser q = ButNot(ChRange('0', '9'), Ch('6'));
    EXPECT_TRUE(ParsesTo(p, "a", "u0x61"));
    EXPECT_TRUE(ParseFails(p, "ab"));
    EXPECT_TRUE(ParsesTo(p, "aa", "u0x61"));
    EXPECT_TRUE(ParseFails(q, "6"));
  }

  TEST(ParserTypes, Difference) {
    Parser p = Difference(Token("ab"), Ch('a'));
    EXPECT_TRUE(ParsesTo(p, "ab", "<61.62>"));
    EXPECT_TRUE(ParseFails(p, "a"));
  }

  TEST(ParserTypes, Xor) {
    Parser p = Xor(ChRange('0', '6'), ChRange('5', '9'));
    EXPECT_TRUE(ParsesTo(p, "0", "u0x30"));
    EXPECT_TRUE(ParsesTo(p, "9", "u0x39"));
    EXPECT_TRUE(ParseFails(p, "5"));
    EXPECT_TRUE(ParseFails(p, "a"));
  }

  TEST(ParserTypes, Many) {
    Parser p = Many(Choice(Ch('a'), Ch('b'), NULL));
    EXPECT_TRUE(ParsesTo(p, "", "()"));
    EXPECT_TRUE(ParsesTo(p, "a", "(u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "b", "(u0x62)"));
    EXPECT_TRUE(ParsesTo(p, "aabbaba", "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)"));
  }

  TEST(ParserTypes, Many1) {
    Parser p = Many1(Choice(Ch('a'), Ch('b'), NULL));
    EXPECT_TRUE(ParseFails(p, "", "()"));
    EXPECT_TRUE(ParsesTo(p, "a", "(u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "b", "(u0x62)"));
    EXPECT_TRUE(ParsesTo(p, "aabbaba", "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)"));
    EXPECT_TRUE(ParseFails(p, "daabbabadef"));
  }

  TEST(ParserTypes, RepeatN) {
    Parser p = RepeatN(Choice(Ch('a'), Ch('b'), NULL), 2);
    EXPECT_TRUE(ParseFails(p, "adef"));
    EXPECT_TRUE(ParsesTo(p, "abdef", "(u0x61 u0x62)"));
    EXPECT_TRUE(ParseFails(p, "dabdef"));
  }

  TEST(ParserTypes, Optional) {
    Parser p = Sequence(Ch('a'), Optional(Choice(Ch('b'), Ch('c'), NULL)), Ch('d'), NULL);
    EXPECT_TRUE(ParsesTo(p, "abd", "(u0x61 u0x62 u0x64)"));
    EXPECT_TRUE(ParsesTo(p, "acd", "(u0x61 u0x63 u0x64)"));
    EXPECT_TRUE(ParsesTo(p, "ad", "(u0x61 null u0x64)"));
    EXPECT_TRUE(ParseFails(p, "aed"));
    EXPECT_TRUE(ParseFails(p, "ab"));
    EXPECT_TRUE(ParseFails(p, "ac"));
  }

  TEST(ParserTypes, Ignore) {
    Parser p = Sequence(Ch('a'), Ignore(Ch('b'), Ch('c'), NULL));
    EXPECT_TRUE(ParsesTo(p, "abc", "(u0x61 u0x63)"));
    EXPECT_TRUE(ParseFails(p, "ac"));
  }

  TEST(ParserTypes, SepBy) {
    Parser p = SepBy(Choice(Ch('1'), Ch('2'), Ch('3'), NULL), Ch(','));
    EXPECT_TRUE(ParsesTo(p, "1,2,3", "(u0x31 u0x32 u0x33)"));
    EXPECT_TRUE(ParsesTo(p, "1,3,2", "(u0x31 u0x33 u0x32)"));
    EXPECT_TRUE(ParsesTo(p, "1,3", "(u0x31 u0x33)"));
    EXPECT_TRUE(ParsesTo(p, "3", "(u0x33)"));
    EXPECT_TRUE(ParsesTo(p, "", "()"));
  }

  TEST(ParserTypes, SepBy1) {
    Parser p = SepBy1(Choice(Ch('1'), Ch('2'), Ch('3'), NULL), Ch(','));
    EXPECT_TRUE(ParsesTo(p, "1,2,3", "(u0x31 u0x32 u0x33)"));
    EXPECT_TRUE(ParsesTo(p, "1,3,2", "(u0x31 u0x33 u0x32)"));
    EXPECT_TRUE(ParsesTo(p, "1,3", "(u0x31 u0x33)"));
    EXPECT_TRUE(ParsesTo(p, "3", "(u0x33)"));
    EXPECT_TRUE(ParseFails(p, ""));
  }

  TEST(ParserTypes, EpsilonP) {
    Parser p = Sequence(Ch('a'), EpsilonP(), Ch('b'), NULL);
    Parser q = Sequence(EpsilonP(), Ch('a'), NULL);
    Parser r = Sequence(Ch('a'), EpsilonP(), NULL);
    EXPECT_TRUE(ParsesTo(p, "ab", "(u0x61 u0x62)"));
    EXPECT_TRUE(ParsesTo(q, "a", "(u0x61)"));
    EXPECT_TRUE(ParsesTo(r, "a", "(u0x61)"));
  }

  bool validate_test_ab() { return false; }

  TEST(ParserTypes, AttrBool) {
    Parser p = AttrBool(Many1(Choice(Ch('a'), Ch('b'), NULL)),
			validate_test_ab, NULL);
    EXPECT_TRUE(ParsesTo(p, "aa", "(u0x61 u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "bb", "(u0x62 u0x62)"));
    EXPECT_TRUE(ParseFails(p, "ab"));
  }

  TEST(ParserTypes, And) {
    Parser p = Sequence(And(Ch('0')), Ch('0'), NULL);
    Parser q = Sequence(And(Ch('0')), Ch('1'), NULL);
    Parser r = Sequence(Ch('1'), And(Ch('2')), NULL);
    EXPECT_TRUE(ParsesTo(p, "0", "(u0x30)"));
    EXPECT_TRUE(ParseFails(q, "0"));
    EXPECT_TRUE(ParsesTo(r, "12", "(u0x31)"));
  }

  TEST(ParserTypes, Not) {
    Parser p = Sequence(Ch('a'), 
			Choice(Ch('+'), Token("++"), NULL),
			Ch('b'), NULL);
    Parser q = Sequence(Ch('a'), 
			Choice(Sequence(Ch('+'), Not(Ch('+')), NULL),
			       Token("++"), NULL),
			Ch('b'), NULL);
    EXPECT_TRUE(ParsesTo(p, "a+b", "(u0x61 u0x2b u0x62)"));
    EXPECT_TRUE(ParseFails(p, "a++b"));
    EXPECT_TRUE(ParsesTo(p, "a+b", "(u0x61 (u0x2b) u0x62)"));
    EXPECT_TRUE(ParsesTo(p, "a++b", "(u0x61 <2b.2b> u0x62)"));
  }

  TEST(ParserTypes, Leftrec) {
    IndirectParser p = Indirect();
    p.bind(Choice(Sequence(p, Ch('a'), NULL), EpsilonP(), NULL));
    EXPECT_TRUE(ParsesTo(p, "a", "(u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "aa", "((u0x61) u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "aaa", "(((u0x61) u0x61) u0x61)"));
  }

  TEST(ParserTypes, Rightrec) {
    IndirectParser p = Indirect();
    p.bind(Choice(Sequence(Ch('a'), p, NULL), EpsilonP(), NULL));
    EXPECT_TRUE(ParsesTo(p, "a", "(u0x61)"));
    EXPECT_TRUE(ParsesTo(p, "aa", "(u0x61 (u0x61))"));
    EXPECT_TRUE(ParsesTo(p, "aaa", "(u0x61 (u0x61 (u0x61)))"));
  }

};

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
