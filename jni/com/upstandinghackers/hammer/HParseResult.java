package com.upstandinghackers.hammer;

import java.util.List;

public class HParseResult
{
    public native List<HParsedToken> getAst();
    public native long getBitLength();
}
