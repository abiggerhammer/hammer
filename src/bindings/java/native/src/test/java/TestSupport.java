package com.upstandinghackers.hammer;

import java.util.Arrays;
import java.math.BigInteger;
import org.testng.Assert;

public class TestSupport {
  protected boolean deepEqual(ParsedToken p, Object known) {
    //if (known != null || p == null)
    //  throw new RuntimeException("Actually got here!");
    if (p == null) {
      return known == null;
    }
    System.err.println("Saw token of type: " + p.getTokenType());
    System.err.println("Arg of type: " + (known == null ? "null" : known.getClass().toString()));
    switch (p.getTokenType()) {
    case BYTES:
      
      byte[] tmp_byte_buffer = p.getBytesValue();
      String knowns = (String)known;
      for (int i = 0; i < tmp_byte_buffer.length; i++) {
        System.err.println("Comparing char " + i);
        if (tmp_byte_buffer[i] != (byte)knowns.charAt(i)) {
          System.err.println("Compare failed at char " + i + ": " + (byte)knowns.charAt(i) + " != " + tmp_byte_buffer[i]);
          return false;
        }
      }
      System.err.println("Compare succeeded");
      return true;
    case SINT:
      return p.getSIntValue().equals(known);
    case UINT:
      return p.getUIntValue().equals(known);
    case SEQUENCE:
      int i=0;
      for (ParsedToken tok : p.getSeqValue()) {
        if (!deepEqual(tok, ((Object[])known)[i]))
          return false;
        ++i;
      }
      return true;
    case NONE:
      return known == null;
    default:
      System.err.println("Unkown token type " + p.getTokenType());
      return false;
    }
  }

  public void checkParseOK(Parser p, String input, Object expected) {
    ParseResult res = p.parse(input);
    if (res == null)
      Assert.fail("parse failed");
    if (!deepEqual(res.getAst(), expected))
      Assert.fail("Parse does not match expected; got " + res.getAst().toString());
  }

  public void checkParseFail(Parser p, String input) {
    ParseResult res = p.parse(input);
    if (res != null)
      Assert.fail("Parse unexpectedly succeeded; got " + res.getAst().toString());
  }
}
