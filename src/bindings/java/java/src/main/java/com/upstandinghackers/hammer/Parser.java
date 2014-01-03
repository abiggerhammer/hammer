package com.upstandinghackers.hammer;

public class Parser
{
    public native void bindIndirect(Parser inner);
    public native void free();
    public long getInner() {return this.inner;}
    public ParseResult parse(String input) {
	return Hammer.parse(this, input.getBytes(), input.length());
    }
    public ParseResult parse(byte[] input, int length) {
	return Hammer.parse(this, input, length);
    }

    private long inner;
    Parser(long inner) {this.inner=inner;}
}
