#include "hammer.hxx"
#include "test_suite.h"
#include <glib.h>
#include <iostream>
#include <string>
//#include <vector>
//#include <boost/variant.hpp>

static void test_token_cxx(gconstpointer backend) {
    hammer::Token token_ = hammer::Token("95\xa2");

    g_check_parse_ok_cxx(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95\xa2", 3, "<39.35.a2>"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95", 2);
}

static void test_ch_cxx(gconstpointer backend) {
    hammer::Ch ch_ = hammer::Ch(0xa2);

    g_check_parse_ok_cxx(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa2", "u0xa2"); // FIXME this will fail, c++ pretty-printing doesn't do this yet
    g_check_parse_failed_cxx(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa3");
}

static void test_ch_range_cxx(gconstpointer backend) {
    hammer::ChRange range_ = hammer::ChRange('a', 'c');

    g_check_parse_ok_cxx(range_, (HParserBackend)GPOINTER_TO_INT(backend), "b", "b");
    g_check_parse_failed_cxx(range_, (HParserBackend)GPOINTER_TO_INT(backend), "d");
}

static void test_int64_cxx(gconstpointer backend) {
    hammer::Int64 int64_ = hammer::Int64();

    g_check_parse_ok_cxx(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00\x00", "s-0x200000000"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00");
}

static void test_int32_cxx(gconstpointer backend) {
    hammer::Int32 int32_ = hammer::Int32();

    g_check_parse_ok_cxx(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00\x00", "s-0x20000"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00");

    g_check_parse_ok_cxx(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", "s0x20000"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00");
}

static void test_int16_cxx(gconstpointer backend) {
    hammer::Int16 int16_ = hammer::Int16();

    g_check_parse_ok_cxx(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe\x00", "s-0x200"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe");

    g_check_parse_ok_cxx(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", "s0x200"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02");
}

static void test_int8_cxx(gconstpointer backend) {
    hammer::Int8 int8_ = hammer::Int8();

    g_check_parse_ok_cxx(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x88", "s-0x78"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "");
}

static void test_uint64_cxx(gconstpointer backend) {
    hammer::Uint64 uint64_ = hammer::Uint64();

    g_check_parse_ok_cxx(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00\x00", "u0x200000000"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00");
}

static void test_uint32_cxx(gconstpointer backend) {
    hammer::Uint32 uint32_ = hammer::Uint32();

    g_check_parse_ok_cxx(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", "u0x20000"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00", 3);
}

static void test_uint16_cxx(gconstpointer backend) {
    hammer::Uint16 uint16_ = hammer::Uint16();

    g_check_parse_ok_cxx(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", "u0x200"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02");
}

static void test_uint8_cxx(gconstpointer backend) {
    hammer::Uint8 uint8_ = hammer::Uint8();

    g_check_parse_ok_cxx(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x78", "u0x78"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "");
}

static void test_int_range_cxx(gconstpointer backend) {
    hammer::IntRange<UintResult> int_range_ = hammer::Uint8().in_range(3, 10);
  
    g_check_parse_ok_cxx(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\x05", "u0x5"); // FIXME this will fail because pretty-printing
    g_check_parse_failed_cxx(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\xb");
}

static void test_whitespace_cxx(gconstpointer backend) {
    hammer::Whitespace whitespace_ = hammer::Whitespace(hammer::Ch('a'));
    hammer::Whitespace whitespace_end = hammer::Whitespace(hammer::End());
    
    g_check_parse_ok_cxx(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "u0x61");
    g_check_parse_ok_cxx(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), " a", "u0x61");
    g_check_parse_ok_cxx(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "  a", "u0x61");
    g_check_parse_ok_cxx(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "\ta", "u0x61");
    g_check_parse_failed_cxx(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "_a");

    g_check_parse_ok_cxx(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend), "", "NULL");
    g_check_parse_ok_cxx(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  ", "NULL");
    g_check_parse_failed_cxx(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  x");
}

static void test_left_cxx(gconstpointer backend) {
    hammer::Left left_ = hammer::Left(hammer::Ch('a'), hammer::Ch(' '));

    g_check_parse_ok_cxx(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a ", "u0x61");
    g_check_parse_failed_cxx(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a");
    g_check_parse_failed_cxx(left_, (HParserBackend)GPOINTER_TO_INT(backend), " ");
    g_check_parse_failed_cxx(left_, (HParserBackend)GPOINTER_TO_INT(backend), "ab");
}

static void test_right_cxx(gconstpointer backend) {
    hammer::Right right_ = hammer::Right(hammer::Ch(' '), hammer::Ch('a'));

    g_check_parse_ok_cxx(right_, (HParserBackend)GPOINTER_TO_INT(backend), " a", "u0x61");
    g_check_parse_failed_cxx(right_, (HParserBackend)GPOINTER_TO_INT(backend), "a");
    g_check_parse_failed_cxx(right_, (HParserBackend)GPOINTER_TO_INT(backend), " ");
    g_check_parse_failed_cxx(right_, (HParserBackend)GPOINTER_TO_INT(backend), "ba");
}

static void test_middle_cxx(gconstpointer backend) {
    hammer::Middle middle_ = hammer::Middle(hammer::Ch(' '), hammer::Ch('a'), hammer::Ch(' '));

    g_check_parse_ok_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a ", "u0x61");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a ");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " b ");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "ba ");
    g_check_parse_failed_cxx(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ab");
}

static void test_in_cxx(gconstpointer backend) {
    std::string options = "abc";
    hammer::In in_ = hammer::In(options);
    g_check_parse_ok_cxx(in_, (HParserBackend)GPOINTER_TO_INT(backend), "b", "u0x62");
    g_check_parse_failed_cxx(in_, (HParserBackend)GPOINTER_TO_INT(backend), "d");

}

static void test_not_in_cxx(gconstpointer backend) {
    std::string options = "abc";
    hammer::NotIn not_in_ = hammer::NotIn(options);
    g_check_parse_ok_cxx(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "d", "u0x64");
    g_check_parse_failed_cxx(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "a");

}

static void test_end_cxx(gconstpointer backend) {
    hammer::End end_ = hammer::Sequence(hammer::Ch('a'), hammer::End());
    g_check_parse_ok_cxx(end_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
    g_check_parse_failed_cxx(end_, (HParserBackend)GPOINTER_TO_INT(backend), "aa");
}

static void test_nothing_cxx(gconstpointer backend) {
    hammer::Nothing nothing_ = hammer::Nothing();
    g_check_parse_failed_cxx(nothing_, (HParserBackend)GPOINTER_TO_INT(backend),"a");
}

static void test_sequence_cxx(gconstpointer backend) {
    hammer::Sequence sequence_1 = hammer::Sequence(hammer::Ch('a'), hammer::Ch('b'));
    hammer::Sequence sequence_2 = hammer::Sequence(hammer::Ch('a'), hammer::Whitespace(hammer::Ch('b')));

    g_check_parse_ok_cxx(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", "(u0x61 u0x62)");
    g_check_parse_failed_cxx(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "a");
    g_check_parse_failed_cxx(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "b");
    g_check_parse_ok_cxx(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "ab", "(u0x61 u0x62)");
    g_check_parse_ok_cxx(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a b", "(u0x61 u0x62)");
    g_check_parse_ok_cxx(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a  b", "(u0x61 u0x62)");  
}

static void test_choice_cxx(gconstpointer backend) {
    hammer::Choice choice_ = hammer::Choice(hammer::Ch('a'), hammer::Ch('b'));

    g_check_parse_ok_cxx(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "u0x61");
    g_check_parse_ok_cxx(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "b", "u0x62");
    g_check_parse_failed_cxx(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "c");
}

static void test_butnot_cxx(gconstpointer backend) {
    hammer::ButNot butnot_1 = hammer::ButNot(hammer::Ch('a'), hammer::Token("ab"));
    hammer::ButNot butnot_2 = hammer::ButNot(hammer::ChRange('0', '9'), hammer::Ch('6'));

    g_check_parse_ok_cxx(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "a", "u0x61");
    g_check_parse_failed_cxx(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab");
    g_check_parse_ok_cxx(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "aa", "u0x61");
    g_check_parse_failed_cxx(butnot_2, (HParserBackend)GPOINTER_TO_INT(backend), "6");
}

static void test_difference_cxx(gconstpointer backend) {
    hammer::Difference difference_ = hammer::Difference(hammer::Token("ab"), hammer::Ch('a'));

    g_check_parse_ok_cxx(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", "<61.62>");
    g_check_parse_failed_cxx(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "a");
}

static void test_xor_cxx(gconstpointer backend) {
    hammer::Xor xor_ = hammer::Xor(hammer::ChRange('0', '6'), hammer::ChRange('5', '9'));

    g_check_parse_ok_cxx(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "0", "u0x30");
    g_check_parse_ok_cxx(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "9", "u0x39");
    g_check_parse_failed_cxx(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "5");
    g_check_parse_failed_cxx(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "a");
}

static void test_many_cxx(gconstpointer backend) {
    hammer::Many many_ = hammer::Choice(hammer::Ch('a'), hammer::Ch('b')).many();

    g_check_parse_ok_cxx(many_, (HParserBackend)GPOINTER_TO_INT(backend), "", "()");
    g_check_parse_ok_cxx(many_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
    g_check_parse_ok_cxx(many_, (HParserBackend)GPOINTER_TO_INT(backend), "b", "(u0x62)");
    g_check_parse_ok_cxx(many_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
}

static void test_many1_cxx(gconstpointer backend) {
    hammer::Many1 many1_ = hammer::Choice(hammer::Ch('a'), hammer::Ch('b')).many1();

    g_check_parse_failed_cxx(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "");
    g_check_parse_ok_cxx(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
    g_check_parse_ok_cxx(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "b", "(u0x62)");
    g_check_parse_ok_cxx(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
    g_check_parse_failed_cxx(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "daabbabadef");  
}

static void test_repeat_n_cxx(gconstpointer backend) {
    hammer::RepeatN repeat_n_ = hammer::Choice(hammer::Ch('a'), hammer::Ch('b'))[2];

    g_check_parse_failed_cxx(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "adef");
    g_check_parse_ok_cxx(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "abdef", "(u0x61 u0x62)");
    g_check_parse_failed_cxx(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "dabdef");
}

static void test_optional_cxx(gconstpointer backend) {
    hammer::Optional optional_ = hammer::Sequence(hammer::Ch('a'), 
						  hammer::Choice(hammer::Ch('b'), hammer::Ch('c')).optional(), 
						  hammer::ch('d'));
  
    g_check_parse_ok_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "abd", "(u0x61 u0x62 u0x64)");
    g_check_parse_ok_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "acd", "(u0x61 u0x63 u0x64)");
    g_check_parse_ok_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ad", "(u0x61 null u0x64)");
    g_check_parse_failed_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "aed");
    g_check_parse_failed_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ab");
    g_check_parse_failed_cxx(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ac");
}

static void test_ignore_cxx(gconstpointer backend) {
    hammer::Ignore ignore_ = hammer::Sequence(hammer::Ch('a'), 
					      hammer::ch('b').ignore(), 
					      hammer::ch('c'));

    g_check_parse_ok_cxx(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "abc", "(u0x61 u0x63)");
    g_check_parse_failed_cxx(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "ac");
}

static void test_sepBy_cxx(gconstpointer backend) {
    hammer::SepBy sepBy_ = hammer::SepBy(hammer::Choice(hammer::Ch('1'), hammer::Ch('2'), hammer::Ch('3')), hammer::Ch(','));

    g_check_parse_ok_cxx(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", "(u0x31 u0x32 u0x33)");
  g_check_parse_ok_cxx(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", "(u0x31 u0x33 u0x32)");
  g_check_parse_ok_cxx(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", "(u0x31 u0x33)");
  g_check_parse_ok_cxx(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "3", "(u0x33)");
  g_check_parse_ok_cxx(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "", "()");
}

static void test_sepBy1_cxx(gconstpointer backend) {
    hammer::SepBy1 sepBy1_ = hammer::SepBy1(hammer::Choice(hammer::Ch('1'), hammer::Ch('2'), hammer::Ch('3')), hammer::Ch(','));

    g_check_parse_ok_cxx(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", "(u0x31 u0x32 u0x33)");
    g_check_parse_ok_cxx(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", "(u0x31 u0x33 u0x32)");
    g_check_parse_ok_cxx(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", "(u0x31 u0x33)");
    g_check_parse_ok_cxx(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "3", "(u0x33)");
    g_check_parse_failed_cxx(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "");
}

static void test_epsilon_cxx(gconstpointer backend) {
    hammer::Epsilon epsilon_p_1 = hammer::Sequence(hammer::Ch('a'), hammer::Epsilon(), hammer::Ch('b'));
    hammer::Epsilon epsilon_p_2 = hammer::Sequence(hammer::Epsilon(), hammer::Ch('a'));
    hammer::Epsilon epsilon_p_3 = hammer::Sequence(hammer::Ch('a'), hammer::Epsilon());
  
    g_check_parse_ok_cxx(epsilon_p_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", "(u0x61 u0x62)");
    g_check_parse_ok_cxx(epsilon_p_2, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
    g_check_parse_ok_cxx(epsilon_p_3, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
}

static void test_and_cxx(gconstpointer backend) {
    hammer::And and_1 = hammer::Sequence(hammer::And(hammer::Ch('0')), hammer::Ch('0'));
    hammer::And and_2 = hammer::Sequence(hammer::And(hammer::Ch('0')), hammer::Ch('1'));
    hammer::And and_3 = hammer::Sequence(hammer::Ch('1'), hammer::And(hammer::Ch('2')));

    g_check_parse_ok_cxx(and_1, (HParserBackend)GPOINTER_TO_INT(backend), "0", "(u0x30)");
    g_check_parse_failed_cxx(and_2, (HParserBackend)GPOINTER_TO_INT(backend), "0");
    g_check_parse_ok_cxx(and_3, (HParserBackend)GPOINTER_TO_INT(backend), "12", "(u0x31)");
}

static void test_not_cxx(gconstpointer backend) {
    hammer::Not not_1 = hammer::Sequence(hammer::Ch('a'), hammer::Choice(hammer::Ch('+'), hammer::Token("++")), hammer::Ch('b'));
    hammer::Not not_2 = hammer::Sequence(hammer::Ch('a'),
					 hammer::Choice(hammer::Sequence(hammer::Ch('+'), 
									 hammer::Not(hammer::Ch('+'))),
							hammer::Token("++")), 
					 hammer::Ch('b'));

    g_check_parse_ok_cxx(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", "(u0x61 u0x2b u0x62)");
    g_check_parse_failed_cxx(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a++b");
    g_check_parse_ok_cxx(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", "(u0x61 (u0x2b) u0x62)");
    g_check_parse_ok_cxx(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a++b", "(u0x61 <2b.2b> u0x62)");
}

static void test_leftrec(gconstpointer backend) {
    hammer::Ch a_ = hammer::Ch('a');

    hammer::Indirect lr_ = hammer::Indirect(); 

    lr_.bind(hammer::Choice(hammer::Sequence(lr_, a_), a_));

    g_check_parse_ok_cxx(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "u0x61");
    g_check_parse_ok_cxx(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", "(u0x61 u0x61)");
    g_check_parse_ok_cxx(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", "((u0x61 u0x61) u0x61)");
}

static void test_rightrec(gconstpointer backend) {
    hammer::Ch a_ = hammer::Ch('a');

    hammer::Indirect rr_ = hammer::Indirect();
    rr_.bind(hammer::Choice(hammer::Sequence(a_, rr_), hammer::Epsilon()));

    g_check_parse_ok_cxx(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", "(u0x61)");
    g_check_parse_ok_cxx(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", "(u0x61 (u0x61))");
    g_check_parse_ok_cxx(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", "(u0x61 (u0x61 (u0x61)))");
}

// FIXME won't be ready until action is ready
/*
static void test_ambiguous(gconstpointer backend) {
  hammer::Ch d_ = hammer::Ch('d');
  hammer::Ch p_ = hammer::Ch('+');
  hammer::Indirect E_ = hammer::Indirect();
  E_.bind(hammer::Choice(hammer::Sequence(E_, p_, E_), d_));
  hammer::Action expr_ = hammer::Action(E_, act_flatten);

  g_check_parse_ok_cxx(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d", "(u0x64)");
  g_check_parse_ok_cxx(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+d", "(u0x64 u0x2b u0x64)");
  g_check_parse_ok_cxx(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+d+d", "(u0x64 u0x2b u0x64 u0x2b u0x64)");
  g_check_parse_failed_cxx(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+");
}
*/

void register_cxx_tests(void) {
    g_test_add_data_func("/core/c++/parser/packrat/token", GINT_TO_POINTER(PB_PACKRAT), test_token_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/ch", GINT_TO_POINTER(PB_PACKRAT), test_ch_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/ch_range", GINT_TO_POINTER(PB_PACKRAT), test_ch_range_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/int64", GINT_TO_POINTER(PB_PACKRAT), test_int64_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/int32", GINT_TO_POINTER(PB_PACKRAT), test_int32_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/int16", GINT_TO_POINTER(PB_PACKRAT), test_int16_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/int8", GINT_TO_POINTER(PB_PACKRAT), test_int8_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/uint64", GINT_TO_POINTER(PB_PACKRAT), test_uint64_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/uint32", GINT_TO_POINTER(PB_PACKRAT), test_uint32_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/uint16", GINT_TO_POINTER(PB_PACKRAT), test_uint16_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/uint8", GINT_TO_POINTER(PB_PACKRAT), test_uint8_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/int_range", GINT_TO_POINTER(PB_PACKRAT), test_int_range_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/whitespace", GINT_TO_POINTER(PB_PACKRAT), test_whitespace_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/left", GINT_TO_POINTER(PB_PACKRAT), test_left_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/right", GINT_TO_POINTER(PB_PACKRAT), test_right_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/middle", GINT_TO_POINTER(PB_PACKRAT), test_middle_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/in", GINT_TO_POINTER(PB_PACKRAT), test_in_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/not_in", GINT_TO_POINTER(PB_PACKRAT), test_not_in_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/end", GINT_TO_POINTER(PB_PACKRAT), test_end_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/nothing", GINT_TO_POINTER(PB_PACKRAT), test_nothing_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/sequence", GINT_TO_POINTER(PB_PACKRAT), test_sequence_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/choice", GINT_TO_POINTER(PB_PACKRAT), test_choice_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/butnot", GINT_TO_POINTER(PB_PACKRAT), test_butnot_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/difference", GINT_TO_POINTER(PB_PACKRAT), test_difference_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/xor", GINT_TO_POINTER(PB_PACKRAT), test_xor_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/many", GINT_TO_POINTER(PB_PACKRAT), test_many_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/many1", GINT_TO_POINTER(PB_PACKRAT), test_many1_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/repeat_n", GINT_TO_POINTER(PB_PACKRAT), test_repeat_n_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/optional", GINT_TO_POINTER(PB_PACKRAT), test_optional_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/ignore", GINT_TO_POINTER(PB_PACKRAT), test_ignore_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/sepBy", GINT_TO_POINTER(PB_PACKRAT), test_sepBy_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/sepBy1", GINT_TO_POINTER(PB_PACKRAT), test_sepBy1_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/epsilon", GINT_TO_POINTER(PB_PACKRAT), test_epsilon_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/and", GINT_TO_POINTER(PB_PACKRAT), test_and_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/not", GINT_TO_POINTER(PB_PACKRAT), test_not_cxx);
    //    g_test_add_data_func("/core/c++/parser/packrat/leftrec", GINT_TO_POINTER(PB_PACKRAT), test_leftrec_cxx);
    g_test_add_data_func("/core/c++/parser/packrat/rightrec", GINT_TO_POINTER(PB_PACKRAT), test_rightrec_cxx);

    g_test_add_data_Func("/core/c++/parser/llk/token", GINT_TO_POINTER(PB_LLk), test_token_cxx);
    g_test_add_data_func("/core/c++/parser/llk/token", GINT_TO_POINTER(PB_LLk), test_token_cxx);
    g_test_add_data_func("/core/c++/parser/llk/ch", GINT_TO_POINTER(PB_LLk), test_ch_cxx);
    g_test_add_data_func("/core/c++/parser/llk/ch_range", GINT_TO_POINTER(PB_LLk), test_ch_range_cxx);
    g_test_add_data_func("/core/c++/parser/llk/int64", GINT_TO_POINTER(PB_LLk), test_int64_cxx);
    g_test_add_data_func("/core/c++/parser/llk/int32", GINT_TO_POINTER(PB_LLk), test_int32_cxx);
    g_test_add_data_func("/core/c++/parser/llk/int16", GINT_TO_POINTER(PB_LLk), test_int16_cxx);
    g_test_add_data_func("/core/c++/parser/llk/int8", GINT_TO_POINTER(PB_LLk), test_int8_cxx);
    g_test_add_data_func("/core/c++/parser/llk/uint64", GINT_TO_POINTER(PB_LLk), test_uint64_cxx);
    g_test_add_data_func("/core/c++/parser/llk/uint32", GINT_TO_POINTER(PB_LLk), test_uint32_cxx);
    g_test_add_data_func("/core/c++/parser/llk/uint16", GINT_TO_POINTER(PB_LLk), test_uint16_cxx);
    g_test_add_data_func("/core/c++/parser/llk/uint8", GINT_TO_POINTER(PB_LLk), test_uint8_cxx);
    g_test_add_data_func("/core/c++/parser/llk/int_range", GINT_TO_POINTER(PB_LLk), test_int_range_cxx);
#if 0
    g_test_add_data_func("/core/c++/parser/llk/float64", GINT_TO_POINTER(PB_LLk), test_float64_cxx);
    g_test_add_data_func("/core/c++/parser/llk/float32", GINT_TO_POINTER(PB_LLk), test_float32_cxx);
#endif
    g_test_add_data_func("/core/c++/parser/llk/whitespace", GINT_TO_POINTER(PB_LLk), test_whitespace_cxx);
    g_test_add_data_func("/core/c++/parser/llk/left", GINT_TO_POINTER(PB_LLk), test_left_cxx);
    g_test_add_data_func("/core/c++/parser/llk/right", GINT_TO_POINTER(PB_LLk), test_right_cxx);
    g_test_add_data_func("/core/c++/parser/llk/middle", GINT_TO_POINTER(PB_LLk), test_middle_cxx);
    //g_test_add_data_func("/core/c++/parser/llk/action", GINT_TO_POINTER(PB_LLk), test_action_cxx);
    g_test_add_data_func("/core/c++/parser/llk/in", GINT_TO_POINTER(PB_LLk), test_in_cxx);
    g_test_add_data_func("/core/c++/parser/llk/not_in", GINT_TO_POINTER(PB_LLk), test_not_in_cxx);
    g_test_add_data_func("/core/c++/parser/llk/end", GINT_TO_POINTER(PB_LLk), test_end_cxx);
    g_test_add_data_func("/core/c++/parser/llk/nothing", GINT_TO_POINTER(PB_LLk), test_nothing_cxx);
    g_test_add_data_func("/core/c++/parser/llk/sequence", GINT_TO_POINTER(PB_LLk), test_sequence_cxx);
    g_test_add_data_func("/core/c++/parser/llk/choice", GINT_TO_POINTER(PB_LLk), test_choice_cxx);
    g_test_add_data_func("/core/c++/parser/llk/many", GINT_TO_POINTER(PB_LLk), test_many_cxx);
    g_test_add_data_func("/core/c++/parser/llk/many1", GINT_TO_POINTER(PB_LLk), test_many1_cxx);
    g_test_add_data_func("/core/c++/parser/llk/optional", GINT_TO_POINTER(PB_LLk), test_optional_cxx);
    g_test_add_data_func("/core/c++/parser/llk/sepBy", GINT_TO_POINTER(PB_LLk), test_sepBy_cxx);
    g_test_add_data_func("/core/c++/parser/llk/sepBy1", GINT_TO_POINTER(PB_LLk), test_sepBy1_cxx);
    g_test_add_data_func("/core/c++/parser/llk/epsilon", GINT_TO_POINTER(PB_LLk), test_epsilon_cxx);
    //g_test_add_data_func("/core/c++/parser/llk/attr_bool", GINT_TO_POINTER(PB_LLk), test_attr_bool_cxx);
    g_test_add_data_func("/core/c++/parser/llk/ignore", GINT_TO_POINTER(PB_LLk), test_ignore_cxx);
    //g_test_add_data_func("/core/c++/parser/llk/leftrec", GINT_TO_POINTER(PB_LLk), test_leftrec_cxx);
    g_test_add_data_func("/core/c++/parser/llk/rightrec", GINT_TO_POINTER(PB_LLk), test_rightrec_cxx);

    g_test_add_data_func("/core/c++/parser/regex/token", GINT_TO_POINTER(PB_REGULAR), test_token_cxx);
    g_test_add_data_func("/core/c++/parser/regex/ch", GINT_TO_POINTER(PB_REGULAR), test_ch_cxx);
    g_test_add_data_func("/core/c++/parser/regex/ch_range", GINT_TO_POINTER(PB_REGULAR), test_ch_range_cxx);
    g_test_add_data_func("/core/c++/parser/regex/int64", GINT_TO_POINTER(PB_REGULAR), test_int64_cxx);
    g_test_add_data_func("/core/c++/parser/regex/int32", GINT_TO_POINTER(PB_REGULAR), test_int32_cxx);
    g_test_add_data_func("/core/c++/parser/regex/int16", GINT_TO_POINTER(PB_REGULAR), test_int16_cxx);
    g_test_add_data_func("/core/c++/parser/regex/int8", GINT_TO_POINTER(PB_REGULAR), test_int8_cxx);
    g_test_add_data_func("/core/c++/parser/regex/uint64", GINT_TO_POINTER(PB_REGULAR), test_uint64_cxx);
    g_test_add_data_func("/core/c++/parser/regex/uint32", GINT_TO_POINTER(PB_REGULAR), test_uint32_cxx);
    g_test_add_data_func("/core/c++/parser/regex/uint16", GINT_TO_POINTER(PB_REGULAR), test_uint16_cxx);
    g_test_add_data_func("/core/c++/parser/regex/uint8", GINT_TO_POINTER(PB_REGULAR), test_uint8_cxx);
    g_test_add_data_func("/core/c++/parser/regex/int_range", GINT_TO_POINTER(PB_REGULAR), test_int_range_cxx);
#if 0
    g_test_add_data_func("/core/c++/parser/regex/float64", GINT_TO_POINTER(PB_REGULAR), test_float64_cxx);
    g_test_add_data_func("/core/c++/parser/regex/float32", GINT_TO_POINTER(PB_REGULAR), test_float32_cxx);
#endif
    g_test_add_data_func("/core/c++/parser/regex/whitespace", GINT_TO_POINTER(PB_REGULAR), test_whitespace_cxx);
    g_test_add_data_func("/core/c++/parser/regex/left", GINT_TO_POINTER(PB_REGULAR), test_left_cxx);
    g_test_add_data_func("/core/c++/parser/regex/right", GINT_TO_POINTER(PB_REGULAR), test_right_cxx);
    g_test_add_data_func("/core/c++/parser/regex/middle", GINT_TO_POINTER(PB_REGULAR), test_middle_cxx);
    //  g_test_add_data_func("/core/c++/parser/regex/action", GINT_TO_POINTER(PB_REGULAR), test_action_cxx);
    g_test_add_data_func("/core/c++/parser/regex/in", GINT_TO_POINTER(PB_REGULAR), test_in_cxx);
    g_test_add_data_func("/core/c++/parser/regex/not_in", GINT_TO_POINTER(PB_REGULAR), test_not_in_cxx);
    g_test_add_data_func("/core/c++/parser/regex/end", GINT_TO_POINTER(PB_REGULAR), test_end_cxx);
    g_test_add_data_func("/core/c++/parser/regex/nothing", GINT_TO_POINTER(PB_REGULAR), test_nothing_cxx);
    g_test_add_data_func("/core/c++/parser/regex/sequence", GINT_TO_POINTER(PB_REGULAR), test_sequence_cxx);
    g_test_add_data_func("/core/c++/parser/regex/choice", GINT_TO_POINTER(PB_REGULAR), test_choice_cxx);
    g_test_add_data_func("/core/c++/parser/regex/many", GINT_TO_POINTER(PB_REGULAR), test_many_cxx);
    g_test_add_data_func("/core/c++/parser/regex/many1", GINT_TO_POINTER(PB_REGULAR), test_many1_cxx);
    g_test_add_data_func("/core/c++/parser/regex/repeat_n", GINT_TO_POINTER(PB_REGULAR), test_repeat_n_cxx);
    g_test_add_data_func("/core/c++/parser/regex/optional", GINT_TO_POINTER(PB_REGULAR), test_optional_cxx);
    g_test_add_data_func("/core/c++/parser/regex/sepBy", GINT_TO_POINTER(PB_REGULAR), test_sepBy_cxx);
    g_test_add_data_func("/core/c++/parser/regex/sepBy1", GINT_TO_POINTER(PB_REGULAR), test_sepBy1_cxx);
    g_test_add_data_func("/core/c++/parser/regex/epsilon", GINT_TO_POINTER(PB_REGULAR), test_epsilon_cxx);
    //  g_test_add_data_func("/core/c++/parser/regex/attr_bool", GINT_TO_POINTER(PB_REGULAR), test_attr_bool_cxx);
    g_test_add_data_func("/core/c++/parser/regex/ignore", GINT_TO_POINTER(PB_REGULAR), test_ignore_cxx);

    g_test_add_data_func("/core/c++/parser/lalr/token", GINT_TO_POINTER(PB_LALR), test_token_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/ch", GINT_TO_POINTER(PB_LALR), test_ch_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/ch_range", GINT_TO_POINTER(PB_LALR), test_ch_range_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/int64", GINT_TO_POINTER(PB_LALR), test_int64_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/int32", GINT_TO_POINTER(PB_LALR), test_int32_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/int16", GINT_TO_POINTER(PB_LALR), test_int16_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/int8", GINT_TO_POINTER(PB_LALR), test_int8_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/uint64", GINT_TO_POINTER(PB_LALR), test_uint64_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/uint32", GINT_TO_POINTER(PB_LALR), test_uint32_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/uint16", GINT_TO_POINTER(PB_LALR), test_uint16_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/uint8", GINT_TO_POINTER(PB_LALR), test_uint8_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/int_range", GINT_TO_POINTER(PB_LALR), test_int_range_cxx);
#if 0
  g_test_add_data_func("/core/c++/parser/lalr/float64", GINT_TO_POINTER(PB_LALR), test_float64_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/float32", GINT_TO_POINTER(PB_LALR), test_float32_cxx);
#endif
  g_test_add_data_func("/core/c++/parser/lalr/whitespace", GINT_TO_POINTER(PB_LALR), test_whitespace_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/left", GINT_TO_POINTER(PB_LALR), test_left_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/right", GINT_TO_POINTER(PB_LALR), test_right_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/middle", GINT_TO_POINTER(PB_LALR), test_middle_cxx);
  //  g_test_add_data_func("/core/c++/parser/lalr/action", GINT_TO_POINTER(PB_LALR), test_action_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/in", GINT_TO_POINTER(PB_LALR), test_in_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/not_in", GINT_TO_POINTER(PB_LALR), test_not_in_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/end", GINT_TO_POINTER(PB_LALR), test_end_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/nothing", GINT_TO_POINTER(PB_LALR), test_nothing_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/sequence", GINT_TO_POINTER(PB_LALR), test_sequence_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/choice", GINT_TO_POINTER(PB_LALR), test_choice_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/many", GINT_TO_POINTER(PB_LALR), test_many_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/many1", GINT_TO_POINTER(PB_LALR), test_many1_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/optional", GINT_TO_POINTER(PB_LALR), test_optional_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/sepBy", GINT_TO_POINTER(PB_LALR), test_sepBy_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/sepBy1", GINT_TO_POINTER(PB_LALR), test_sepBy1_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/epsilon", GINT_TO_POINTER(PB_LALR), test_epsilon_cxx);
  //  g_test_add_data_func("/core/c++/parser/lalr/attr_bool", GINT_TO_POINTER(PB_LALR), test_attr_bool_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/ignore", GINT_TO_POINTER(PB_LALR), test_ignore_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/leftrec", GINT_TO_POINTER(PB_LALR), test_leftrec_cxx);
  g_test_add_data_func("/core/c++/parser/lalr/rightrec", GINT_TO_POINTER(PB_LALR), test_rightrec_cxx);

    g_test_add_data_func("/core/c++/parser/glr/token", GINT_TO_POINTER(PB_GLR), test_token_cxx);
    g_test_add_data_func("/core/c++/parser/glr/ch", GINT_TO_POINTER(PB_GLR), test_ch_cxx);
    g_test_add_data_func("/core/c++/parser/glr/ch_range", GINT_TO_POINTER(PB_GLR), test_ch_range_cxx);
    g_test_add_data_func("/core/c++/parser/glr/int64", GINT_TO_POINTER(PB_GLR), test_int64_cxx);
    g_test_add_data_func("/core/c++/parser/glr/int32", GINT_TO_POINTER(PB_GLR), test_int32_cxx);
    g_test_add_data_func("/core/c++/parser/glr/int16", GINT_TO_POINTER(PB_GLR), test_int16_cxx);
    g_test_add_data_func("/core/c++/parser/glr/int8", GINT_TO_POINTER(PB_GLR), test_int8_cxx);
    g_test_add_data_func("/core/c++/parser/glr/uint64", GINT_TO_POINTER(PB_GLR), test_uint64_cxx);
    g_test_add_data_func("/core/c++/parser/glr/uint32", GINT_TO_POINTER(PB_GLR), test_uint32_cxx);
    g_test_add_data_func("/core/c++/parser/glr/uint16", GINT_TO_POINTER(PB_GLR), test_uint16_cxx);
    g_test_add_data_func("/core/c++/parser/glr/uint8", GINT_TO_POINTER(PB_GLR), test_uint8_cxx);
    g_test_add_data_func("/core/c++/parser/glr/int_range", GINT_TO_POINTER(PB_GLR), test_int_range_cxx);
#if 0
    g_test_add_data_func("/core/c++/parser/glr/float64", GINT_TO_POINTER(PB_GLR), test_float64_cxx);
    g_test_add_data_func("/core/c++/parser/glr/float32", GINT_TO_POINTER(PB_GLR), test_float32_cxx);
#endif
    g_test_add_data_func("/core/c++/parser/glr/whitespace", GINT_TO_POINTER(PB_GLR), test_whitespace_cxx);
    g_test_add_data_func("/core/c++/parser/glr/left", GINT_TO_POINTER(PB_GLR), test_left_cxx);
    g_test_add_data_func("/core/c++/parser/glr/right", GINT_TO_POINTER(PB_GLR), test_right_cxx);
    g_test_add_data_func("/core/c++/parser/glr/middle", GINT_TO_POINTER(PB_GLR), test_middle_cxx);
    g_test_add_data_func("/core/c++/parser/glr/action", GINT_TO_POINTER(PB_GLR), test_action_cxx);
    g_test_add_data_func("/core/c++/parser/glr/in", GINT_TO_POINTER(PB_GLR), test_in_cxx);
    g_test_add_data_func("/core/c++/parser/glr/not_in", GINT_TO_POINTER(PB_GLR), test_not_in_cxx);
    g_test_add_data_func("/core/c++/parser/glr/end", GINT_TO_POINTER(PB_GLR), test_end_cxx);
    g_test_add_data_func("/core/c++/parser/glr/nothing", GINT_TO_POINTER(PB_GLR), test_nothing_cxx);
    g_test_add_data_func("/core/c++/parser/glr/sequence", GINT_TO_POINTER(PB_GLR), test_sequence_cxx);
    g_test_add_data_func("/core/c++/parser/glr/choice", GINT_TO_POINTER(PB_GLR), test_choice_cxx);
    g_test_add_data_func("/core/c++/parser/glr/many", GINT_TO_POINTER(PB_GLR), test_many_cxx);
    g_test_add_data_func("/core/c++/parser/glr/many1", GINT_TO_POINTER(PB_GLR), test_many1_cxx);
    g_test_add_data_func("/core/c++/parser/glr/optional", GINT_TO_POINTER(PB_GLR), test_optional_cxx);
    g_test_add_data_func("/core/c++/parser/glr/sepBy", GINT_TO_POINTER(PB_GLR), test_sepBy_cxx);
    g_test_add_data_func("/core/c++/parser/glr/sepBy1", GINT_TO_POINTER(PB_GLR), test_sepBy1_cxx);
    g_test_add_data_func("/core/c++/parser/glr/epsilon", GINT_TO_POINTER(PB_GLR), test_epsilon_cxx);
    g_test_add_data_func("/core/c++/parser/glr/attr_bool", GINT_TO_POINTER(PB_GLR), test_attr_bool_cxx);
    g_test_add_data_func("/core/c++/parser/glr/ignore", GINT_TO_POINTER(PB_GLR), test_ignore_cxx);
    g_test_add_data_func("/core/c++/parser/glr/leftrec", GINT_TO_POINTER(PB_GLR), test_leftrec_cxx);
    g_test_add_data_func("/core/c++/parser/glr/rightrec", GINT_TO_POINTER(PB_GLR), test_rightrec_cxx);
    g_test_add_data_func("/core/c++/parser/glr/ambiguous", GINT_TO_POINTER(PB_GLR), test_ambiguous_cxx);

}

/*
int main(int argc, char **argv) {
  hammer::Ch p = hammer::Ch('a');
  std::string test = "a";
  hammer::UintResult r = p.parse(test);
  std::cerr << r.result() << std::endl;
  hammer::SequenceResult s = p.many().parse(test);
  hammer::UintResult u = boost::get<hammer::UintResult>(s.result()[0]);
  std::cerr << u.result() << std::endl;
  return 0;
}
*/
