package com.upstandinghackers.hammer;
import java.math.BigInteger;

public class ParsedToken
{
    public Hammer.TokenType getTokenType()
    {
        int tt = this.getTokenTypeInternal();
        if(0==tt)
            return null;
        return Hammer.tokenTypeMap.get(new Integer(tt));
    }

    private native int getTokenTypeInternal();
    public native int getIndex();
    public native byte getBitOffset();
    public native byte[] getBytesValue();
    public native BigInteger getSIntValue();
    public native BigInteger getUIntValue();
    public native double getDoubleValue();
    public native float getFloatValue();
    public native ParsedToken[] getSeqValue();
//    public native Object getUserValue();

    native void setTokenType(Hammer.TokenType type);
    native void setIndex(int index);
    native void setBitOffset(byte offset);
    native void setBytesValue(byte[] value);
  native void setSIntValue(long value); // TODO: Change these to take a biginteger
  native void setUIntValue(long value); // TODO: Change these to take a biginteger
    native void setDoubleValue(double value);
    native void setFloatValue(float value);
    native void setSeqValue(ParsedToken value[]);
//    native void setUserValue(Object value);
    
//    public native void free();
    public long getInner() {return this.inner;}

    private long inner;
    ParsedToken(long inner) {this.inner=inner;}

  private void write(StringBuilder b) {
    switch (getTokenType()) {
    case BYTES:
      byte[] bytes = getBytesValue();
      for (int i = 0; i < bytes.length; i++) {
        b.append(i == 0 ? "<" : ".");
        String byteStr = Integer.toHexString(((int)bytes[i] + 256)%256);
        if (byteStr.length() < 2) {
          b.append("0");
        }
        b.append(byteStr);
      }
      b.append(">");
      break;
    case SINT:
      b.append("s");
      b.append(getSIntValue());
      break;
    case UINT:
      b.append("u");
      b.append(getUIntValue());
      break;
    case SEQUENCE:
      boolean first = true;
      b.append("[");
      for (ParsedToken tok : getSeqValue()) {
        if (!first)
          b.append(' ');
        first = false;
        if (tok == null)
          // I don't think this can ever be the case
          b.append("null");
        else
          tok.write(b);
      }
      b.append("]");
      break;
    case NONE:
      b.append("NONE");
    }
  }
  
  public String toString() {
    StringBuilder b = new StringBuilder();
    write(b);
    return b.toString();
  }
}
  
