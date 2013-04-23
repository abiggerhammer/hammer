/**
* Example JHammer usage
*/
public class Example
{

private HParser initParser()
{
    HParser digit = Hammer.chRange(0x30, 0x39);
    HParser alpha = Hammer.choice({Hammer.chRange(0x41, 0x5a), Hammer.chRange(0x61, 0x7a)});
    
    HParser plus = Hammer.ch('+');
    HParser slash = Hammer.ch('/');
    HParser equals = Hammer.ch('=');
    HParser bsfdig = Hammer.choice({alpha, digit, plus, slash});

    byte[] AEIMQUYcgkosw048 = "AEIMQUYcgkosw048".getBytes();
    HParser bsfdig_4bit = Hammer.in(AEIMQUYcgkosw048, AEIMQUYcgkosw048.length);
    byte[] AQgw = "AQgw".getBytes();
    HParser bsfdig_2bit = Hammer.in(AQgw, AQgw.length);
    HParser base64_3 = Hammer.repeatN(bsfdig, 4);
    HParser base64_2 = Hammer.sequence({bsfdig, bsfdig, bsfdig_4bit, equals});
    HParser base64_1 = Hammer.sequence({bsfdig, bsfdig_2bit, equals, equals});
    HParser base64 = Hammer.sequence({  Hammer.many(base64_3),
                                        Hammer.optional(Hammer.choice({base64_2, base64_1}))
                                    });

    return Hammer.sequence({Hammer.whitespace(base64), Hammer.whitespace(Hammer.endP()}});
}

public static void main(String args[])
{
    byte[] input = "RXMgaXN0IFNwYXJnZWx6ZWl0IQo=".getBytes();
    int length = input.length;
    HParsedResult result = Hammer.parse(initParser(), input, length);
    if(result == null)
    {
        System.out.println("FAIL");
    }
    else
    {
        System.out.println("PASS");
        //TODO: Pretty print
    }
}



}
