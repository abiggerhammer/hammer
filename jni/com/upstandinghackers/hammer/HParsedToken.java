package com.upstandinghackers.hammer;

import java.util.List;

public class HParsedToken
{
    public native Hammer.HTokenType getTokenType();
    public native int getIndex();
    public native byte getBitOffset();
    public native byte[] getBytesValue();
    public native long getSIntValue();
    public native long getUIntValue();
    public native double getDoubleValue();
    public native float getFloatValue();
    public native List<HParsedToken> getSeqValue();
    public native Object getUserValue();

    native void setTokenType(Hammer.HTokenType type);
    native void setIndex(int index);
    native void setBitOffset(byte offset);
    native void setBytesValue(byte[] value);
    native void setSIntValue(long value);
    native void setUIntValue(long value);
    native void setDoubleValue(double value);
    native void setFloatValue(float value);
    native void setSeqValue(List<HParsedToken> value);
    native void setUserValue(Object value);
}
