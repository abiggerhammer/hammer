package com.upstandinghackers.hammer;

public class Hammer
{
    public final static byte BYTE_BIG_ENDIAN = 0x1;
    public final static byte BIT_BIG_ENDIAN = 0x2;
    public final static byte BYTE_LITTLE_ENDIAN = 0x0;
    public final static byte BIT_LITTLE_ENDIAN = 0x0;
   
    public enum HTokenType
    {
        TT_NONE(1),
        TT_BYTES(2),
        TT_SINT(4),
        TT_UINT(8),
        TT_SEQUENCE(16),
        TT_ERR(32),
        TT_USER(64),
        TT_MAX(128);

        private int value;
        public int getValue() { return this.value; }
        private HTokenType(int value) { this.value = value; }
    }

    public static native HParseResult hParse(HParser parser, byte[] input, int length);
    public static native HParser hToken(byte[] str, int length);
    public static native HParser hCh(byte c);
    public static native HParser hChRange(byte from, byte to);
    public static native HParser hIntRange(HParser p, int lower, int upper);
    public static native HParser hBits(int len, boolean sign);
    public static native HParser hInt64();
    public static native HParser hInt32();
    public static native HParser hInt16();
    public static native HParser hInt8();
    public static native HParser hUInt64();
    public static native HParser hUInt32();
    public static native HParser hUInt16();
    public static native HParser hUInt8();
    public static native HParser hWhitespace(HParser p);
    public static native HParser hLeft(HParser p, HParser q);
    public static native HParser hRight(HParser p, HParser q);
    public static native HParser hMiddle(HParser p, HParser x, HParser q);
    public static native HParser hAction(HParser p, HAction a);
    public static native HParser hIn(byte[] charset, int length);
    public static native HParser hEndP();
    public static native HParser hNothingP();
    public static native HParser hSequence(HParser[] parsers);
    public static native HParser hChoice(HParser[] parsers);
    public static native HParser hButNot(HParser p1, HParser p2);
    public static native HParser hDifference(HParser p1, HParser p2);
    public static native HParser hXor(HParser p1, HParser p2);
    public static native HParser hMany(HParser p);
    public static native HParser hMany1(HParser p);
    public static native HParser hRepeatN(HParser p, int n);
    public static native HParser hOptional(HParser p);
    public static native HParser hIgnore(HParser p);
    public static native HParser hSepBy(HParser p, HParser sep);
    public static native HParser hSepBy1(HParser p, HParser sep);
    public static native HParser hEpsilonP();
    public static native HParser hLengthValue(HParser length, HParser value);
    public static native HParser hAttrBool(HParser p, HPredicate pred);
    public static native HParser hAnd(HParser p);
    public static native HParser hNot(HParser p);
    public static native HParser hIndirect();
}
