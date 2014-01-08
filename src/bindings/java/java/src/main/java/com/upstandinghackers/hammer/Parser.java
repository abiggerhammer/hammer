package com.upstandinghackers.hammer;

public class Parser
{
    public native void bindIndirect(Parser inner);
    public native void free();
    public long getInner() {return this.inner;}
    public ParseResult parse(String input) {
      byte[] bytes = new byte[input.length()];
      for (int i = 0; i < input.length(); i++)
        bytes[i] = (byte)input.charAt(i);
      return Hammer.parse(this, bytes, bytes.length);
    }
    public ParseResult parse(byte[] input, int length) {
	return Hammer.parse(this, input, length);
    }

    private long inner;
    Parser(long inner) {this.inner=inner;}
}
