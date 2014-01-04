namespace Hammer.Test
{
  using NUnit.Framework;
  [TestFixture]
  public partial class HammerTest
  {
    [Test]
    public void TestAction()
    {
      Parser parser = Hammer.Action(Hammer.Sequence(Hammer.Choice(Hammer.Ch('a'),
                                                                  Hammer.Ch('A')),
                                                    Hammer.Choice(Hammer.Ch('b'),
                                                                  Hammer.Ch('B'))),
                                    (HAction)(x => string.Join(",",(object[])x)));
      CheckParseOK(parser, "ab", "a,b");
      CheckParseOK(parser, "AB", "A,B");
      CheckParseFail(parser, "XX");
    }
    [Test]
    public void TestAttrBool()
    {
      Parser parser = Hammer.AttrBool(Hammer.Many1(Hammer.Choice(Hammer.Ch('a'),
                                                                 Hammer.Ch('b'))),
                                      (HPredicate)(x => { 
                                          object[] elems = (object[])x;
                                          return elems.Length > 1 && (char)elems[0] == (char)elems[1];
                                        }));
      
      CheckParseOK(parser, "aa", new object[]{ 'a','a' });
      CheckParseOK(parser, "bb", new object[]{ 'b','b' });
      CheckParseFail(parser, "ab");
                                                                  
    }
  }
}