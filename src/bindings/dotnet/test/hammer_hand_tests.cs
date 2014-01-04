namespace Hammer.Test
{
  using NUnit.Framework;
  [TestFixture]
  public partial class HammerTest
  {
    [Test]
    public void TestAction()
    {
      Parser parser = Hammer.Action(Hammer.Sequence(Hammer.Choice(Hammer.Token("a"),
                                                                  Hammer.Token("A")),
                                                    Hammer.Choice(Hammer.Token("b"),
                                                                  Hammer.Token("B"))),
                                    (HAction)(x => char.ToUpper(((string)x)[0])));
                                                                  
    }
  }
}