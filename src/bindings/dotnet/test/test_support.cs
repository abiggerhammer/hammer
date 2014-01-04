using System;
using Hammer;
namespace Hammer.Test
{
  using NUnit.Framework;

  public partial class HammerTest
  {

    protected bool DeepEquals(Object o1, Object o2)
    {
      if (o1.Equals(o2))
        return true;
      if (o1.GetType() != o2.GetType())
        return false;
      if (o1 is byte[])
        {
          byte[] a1 = (byte[])o1, a2 = (byte[])o2;
          if (a1.Length != a2.Length)
            return false;
          for (uint i = 0; i < a1.Length; i++)
            if (a1[i] != a2[i])
              return false;
          return true;
        }
      else if (o1 is Object[])
        {
          Object[] a1 = (Object[])o1, a2 = (Object[])o2;
          if (a1.Length != a2.Length)
            return false;
          for (uint i = 0; i < a1.Length; i++)
            if (!DeepEquals(a1[i],a2[i]))
              return false;
          return true;
        }
      else
        return false;
    }

    protected static string ToString(Object o)
    {
      if (o == null)
        {
          return "null";
        }
      if (o is byte[])
        {
          string ret = "<";
          byte[] a = (byte[])o;
          for (uint i = 0; i < a.Length; i++)
            {
              if (i != 0)
                ret += ".";
              ret += a[i].ToString("X2");
            }
          ret += ">";
          return ret;
        }
      else if (o is Object[])
        {
          Object[] a = (Object[])o;
          string ret = "[";
          
          for (uint i = 0; i < a.Length; i++)
            {
              if (i != 0)
                ret += " ";
              ret += ToString(a[i]);
            }
          ret += "]";
          return ret;
        }
      else if (o is System.Int64)
        {
          System.Int64 i = (System.Int64)o;
          return (i < 0 ? "s-0x" : "s0x") + i.ToString("X");
        }
      else if (o is System.UInt64)
        {
          System.UInt64 i = (System.UInt64)o;
          return "u0x" + i.ToString("X");
        }
      else if (o is System.String)
        {
          return "\"" + o.ToString() + "\"";
        }
      else if (o is System.Char)
        {
          return "\'" + o.ToString() + "\'";
        }
      else
        return "WAT(" + o.GetType() + ")";
    }
    

    internal static byte[] ToBytes(string s)
    {
      // Probably not what you want unless you're parsing binary data.
      // This is just a one-to-one encoding of the string's codepoints
      byte []ret = new byte[s.Length];
      for (int i = 0; i < s.Length; i++)
        {
          ret[i] = (byte)s[i];
        }
      return ret;
    }

    protected void CheckParseOK(Parser p, string probe, Object expected)
    {
      Object ret = p.Parse(ToBytes(probe));
      Assert.That(ret, Is.Not.Null);
      //System.Console.WriteLine(ToString(ret));
      //System.Console.WriteLine(ToString(expected));
      if (!DeepEquals(ret, expected))
        Assert.Fail();
      else
        Assert.Pass();
    }
    protected void CheckParseFail(Parser p, string probe)
    {
      Object ret = p.Parse(ToBytes(probe));
      Assert.That(ret, Is.Null);
    }
  }
}