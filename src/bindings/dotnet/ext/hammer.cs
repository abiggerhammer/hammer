using Hammer.Internal;
using System;
using System.Runtime.InteropServices;
using System.Collections;
namespace Hammer
{

  public delegate Object HAction(Object obj);
  public delegate bool HPredicate(Object obj);

  public class ParseError : Exception
  {
    public readonly string Reason;
    public ParseError() : this(null) {}
    public ParseError(string reason) : base() {
      Reason = reason;
    }
  }
    

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
      byte[] strp;
      if (str.Length == 0)
        strp = new byte[1];
      else
        strp = str;
      try {
        unsafe {
          fixed(byte* b = &strp[0]) {
            HParseResult res = hammer.h_parse(wrapped, (IntPtr)b, (uint)str.Length);
            if (res != null) {
              // TODO: free the PR
              return Unmarshal(res.ast);
            } else {
              throw new ParseError();
            }
          }
        }
      } catch (ParseError e) {
        return null;
      }
    }
    
    internal Object Unmarshal(HParsedToken tok)
    {
      // TODO
      switch(tok.token_type) {
      case HTokenType.TT_NONE:
        return null;
      case HTokenType.TT_BYTES:
        {
          byte[] ret = new byte[tok.token_data.bytes.len];
          Marshal.Copy(tok.token_data.bytes.token,
                       ret,
                       0, ret.Length);
          return ret;
        }
      case HTokenType.TT_SINT:
        return (System.Int64)tok.token_data.sint;
      case HTokenType.TT_UINT:
        return (System.UInt64)tok.token_data._uint;
      case HTokenType.TT_SEQUENCE:
        {
          Object[] ret = new Object[tok.token_data.seq.used];
          for (uint i = 0; i < ret.Length; i++)
            ret[i] = Unmarshal(tok.token_data.seq.at(i));
          return ret;
        }
      default:
        if (tok.token_type == Hammer.tt_dotnet)
          {
            HTaggedToken tagged = hammer.h_parsed_token_get_tagged_token(tok);
            Object cb = Hammer.tag_to_action[tagged.label];
            Object unmarshalled = Unmarshal(tagged.token);
            if (cb is HAction) {
              HAction act = (HAction)cb;
              return act(unmarshalled);
            } else if (cb is HPredicate) {
              HPredicate pred = (HPredicate)cb;
              if (!pred(unmarshalled))
                throw new ParseError("Predicate failed");
              else
                return unmarshalled;
            }
          }
        throw new Exception("Should not reach here");
      }
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
    internal static IDictionary tag_to_action;
    internal static ulong charify_action;
    internal static HTokenType tt_dotnet;
    static Hammer()
    {
      tt_dotnet = hammer.h_allocate_token_type("com.upstandinghackers.hammer.dotnet.tagged");
      hammer.h_set_dotnet_tagged_token_type(tt_dotnet);
      tag_to_action = new System.Collections.Hashtable();
      charify_action = RegisterAction(x => {
          //System.Console.WriteLine(x.GetType());
          return char.ConvertFromUtf32((int)(ulong)x)[0];
        });
    }
    
    internal static ulong RegisterAction(HAction action)
    {
      ulong newAction = (ulong)tag_to_action.Count;
      tag_to_action[newAction] = action;
      return newAction;
    }
    
    internal static ulong RegisterPredicate(HPredicate predicate)
    {
      ulong newPredicate = (ulong)tag_to_action.Count;
      tag_to_action[newPredicate] = predicate;
      return newPredicate;
    }
    
    internal static Parser CharParser(Parser p)
    {
      return new Parser(hammer.h_tag(p.wrapped, charify_action)).Pin(p);
    }

    internal static byte[] ToBytes(string s)
    {
      // Probably not what you want unless you're parsing binary data.
      // This is just a one-to-one encoding of the string's codepoints
      byte[] ret = new byte[s.Length];
      for (int i = 0; i < s.Length; i++)
        {
          ret[i] = (byte)s[i];
        }
      return ret;
    }
    
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
      return CharParser(new Parser(hammer.h_ch(ch)));
      
    }
    public static Parser Ch(char ch)
    {
      return Ch((byte)ch);
    }

    public static Parser Ch_range(byte c1, byte c2)
    {
      return CharParser(new Parser(hammer.h_ch_range(c1, c2)));
    }

    public static Parser Ch_range(char c1, char c2)
    {
      return CharParser(new Parser(hammer.h_ch_range((byte)c1, (byte)c2)));
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
            return CharParser(new Parser(hammer.h_in((IntPtr)b, (uint)charset.Length)));
          }
      }
    }

    public static Parser Not_in(byte[] charset)
    {
      unsafe {
        fixed(byte* b = &charset[0])
          {
            return CharParser(new Parser(hammer.h_not_in((IntPtr)b, (uint)charset.Length)));
          }
      }
    }

    public static Parser Token(string token)
    {
      // Encodes in UTF-8
      return Token(ToBytes(token));
    }

    public static Parser In(string charset)
    {
      // Encodes in UTF-8
      return In(ToBytes(charset));
    }

    public static Parser Not_in(string charset)
    {
      // Encodes in UTF-8
      return Not_in(ToBytes(charset));
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
      return new Parser(hammer.h_ignore(p.wrapped)).Pin(p);
    }

    public static Parser Not(Parser p)
    {
      return new Parser(hammer.h_not(p.wrapped)).Pin(p);
    }

    public static Parser Whitespace(Parser p)
    {
      return new Parser(hammer.h_whitespace(p.wrapped)).Pin(p);
    }

    public static Parser Optional(Parser p)
    {
      return new Parser(hammer.h_optional(p.wrapped)).Pin(p);
    }

    public static Parser And(Parser p)
    {
      return new Parser(hammer.h_and(p.wrapped)).Pin(p);
    }
    
    public static Parser Many(Parser p)
    {
      return new Parser(hammer.h_many(p.wrapped)).Pin(p);
    }

    public static Parser Many1(Parser p)
    {
      return new Parser(hammer.h_many1(p.wrapped)).Pin(p);
    }

    public static Parser SepBy(Parser p, Parser sep)
    {
      return new Parser(hammer.h_sepBy(p.wrapped, sep.wrapped)).Pin(p);
    }

    public static Parser SepBy1(Parser p, Parser sep)
    {
      return new Parser(hammer.h_sepBy1(p.wrapped, sep.wrapped)).Pin(p);
    }

    // 2-arg parsers
    
    public static Parser Left(Parser p1, Parser p2)
    {
      return new Parser(hammer.h_left(p1.wrapped, p2.wrapped)).Pin(p1).Pin(p2);
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
    public static Parser Action(Parser p, HAction action)
    {
      ulong actionNo = Hammer.RegisterAction(action);
      return new Parser(hammer.h_tag(p.wrapped, actionNo)).Pin(p).Pin(action);
    }
    public static Parser AttrBool(Parser p, HPredicate predicate)
    {
      ulong predNo = Hammer.RegisterPredicate(predicate);
      return new Parser(hammer.h_tag(p.wrapped, predNo)).Pin(p).Pin(predicate);
    }
  }
  
}