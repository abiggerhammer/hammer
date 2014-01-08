package com.upstandinghackers.hammer; 

import java.math.BigInteger;
import java.util.Arrays;
import org.testng.annotations.*;
import org.testng.Assert;

public class HammerTest extends TestSupport {

    static {
        System.loadLibrary("hammer-java");
    }

    @Test
    public void TestToken() {
        Parser parser;
        parser = Hammer.token("95\u00a2");
            checkParseOK(parser, "95\u00a2", "95\u00a2");
            checkParseFail(parser, "95\u00a3");
    }
    @Test
    public void TestCh() {
        Parser parser;
        parser = Hammer.ch(0xa2);
            checkParseOK(parser, "\u00a2", new BigInteger("a2", 16));
            checkParseFail(parser, "\u00a3");
    }
    @Test
    public void TestChRange() {
        Parser parser;
        parser = Hammer.chRange(0x61, 0x63);
            checkParseOK(parser, "b", new BigInteger("62", 16));
            checkParseFail(parser, "d");
    }
    @Test
    public void TestInt64() {
        Parser parser;
        parser = Hammer.int64();
            checkParseOK(parser, "\u00ff\u00ff\u00ff\u00fe\u0000\u0000\u0000\u0000", new BigInteger("-200000000", 16));
            checkParseFail(parser, "\u00ff\u00ff\u00ff\u00fe\u0000\u0000\u0000");
    }
    @Test
    public void TestInt32() {
        Parser parser;
        parser = Hammer.int32();
            checkParseOK(parser, "\u00ff\u00fe\u0000\u0000", new BigInteger("-20000", 16));
            checkParseFail(parser, "\u00ff\u00fe\u0000");
            checkParseOK(parser, "\u0000\u0002\u0000\u0000", new BigInteger("20000", 16));
            checkParseFail(parser, "\u0000\u0002\u0000");
    }
    @Test
    public void TestInt16() {
        Parser parser;
        parser = Hammer.int16();
            checkParseOK(parser, "\u00fe\u0000", new BigInteger("-200", 16));
            checkParseFail(parser, "\u00fe");
            checkParseOK(parser, "\u0002\u0000", new BigInteger("200", 16));
            checkParseFail(parser, "\u0002");
    }
    @Test
    public void TestInt8() {
        Parser parser;
        parser = Hammer.int8();
            checkParseOK(parser, "\u0088", new BigInteger("-78", 16));
            checkParseFail(parser, "");
    }
    @Test
    public void TestUint64() {
        Parser parser;
        parser = Hammer.uint64();
            checkParseOK(parser, "\u0000\u0000\u0000\u0002\u0000\u0000\u0000\u0000", new BigInteger("200000000", 16));
            checkParseFail(parser, "\u0000\u0000\u0000\u0002\u0000\u0000\u0000");
    }
    @Test
    public void TestUint32() {
        Parser parser;
        parser = Hammer.uint32();
            checkParseOK(parser, "\u0000\u0002\u0000\u0000", new BigInteger("20000", 16));
            checkParseFail(parser, "\u0000\u0002\u0000");
    }
    @Test
    public void TestUint16() {
        Parser parser;
        parser = Hammer.uint16();
            checkParseOK(parser, "\u0002\u0000", new BigInteger("200", 16));
            checkParseFail(parser, "\u0002");
    }
    @Test
    public void TestUint8() {
        Parser parser;
        parser = Hammer.uint8();
            checkParseOK(parser, "x", new BigInteger("78", 16));
            checkParseFail(parser, "");
    }
    @Test
    public void TestIntRange() {
        Parser parser;
        parser = Hammer.intRange(Hammer.uint8(), 0x3, 0xa);
            checkParseOK(parser, "\u0005", new BigInteger("5", 16));
            checkParseFail(parser, "\u000b");
    }
    @Test
    public void TestWhitespace() {
        Parser parser;
        parser = Hammer.whitespace(Hammer.ch(0x61));
            checkParseOK(parser, "a", new BigInteger("61", 16));
            checkParseOK(parser, " a", new BigInteger("61", 16));
            checkParseOK(parser, "  a", new BigInteger("61", 16));
            checkParseOK(parser, "\u0009a", new BigInteger("61", 16));
            checkParseFail(parser, "_a");
        parser = Hammer.whitespace(Hammer.endP());
            checkParseOK(parser, "", null);
            checkParseOK(parser, "  ", null);
            checkParseFail(parser, "  x");
    }
    @Test
    public void TestLeft() {
        Parser parser;
        parser = Hammer.left(Hammer.ch(0x61), Hammer.ch(0x20));
            checkParseOK(parser, "a ", new BigInteger("61", 16));
            checkParseFail(parser, "a");
            checkParseFail(parser, " ");
            checkParseFail(parser, "ba");
    }
    @Test
    public void TestMiddle() {
        Parser parser;
        parser = Hammer.middle(Hammer.ch(" "), Hammer.ch("a"), Hammer.ch(" "));
            checkParseOK(parser, " a ", new BigInteger("61", 16));
            checkParseFail(parser, "a");
            checkParseFail(parser, " a");
            checkParseFail(parser, "a ");
            checkParseFail(parser, " b ");
            checkParseFail(parser, "ba ");
            checkParseFail(parser, " ab");
    }
    @Test
    public void TestIn() {
        Parser parser;
        parser = Hammer.in("abc");
            checkParseOK(parser, "b", new BigInteger("62", 16));
            checkParseFail(parser, "d");
    }
    @Test
    public void TestNotIn() {
        Parser parser;
        parser = Hammer.notIn("abc");
            checkParseOK(parser, "d", new BigInteger("64", 16));
            checkParseFail(parser, "a");
    }
    @Test
    public void TestEndP() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.endP());
            checkParseOK(parser, "a", new Object[]{ new BigInteger("61", 16)});
            checkParseFail(parser, "aa");
    }
    @Test
    public void TestNothingP() {
        Parser parser;
        parser = Hammer.nothingP();
            checkParseFail(parser, "a");
    }
    @Test
    public void TestSequence() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.ch("b"));
            checkParseOK(parser, "ab", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16)});
            checkParseFail(parser, "a");
            checkParseFail(parser, "b");
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.whitespace(Hammer.ch("b")));
            checkParseOK(parser, "ab", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16)});
            checkParseOK(parser, "a b", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16)});
            checkParseOK(parser, "a  b", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16)});
    }
    @Test
    public void TestChoice() {
        Parser parser;
        parser = Hammer.choice(Hammer.ch("a"), Hammer.ch("b"));
            checkParseOK(parser, "a", new BigInteger("61", 16));
            checkParseOK(parser, "b", new BigInteger("62", 16));
            checkParseOK(parser, "ab", new BigInteger("61", 16));
            checkParseFail(parser, "c");
    }
    @Test
    public void TestButnot() {
        Parser parser;
        parser = Hammer.butnot(Hammer.ch("a"), Hammer.token("ab"));
            checkParseOK(parser, "a", new BigInteger("61", 16));
            checkParseFail(parser, "ab");
            checkParseOK(parser, "aa", new BigInteger("61", 16));
        parser = Hammer.butnot(Hammer.chRange("0", "9"), Hammer.ch("6"));
            checkParseOK(parser, "5", new BigInteger("35", 16));
            checkParseFail(parser, "6");
    }
    @Test
    public void TestDifference() {
        Parser parser;
        parser = Hammer.difference(Hammer.token("ab"), Hammer.ch("a"));
            checkParseOK(parser, "ab", "ab");
            checkParseFail(parser, "a");
    }
    @Test
    public void TestXor() {
        Parser parser;
        parser = Hammer.xor(Hammer.chRange("0", "6"), Hammer.chRange("5", "9"));
            checkParseOK(parser, "0", new BigInteger("30", 16));
            checkParseOK(parser, "9", new BigInteger("39", 16));
            checkParseFail(parser, "5");
            checkParseFail(parser, "a");
    }
    @Test
    public void TestMany() {
        Parser parser;
        parser = Hammer.many(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")));
            checkParseOK(parser, "", new Object[]{ });
            checkParseOK(parser, "a", new Object[]{ new BigInteger("61", 16)});
            checkParseOK(parser, "b", new Object[]{ new BigInteger("62", 16)});
            checkParseOK(parser, "aabbaba", new Object[]{ new BigInteger("61", 16), new BigInteger("61", 16), new BigInteger("62", 16), new BigInteger("62", 16), new BigInteger("61", 16), new BigInteger("62", 16), new BigInteger("61", 16)});
    }
    @Test
    public void TestMany1() {
        Parser parser;
        parser = Hammer.many1(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")));
            checkParseFail(parser, "");
            checkParseOK(parser, "a", new Object[]{ new BigInteger("61", 16)});
            checkParseOK(parser, "b", new Object[]{ new BigInteger("62", 16)});
            checkParseOK(parser, "aabbaba", new Object[]{ new BigInteger("61", 16), new BigInteger("61", 16), new BigInteger("62", 16), new BigInteger("62", 16), new BigInteger("61", 16), new BigInteger("62", 16), new BigInteger("61", 16)});
            checkParseFail(parser, "daabbabadef");
    }
    @Test
    public void TestRepeatN() {
        Parser parser;
        parser = Hammer.repeatN(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")), 0x2);
            checkParseFail(parser, "adef");
            checkParseOK(parser, "abdef", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16)});
            checkParseFail(parser, "dabdef");
    }
    @Test
    public void TestOptional() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.optional(Hammer.choice(Hammer.ch("b"), Hammer.ch("c"))), Hammer.ch("d"));
            checkParseOK(parser, "abd", new Object[]{ new BigInteger("61", 16), new BigInteger("62", 16), new BigInteger("64", 16)});
            checkParseOK(parser, "acd", new Object[]{ new BigInteger("61", 16), new BigInteger("63", 16), new BigInteger("64", 16)});
            checkParseOK(parser, "ad", new Object[]{ new BigInteger("61", 16), null, new BigInteger("64", 16)});
            checkParseFail(parser, "aed");
            checkParseFail(parser, "ab");
            checkParseFail(parser, "ac");
    }
    @Test
    public void TestIgnore() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.ignore(Hammer.ch("b")), Hammer.ch("c"));
            checkParseOK(parser, "abc", new Object[]{ new BigInteger("61", 16), new BigInteger("63", 16)});
            checkParseFail(parser, "ac");
    }
    @Test
    public void TestSepBy() {
        Parser parser;
        parser = Hammer.sepBy(Hammer.choice(Hammer.ch("1"), Hammer.ch("2"), Hammer.ch("3")), Hammer.ch(","));
            checkParseOK(parser, "1,2,3", new Object[]{ new BigInteger("31", 16), new BigInteger("32", 16), new BigInteger("33", 16)});
            checkParseOK(parser, "1,3,2", new Object[]{ new BigInteger("31", 16), new BigInteger("33", 16), new BigInteger("32", 16)});
            checkParseOK(parser, "1,3", new Object[]{ new BigInteger("31", 16), new BigInteger("33", 16)});
            checkParseOK(parser, "3", new Object[]{ new BigInteger("33", 16)});
            checkParseOK(parser, "", new Object[]{ });
    }
    @Test
    public void TestSepBy1() {
        Parser parser;
        parser = Hammer.sepBy1(Hammer.choice(Hammer.ch("1"), Hammer.ch("2"), Hammer.ch("3")), Hammer.ch(","));
            checkParseOK(parser, "1,2,3", new Object[]{ new BigInteger("31", 16), new BigInteger("32", 16), new BigInteger("33", 16)});
            checkParseOK(parser, "1,3,2", new Object[]{ new BigInteger("31", 16), new BigInteger("33", 16), new BigInteger("32", 16)});
            checkParseOK(parser, "1,3", new Object[]{ new BigInteger("31", 16), new BigInteger("33", 16)});
            checkParseOK(parser, "3", new Object[]{ new BigInteger("33", 16)});
            checkParseFail(parser, "");
    }
    @Test
    public void TestAnd() {
        Parser parser;
        parser = Hammer.sequence(Hammer.and(Hammer.ch("0")), Hammer.ch("0"));
            checkParseOK(parser, "0", new Object[]{ new BigInteger("30", 16)});
            checkParseFail(parser, "1");
        parser = Hammer.sequence(Hammer.and(Hammer.ch("0")), Hammer.ch("1"));
            checkParseFail(parser, "0");
            checkParseFail(parser, "1");
        parser = Hammer.sequence(Hammer.ch("1"), Hammer.and(Hammer.ch("2")));
            checkParseOK(parser, "12", new Object[]{ new BigInteger("31", 16)});
            checkParseFail(parser, "13");
    }
    @Test
    public void TestNot() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.choice(Hammer.token("+"), Hammer.token("++")), Hammer.ch("b"));
            checkParseOK(parser, "a+b", new Object[]{ new BigInteger("61", 16), "+", new BigInteger("62", 16)});
            checkParseFail(parser, "a++b");
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.choice(Hammer.sequence(Hammer.token("+"), Hammer.not(Hammer.ch("+"))), Hammer.token("++")), Hammer.ch("b"));
            checkParseOK(parser, "a+b", new Object[]{ new BigInteger("61", 16), new Object[]{ "+"}, new BigInteger("62", 16)});
            checkParseOK(parser, "a++b", new Object[]{ new BigInteger("61", 16), "++", new BigInteger("62", 16)});
    }
    @Test
    public void TestRightrec() {
        Parser parser;
        Parser sp_rr = Hammer.indirect();
        sp_rr.bindIndirect(Hammer.choice(Hammer.sequence(Hammer.ch("a"), sp_rr), Hammer.epsilonP()));
        parser = sp_rr;
            checkParseOK(parser, "a", new Object[]{ new BigInteger("61", 16)});
            checkParseOK(parser, "aa", new Object[]{ new BigInteger("61", 16), new Object[]{ new BigInteger("61", 16)}});
            checkParseOK(parser, "aaa", new Object[]{ new BigInteger("61", 16), new Object[]{ new BigInteger("61", 16), new Object[]{ new BigInteger("61", 16)}}});
    }
}
