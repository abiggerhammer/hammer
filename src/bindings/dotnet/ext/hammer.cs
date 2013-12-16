using Hammer.Internal;
using System;
namespace Hammer
{

  public class Parser
  {
    internal HParser wrapped;
    internal System.Collections.IList pins; // objects that need to stay in scope for this one
    internal Parser(HParser parser)
    {
      wrapped = parser;
      pins = new System.Collections.ArrayList();
    }
    
    internal Parser Pin(Object o)
    {
      pins.Add(o);
      return this;
    }
    
    public Object Parse(byte[] str)
    {
      unsafe {
        fixed(byte* b = &str[0]) {
          HParseResult res = hammer.h_parse(wrapped, (IntPtr)b, (uint)str.Length);
          if (res != null) {
            return Unmarshal(res.ast);
          } else {
            return null;
          }
        }
      }
    }
    
    internal Object Unmarshal(HParsedToken tok)
    {
      // TODO
      return new Object();
    }
    
  }
    
  public class IndirectParser : Parser
  {
    internal IndirectParser(HParser parser)
    : base(parser)
    {
    }
    
    public void Bind(Parser p)
    {
      hammer.h_bind_indirect(this.wrapped, p.wrapped);
    }
  }
    
  public class Hammer
  {
    internal static IntPtr[] BuildParserArray(Parser[] parsers)
    {
      IntPtr[] rlist = new IntPtr[parsers.Length+1];
      for (int i = 0; i < parsers.Length; i++)
        {
          rlist[i] = HParser.getCPtr(parsers[i].wrapped).Handle;          
        }
      rlist[parsers.Length] = IntPtr.Zero;
      return rlist;
    }
    public static Parser Sequence(params Parser[] parsers)
    {
      // TODO
      IntPtr[] plist = BuildParserArray(parsers);
      unsafe
        {
          fixed (IntPtr *pp = &plist[0])
            {
              return new Parser(hammer.h_sequence__a((IntPtr)pp)).Pin(parsers);
            }
        }
    }

    public static Parser Choice(params Parser[] parsers)
    {
      // TODO
      IntPtr[] plist = BuildParserArray(parsers);
      unsafe
        {
          fixed (IntPtr *pp = &plist[0])
            {
              return new Parser(hammer.h_choice__a((IntPtr)pp)).Pin(parsers);
            }
        }
    }

    public static IndirectParser Indirect()
    {
      return new IndirectParser(hammer.h_indirect());
    }

    public static Parser Ch(byte ch)
    {
      return new Parser(hammer.h_ch(ch));
      
    }
    public static Parser Ch(char ch)
    {
      return Ch((byte)ch);
    }

    public static Parser Ch_range(byte c1, byte c2)
    {
      return new Parser(hammer.h_ch_range(c1, c2));
    }

    public static Parser Ch_range(char c1, char c2)
    {
      return new Parser(hammer.h_ch_range((byte)c1, (byte)c2));
    }

    public static Parser Int_range(Parser p, System.Int64 i1, System.Int64 i2)
    {
      return new Parser(hammer.h_int_range(p.wrapped, i1, i2));
    }
    
    public static Parser Token(byte[] token)
    {
      unsafe {
        fixed(byte* b = &token[0])
          {
            return new Parser(hammer.h_token((IntPtr)b, (uint)token.Length));
          }
      }
    }

    public static Parser In(byte[] charset)
    {
      unsafe {
        fixed(byte* b = &charset[0])
          {
            return new Parser(hammer.h_in((IntPtr)b, (uint)charset.Length));
          }
      }
    }

    public static Parser Not_in(byte[] charset)
    {
      unsafe {
        fixed(byte* b = &charset[0])
          {
            return new Parser(hammer.h_not_in((IntPtr)b, (uint)charset.Length));
          }
      }
    }

    public static Parser Token(string token)
    {
      // Encodes in UTF-8
      return Token(System.Text.Encoding.UTF8.GetBytes(token));
    }

    public static Parser In(string charset)
    {
      // Encodes in UTF-8
      return In(System.Text.Encoding.UTF8.GetBytes(charset));
    }

    public static Parser Not_in(string charset)
    {
      // Encodes in UTF-8
      return Not_in(System.Text.Encoding.UTF8.GetBytes(charset));
    }

    // No-arg parsers
    public static Parser Int8() {return new Parser(hammer.h_int8());}
    public static Parser Int16() {return new Parser(hammer.h_int16());}
    public static Parser Int32() {return new Parser(hammer.h_int32());}
    public static Parser Int64() {return new Parser(hammer.h_int64());}
    public static Parser Uint8() {return new Parser(hammer.h_uint8());}
    public static Parser Uint16() {return new Parser(hammer.h_uint16());}
    public static Parser Uint32() {return new Parser(hammer.h_uint32());}
    public static Parser Uint64() {return new Parser(hammer.h_uint64());}
    
    public static Parser End_p() {return new Parser(hammer.h_end_p());}
    public static Parser Nothing_p() {return new Parser(hammer.h_nothing_p());}
    public static Parser Epsilon_p() {return new Parser(hammer.h_epsilon_p());}

    
    // 1-arg parsers
    public static Parser Ignore(Parser p)
    {
      return new Parser(hammer.h_ignore(p.wrapped));
    }

    public static Parser Not(Parser p)
    {
      return new Parser(hammer.h_not(p.wrapped));
    }

    public static Parser Whitespace(Parser p)
    {
      return new Parser(hammer.h_whitespace(p.wrapped));
    }

    public static Parser Optional(Parser p)
    {
      return new Parser(hammer.h_optional(p.wrapped));
    }

    public static Parser And(Parser p)
    {
      return new Parser(hammer.h_and(p.wrapped));
    }
    
    public static Parser Many(Parser p)
    {
      return new Parser(hammer.h_many(p.wrapped));
    }

    public static Parser Many1(Parser p)
    {
      return new Parser(hammer.h_many1(p.wrapped));
    }

    public static Parser SepBy(Parser p, Parser sep)
    {
      return new Parser(hammer.h_sepBy(p.wrapped, sep.wrapped));
    }

    public static Parser SepBy1(Parser p, Parser sep)
    {
      return new Parser(hammer.h_sepBy1(p.wrapped, sep.wrapped));
    }

    // 2-arg parsers
    
    public static Parser Left(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_left(p1.wrapped, p2.wrapped));
    }
    public static Parser Right(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_right(p1.wrapped, p2.wrapped));
    }
    public static Parser Xor(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_xor(p1.wrapped, p2.wrapped));
    }
    public static Parser Difference(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_difference(p1.wrapped, p2.wrapped));
    }
    public static Parser Butnot(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_butnot(p1.wrapped, p2.wrapped));
    }


    // Multi-arg parsers
    public static Parser Middle(Parser p1, Parser p2, Parser p3)
    {
      return new Parser(hammer.h_middle(p1.wrapped, p2.wrapped, p3.wrapped));
    }
    public static Parser Repeat_n(Parser p, uint count)
    {
      return new Parser(hammer.h_repeat_n(p.wrapped, count));
    }

  }
  
}