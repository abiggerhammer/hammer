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

        private int value;
        public int getValue() { return this.value; }
        private HTokenType(int value) { this.value = value; }
    }

    public static native HParseResult parse(HParser parser, byte[] input, int length);
    public static native HParser token(byte[] str, int length);
    public static native HParser ch(byte c);
    public static native HParser chRange(byte from, byte to);
    public static native HParser intRange(HParser p, int lower, int upper);
    public static native HParser bits(int len, boolean sign);
    public static native HParser int64();
    public static native HParser int32();
    public static native HParser int16();
    public static native HParser int8();
    public static native HParser uInt64();
    public static native HParser uInt32();
    public static native HParser uInt16();
    public static native HParser uInt8();
    public static native HParser whitespace(HParser p);
    public static native HParser left(HParser p, HParser q);
    public static native HParser right(HParser p, HParser q);
    public static native HParser middle(HParser p, HParser x, HParser q);
    public static native HParser action(HParser p, HAction a);
    public static native HParser in(byte[] charset, int length);
    public static native HParser endP();
    public static native HParser nothingP();
    public static native HParser sequence(HParser[] parsers);
    public static native HParser choice(HParser[] parsers);
    public static native HParser butNot(HParser p1, HParser p2);
    public static native HParser difference(HParser p1, HParser p2);
    public static native HParser xor(HParser p1, HParser p2);
    public static native HParser many(HParser p);
    public static native HParser many1(HParser p);
    public static native HParser repeatN(HParser p, int n);
    public static native HParser optional(HParser p);
    public static native HParser ignore(HParser p);
    public static native HParser sepBy(HParser p, HParser sep);
    public static native HParser sepBy1(HParser p, HParser sep);
    public static native HParser epsilonP();
    public static native HParser lengthValue(HParser length, HParser value);
    public static native HParser attrBool(HParser p, HPredicate pred);
    public static native HParser and(HParser p);
    public static native HParser not(HParser p);
    public static native HParser indirect();
}
