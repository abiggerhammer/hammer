package com.upstandinghackers.hammer;

import java.util.List;

public interface HAction
{
    public List<HParsedToken> execute(HParseResult p);
}
