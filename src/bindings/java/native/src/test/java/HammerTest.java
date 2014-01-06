package com.upstandinghackers.hammer; 

import java.math.BigInteger;
import java.util.Arrays;
import org.testng.annotations.*;
import org.testng.Assert;

public class HammerTest {

    static {
        System.loadLibrary("hammer-java");
    }

    private boolean handle(ParsedToken p, Object known) {
	System.out.println("Known: " + known);
        switch (p.getTokenType()) {
        case BYTES:
	    System.out.println("Parsed: " + Arrays.toString(p.getBytesValue()));
            return Arrays.toString(p.getBytesValue()).equals((String)known);
        case SINT:
	    System.out.println("Parsed: " + p.getSIntValue());
            return ((Long)p.getSIntValue()).equals(known);
        case UINT:
	    System.out.println("Parsed: " + p.getUIntValue());
            return ((Long)p.getUIntValue()).equals(known);
        case SEQUENCE:
            int i=0;
            for (ParsedToken tok : p.getSeqValue()) {
                if (!handle(tok, ((Object[])known)[i]))
                    return false;
                ++i;
            }
            return true;
        default:
            return false;
        }
    }

    @Test
    public void TestToken() {
        Parser parser;
        parser = Hammer.token("95\u00a2");
        Assert.assertTrue(handle(parser.parse("95\u00a2").getAst(), "95\u00a2"));
        Assert.assertNull(parser.parse("95\u00a2"));
    }
    @Test
    public void TestCh() {
        Parser parser;
        parser = Hammer.ch(0xa2);
        Assert.assertTrue(handle(parser.parse("\u00a2").getAst(), "\u00a2".getBytes()[0]));
        Assert.assertNull(parser.parse("\u00a3"));
    }
    @Test
    public void TestChRange() {
        Parser parser;
        parser = Hammer.chRange(0x61, 0x63);
        Assert.assertTrue(handle(parser.parse("b").getAst(), "b".getBytes()[0]));
        Assert.assertNull(parser.parse("d"));
    }
    @Test
    public void TestInt64() {
        Parser parser;
        parser = Hammer.int64();
        Assert.assertTrue(handle(parser.parse("\u00ff\u00ff\u00ff\u00fe\u0000\u0000\u0000\u0000").getAst(), new BigInteger("-200000000", 16)));
        Assert.assertNull(parser.parse("\u00ff\u00ff\u00ff\u00fe\u0000\u0000\u0000"));
    }
    @Test
    public void TestInt32() {
        Parser parser;
        parser = Hammer.int32();
        Assert.assertTrue(handle(parser.parse("\u00ff\u00fe\u0000\u0000").getAst(), new BigInteger("-20000", 16)));
        Assert.assertNull(parser.parse("\u00ff\u00fe\u0000"));
        Assert.assertTrue(handle(parser.parse("\u0000\u0002\u0000\u0000").getAst(), new BigInteger("20000", 16)));
        Assert.assertNull(parser.parse("\u0000\u0002\u0000"));
    }
    @Test
    public void TestInt16() {
        Parser parser;
        parser = Hammer.int16();
        Assert.assertTrue(handle(parser.parse("\u00fe\u0000").getAst(), new BigInteger("-200", 16)));
        Assert.assertNull(parser.parse("\u00fe"));
        Assert.assertTrue(handle(parser.parse("\u0002\u0000").getAst(), new BigInteger("200", 16)));
        Assert.assertNull(parser.parse("\u0002"));
    }
    @Test
    public void TestInt8() {
        Parser parser;
        parser = Hammer.int8();
        Assert.assertTrue(handle(parser.parse("\u0088").getAst(), new BigInteger("-78", 16)));
        Assert.assertNull(parser.parse(""));
    }
    @Test
    public void TestUint64() {
        Parser parser;
        parser = Hammer.uint64();
        Assert.assertTrue(handle(parser.parse("\u0000\u0000\u0000\u0002\u0000\u0000\u0000\u0000").getAst(), new BigInteger("200000000", 16)));
        Assert.assertNull(parser.parse("\u0000\u0000\u0000\u0002\u0000\u0000\u0000"));
    }
    @Test
    public void TestUint32() {
        Parser parser;
        parser = Hammer.uint32();
        Assert.assertTrue(handle(parser.parse("\u0000\u0002\u0000\u0000").getAst(), new BigInteger("20000", 16)));
        Assert.assertNull(parser.parse("\u0000\u0002\u0000"));
    }
    @Test
    public void TestUint16() {
        Parser parser;
        parser = Hammer.uint16();
        Assert.assertTrue(handle(parser.parse("\u0002\u0000").getAst(), new BigInteger("200", 16)));
        Assert.assertNull(parser.parse("\u0002"));
    }
    @Test
    public void TestUint8() {
        Parser parser;
        parser = Hammer.uint8();
        Assert.assertTrue(handle(parser.parse("x").getAst(), new BigInteger("78", 16)));
        Assert.assertNull(parser.parse(""));
    }
    @Test
    public void TestIntRange() {
        Parser parser;
        parser = Hammer.intRange(Hammer.uint8(), 0x3, 0x10);
        Assert.assertTrue(handle(parser.parse("\u0005").getAst(), new BigInteger("5", 16)));
        Assert.assertNull(parser.parse("\u000b"));
    }
    @Test
    public void TestWhitespace() {
        Parser parser;
        parser = Hammer.whitespace(Hammer.ch(0x61));
        Assert.assertTrue(handle(parser.parse("a").getAst(), "a".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse(" a").getAst(), "a".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("  a").getAst(), "a".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("\u0009a").getAst(), "a".getBytes()[0]));
        Assert.assertNull(parser.parse("_a"));
        parser = Hammer.whitespace(Hammer.endP());
        Assert.assertTrue(handle(parser.parse("").getAst(), null));
        Assert.assertTrue(handle(parser.parse("  ").getAst(), null));
        Assert.assertNull(parser.parse("  x"));
    }
    @Test
    public void TestLeft() {
        Parser parser;
        parser = Hammer.left(Hammer.ch(0x61), Hammer.ch(0x20));
        Assert.assertTrue(handle(parser.parse("a ").getAst(), "a".getBytes()[0]));
        Assert.assertNull(parser.parse("a"));
        Assert.assertNull(parser.parse(" "));
        Assert.assertNull(parser.parse("ba"));
    }
    @Test
    public void TestMiddle() {
        Parser parser;
        parser = Hammer.middle(Hammer.ch(" "), Hammer.ch("a"), Hammer.ch(" "));
        Assert.assertTrue(handle(parser.parse(" a ").getAst(), "a".getBytes()[0]));
        Assert.assertNull(parser.parse("a"));
        Assert.assertNull(parser.parse(" a"));
        Assert.assertNull(parser.parse("a "));
        Assert.assertNull(parser.parse(" b "));
        Assert.assertNull(parser.parse("ba "));
        Assert.assertNull(parser.parse(" ab"));
    }
    @Test
    public void TestIn() {
        Parser parser;
        parser = Hammer.in("abc");
        Assert.assertTrue(handle(parser.parse("b").getAst(), "b".getBytes()[0]));
        Assert.assertNull(parser.parse("d"));
    }
    @Test
    public void TestNotIn() {
        Parser parser;
        parser = Hammer.notIn("abc");
        Assert.assertTrue(handle(parser.parse("d").getAst(), "d".getBytes()[0]));
        Assert.assertNull(parser.parse("a"));
    }
    @Test
    public void TestEndP() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.endP());
        Assert.assertTrue(handle(parser.parse("a").getAst(), new Object[]{ "a".getBytes()[0]}));
        Assert.assertNull(parser.parse("aa"));
    }
    @Test
    public void TestNothingP() {
        Parser parser;
        parser = Hammer.nothingP();
        Assert.assertNull(parser.parse("a"));
    }
    @Test
    public void TestSequence() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.ch("b"));
        Assert.assertTrue(handle(parser.parse("ab").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0]}));
        Assert.assertNull(parser.parse("a"));
        Assert.assertNull(parser.parse("b"));
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.whitespace(Hammer.ch("b")));
        Assert.assertTrue(handle(parser.parse("ab").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("a b").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("a  b").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0]}));
    }
    @Test
    public void TestChoice() {
        Parser parser;
        parser = Hammer.choice(Hammer.ch("a"), Hammer.ch("b"));
        Assert.assertTrue(handle(parser.parse("a").getAst(), "a".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("b").getAst(), "b".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("ab").getAst(), "a".getBytes()[0]));
        Assert.assertNull(parser.parse("c"));
    }
    @Test
    public void TestButnot() {
        Parser parser;
        parser = Hammer.butnot(Hammer.ch("a"), Hammer.token("ab"));
        Assert.assertTrue(handle(parser.parse("a").getAst(), "a".getBytes()[0]));
        Assert.assertNull(parser.parse("ab"));
        Assert.assertTrue(handle(parser.parse("aa").getAst(), "a".getBytes()[0]));
        parser = Hammer.butnot(Hammer.chRange("0", "9"), Hammer.ch("6"));
        Assert.assertTrue(handle(parser.parse("5").getAst(), "5".getBytes()[0]));
        Assert.assertNull(parser.parse("6"));
    }
    @Test
    public void TestDifference() {
        Parser parser;
        parser = Hammer.difference(Hammer.token("ab"), Hammer.ch("a"));
        Assert.assertTrue(handle(parser.parse("ab").getAst(), "ab"));
        Assert.assertNull(parser.parse("a"));
    }
    @Test
    public void TestXor() {
        Parser parser;
        parser = Hammer.xor(Hammer.chRange("0", "6"), Hammer.chRange("5", "9"));
        Assert.assertTrue(handle(parser.parse("0").getAst(), "0".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("9").getAst(), "9".getBytes()[0]));
        Assert.assertNull(parser.parse("5"));
        Assert.assertNull(parser.parse("a"));
    }
    @Test
    public void TestMany() {
        Parser parser;
        parser = Hammer.many(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")));
        Assert.assertTrue(handle(parser.parse("").getAst(), new Object[]{ }));
        Assert.assertTrue(handle(parser.parse("a").getAst(), new Object[]{ "a".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("b").getAst(), new Object[]{ "b".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("aabbaba").getAst(), new Object[]{ "a".getBytes()[0], "a".getBytes()[0], "b".getBytes()[0], "b".getBytes()[0], "a".getBytes()[0], "b".getBytes()[0], "a".getBytes()[0]}));
    }
    @Test
    public void TestMany1() {
        Parser parser;
        parser = Hammer.many1(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")));
        Assert.assertNull(parser.parse(""));
        Assert.assertTrue(handle(parser.parse("a").getAst(), new Object[]{ "a".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("b").getAst(), new Object[]{ "b".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("aabbaba").getAst(), new Object[]{ "a".getBytes()[0], "a".getBytes()[0], "b".getBytes()[0], "b".getBytes()[0], "a".getBytes()[0], "b".getBytes()[0], "a".getBytes()[0]}));
        Assert.assertNull(parser.parse("daabbabadef"));
    }
    @Test
    public void TestRepeatN() {
        Parser parser;
        parser = Hammer.repeatN(Hammer.choice(Hammer.ch("a"), Hammer.ch("b")), 0x2);
        Assert.assertNull(parser.parse("adef"));
        Assert.assertTrue(handle(parser.parse("abdef").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0]}));
        Assert.assertNull(parser.parse("dabdef"));
    }
    @Test
    public void TestOptional() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.optional(Hammer.choice(Hammer.ch("b"), Hammer.ch("c"))), Hammer.ch("d"));
        Assert.assertTrue(handle(parser.parse("abd").getAst(), new Object[]{ "a".getBytes()[0], "b".getBytes()[0], "d".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("acd").getAst(), new Object[]{ "a".getBytes()[0], "c".getBytes()[0], "d".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("ad").getAst(), new Object[]{ "a".getBytes()[0], null, "d".getBytes()[0]}));
        Assert.assertNull(parser.parse("aed"));
        Assert.assertNull(parser.parse("ab"));
        Assert.assertNull(parser.parse("ac"));
    }
    @Test
    public void TestIgnore() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.ignore(Hammer.ch("b")), Hammer.ch("c"));
        Assert.assertTrue(handle(parser.parse("abc").getAst(), new Object[]{ "a".getBytes()[0], "c".getBytes()[0]}));
        Assert.assertNull(parser.parse("ac"));
    }
    @Test
    public void TestSepBy() {
        Parser parser;
        parser = Hammer.sepBy(Hammer.choice(Hammer.ch("1"), Hammer.ch("2"), Hammer.ch("3")), Hammer.ch(","));
        Assert.assertTrue(handle(parser.parse("1,2,3").getAst(), new Object[]{ "1".getBytes()[0], "2".getBytes()[0], "3".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("1,3,2").getAst(), new Object[]{ "1".getBytes()[0], "3".getBytes()[0], "2".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("1,3").getAst(), new Object[]{ "1".getBytes()[0], "3".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("3").getAst(), new Object[]{ "3".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("").getAst(), new Object[]{ }));
    }
    @Test
    public void TestSepBy1() {
        Parser parser;
        parser = Hammer.sepBy1(Hammer.choice(Hammer.ch("1"), Hammer.ch("2"), Hammer.ch("3")), Hammer.ch(","));
        Assert.assertTrue(handle(parser.parse("1,2,3").getAst(), new Object[]{ "1".getBytes()[0], "2".getBytes()[0], "3".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("1,3,2").getAst(), new Object[]{ "1".getBytes()[0], "3".getBytes()[0], "2".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("1,3").getAst(), new Object[]{ "1".getBytes()[0], "3".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("3").getAst(), new Object[]{ "3".getBytes()[0]}));
        Assert.assertNull(parser.parse(""));
    }
    @Test
    public void TestAnd() {
        Parser parser;
        parser = Hammer.sequence(Hammer.and(Hammer.ch("0")), Hammer.ch("0"));
        Assert.assertTrue(handle(parser.parse("0").getAst(), new Object[]{ "0".getBytes()[0]}));
        Assert.assertNull(parser.parse("1"));
        parser = Hammer.sequence(Hammer.and(Hammer.ch("0")), Hammer.ch("1"));
        Assert.assertNull(parser.parse("0"));
        Assert.assertNull(parser.parse("1"));
        parser = Hammer.sequence(Hammer.ch("1"), Hammer.and(Hammer.ch("2")));
        Assert.assertTrue(handle(parser.parse("12").getAst(), new Object[]{ "1".getBytes()[0]}));
        Assert.assertNull(parser.parse("13"));
    }
    @Test
    public void TestNot() {
        Parser parser;
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.choice(Hammer.token("+"), Hammer.token("++")), Hammer.ch("b"));
        Assert.assertTrue(handle(parser.parse("a+b").getAst(), new Object[]{ "a".getBytes()[0], "+", "b".getBytes()[0]}));
        Assert.assertNull(parser.parse("a++b"));
        parser = Hammer.sequence(Hammer.ch("a"), Hammer.choice(Hammer.sequence(Hammer.token("+"), Hammer.not(Hammer.ch("+"))), Hammer.token("++")), Hammer.ch("b"));
        Assert.assertTrue(handle(parser.parse("a+b").getAst(), new Object[]{ "a".getBytes()[0], new Object[]{ "+"}, "b".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("a++b").getAst(), new Object[]{ "a".getBytes()[0], "++", "b".getBytes()[0]}));
    }
    @Test
    public void TestRightrec() {
        Parser parser;
        Parser sp_rr = Hammer.indirect();
        sp_rr.bindIndirect(Hammer.choice(Hammer.sequence(Hammer.ch("a"), sp_rr), Hammer.epsilonP()));
        parser = sp_rr;
        Assert.assertTrue(handle(parser.parse("a").getAst(), new Object[]{ "a".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("aa").getAst(), new Object[]{ "a".getBytes()[0], new Object[]{ "a".getBytes()[0]}}));
        Assert.assertTrue(handle(parser.parse("aaa").getAst(), new Object[]{ "a".getBytes()[0], new Object[]{ "a".getBytes()[0], new Object[]{ "a".getBytes()[0]}}}));
    }
    @Test
    public void TestAmbiguous() {
        Parser parser;
        Parser sp_d = Hammer.indirect();
        Parser sp_p = Hammer.indirect();
        Parser sp_e = Hammer.indirect();
        sp_d.bindIndirect(Hammer.ch("d"));
        sp_p.bindIndirect(Hammer.ch("+"));
        sp_e.bindIndirect(Hammer.choice(Hammer.sequence(sp_e, sp_p, sp_e), sp_d));
        parser = sp_e;
        Assert.assertTrue(handle(parser.parse("d").getAst(), "d".getBytes()[0]));
        Assert.assertTrue(handle(parser.parse("d+d").getAst(), new Object[]{ "d".getBytes()[0], "+".getBytes()[0], "d".getBytes()[0]}));
        Assert.assertTrue(handle(parser.parse("d+d+d").getAst(), new Object[]{ new Object[]{ "d".getBytes()[0], "+".getBytes()[0], "d".getBytes()[0]}, "+".getBytes()[0], "d".getBytes()[0]}));
    }
}
