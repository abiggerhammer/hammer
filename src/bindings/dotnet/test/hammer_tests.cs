namespace Hammer.Test {
    using NUnit.Framework;
    [TestFixture]
    public partial class HammerTest {
        [Test]
        public void TestToken() {
            Parser parser;
            parser = Hammer.Token("95\xa2");
              CheckParseOK(parser, "95\xa2", new byte[]{ 0x39, 0x35, 0xa2});
              CheckParseFail(parser, "95\xa2");
        }
        [Test]
        public void TestCh() {
            Parser parser;
            parser = Hammer.Ch(0xa2);
              CheckParseOK(parser, "\xa2", '\xa2');
              CheckParseFail(parser, "\xa3");
        }
        [Test]
        public void TestCh_range() {
            Parser parser;
            parser = Hammer.Ch_range(0x61, 0x63);
              CheckParseOK(parser, "b", 'b');
              CheckParseFail(parser, "d");
        }
        [Test]
        public void TestInt64() {
            Parser parser;
            parser = Hammer.Int64();
              CheckParseOK(parser, "\xff\xff\xff\xfe\x00\x00\x00\x00", (System.Int64)(-0x200000000));
              CheckParseFail(parser, "\xff\xff\xff\xfe\x00\x00\x00");
        }
        [Test]
        public void TestInt32() {
            Parser parser;
            parser = Hammer.Int32();
              CheckParseOK(parser, "\xff\xfe\x00\x00", (System.Int64)(-0x20000));
              CheckParseFail(parser, "\xff\xfe\x00");
              CheckParseOK(parser, "\x00\x02\x00\x00", (System.Int64)(0x20000));
              CheckParseFail(parser, "\x00\x02\x00");
        }
        [Test]
        public void TestInt16() {
            Parser parser;
            parser = Hammer.Int16();
              CheckParseOK(parser, "\xfe\x00", (System.Int64)(-0x200));
              CheckParseFail(parser, "\xfe");
              CheckParseOK(parser, "\x02\x00", (System.Int64)(0x200));
              CheckParseFail(parser, "\x02");
        }
        [Test]
        public void TestInt8() {
            Parser parser;
            parser = Hammer.Int8();
              CheckParseOK(parser, "\x88", (System.Int64)(-0x78));
              CheckParseFail(parser, "");
        }
        [Test]
        public void TestUint64() {
            Parser parser;
            parser = Hammer.Uint64();
              CheckParseOK(parser, "\x00\x00\x00\x02\x00\x00\x00\x00", (System.UInt64)0x200000000);
              CheckParseFail(parser, "\x00\x00\x00\x02\x00\x00\x00");
        }
        [Test]
        public void TestUint32() {
            Parser parser;
            parser = Hammer.Uint32();
              CheckParseOK(parser, "\x00\x02\x00\x00", (System.UInt64)0x20000);
              CheckParseFail(parser, "\x00\x02\x00");
        }
        [Test]
        public void TestUint16() {
            Parser parser;
            parser = Hammer.Uint16();
              CheckParseOK(parser, "\x02\x00", (System.UInt64)0x200);
              CheckParseFail(parser, "\x02");
        }
        [Test]
        public void TestUint8() {
            Parser parser;
            parser = Hammer.Uint8();
              CheckParseOK(parser, "x", (System.UInt64)0x78);
              CheckParseFail(parser, "");
        }
        [Test]
        public void TestInt_range() {
            Parser parser;
            parser = Hammer.Int_range(Hammer.Uint8(), 0x3, 0x10);
              CheckParseOK(parser, "\x05", (System.UInt64)0x5);
              CheckParseFail(parser, "\x0b");
        }
        [Test]
        public void TestWhitespace() {
            Parser parser;
            parser = Hammer.Whitespace(Hammer.Ch(0x61));
              CheckParseOK(parser, "a", 'a');
              CheckParseOK(parser, " a", 'a');
              CheckParseOK(parser, "  a", 'a');
              CheckParseOK(parser, "\x09a", 'a');
              CheckParseFail(parser, "_a");
            parser = Hammer.Whitespace(Hammer.End_p());
              CheckParseOK(parser, "", null);
              CheckParseOK(parser, "  ", null);
              CheckParseFail(parser, "  x");
        }
        [Test]
        public void TestLeft() {
            Parser parser;
            parser = Hammer.Left(Hammer.Ch(0x61), Hammer.Ch(0x20));
              CheckParseOK(parser, "a ", 'a');
              CheckParseFail(parser, "a");
              CheckParseFail(parser, " ");
              CheckParseFail(parser, "ba");
        }
        [Test]
        public void TestMiddle() {
            Parser parser;
            parser = Hammer.Middle(Hammer.Ch(' '), Hammer.Ch('a'), Hammer.Ch(' '));
              CheckParseOK(parser, " a ", 'a');
              CheckParseFail(parser, "a");
              CheckParseFail(parser, " a");
              CheckParseFail(parser, "a ");
              CheckParseFail(parser, " b ");
              CheckParseFail(parser, "ba ");
              CheckParseFail(parser, " ab");
        }
        [Test]
        public void TestIn() {
            Parser parser;
            parser = Hammer.In("abc");
              CheckParseOK(parser, "b", 'b');
              CheckParseFail(parser, "d");
        }
        [Test]
        public void TestNot_in() {
            Parser parser;
            parser = Hammer.Not_in("abc");
              CheckParseOK(parser, "d", 'd');
              CheckParseFail(parser, "a");
        }
        [Test]
        public void TestEnd_p() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.End_p());
              CheckParseOK(parser, "a", new object[]{ 'a'});
              CheckParseFail(parser, "aa");
        }
        [Test]
        public void TestNothing_p() {
            Parser parser;
            parser = Hammer.Nothing_p();
              CheckParseFail(parser, "a");
        }
        [Test]
        public void TestSequence() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Ch('b'));
              CheckParseOK(parser, "ab", new object[]{ 'a', 'b'});
              CheckParseFail(parser, "a");
              CheckParseFail(parser, "b");
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Whitespace(Hammer.Ch('b')));
              CheckParseOK(parser, "ab", new object[]{ 'a', 'b'});
              CheckParseOK(parser, "a b", new object[]{ 'a', 'b'});
              CheckParseOK(parser, "a  b", new object[]{ 'a', 'b'});
        }
        [Test]
        public void TestChoice() {
            Parser parser;
            parser = Hammer.Choice(Hammer.Ch('a'), Hammer.Ch('b'));
              CheckParseOK(parser, "a", 'a');
              CheckParseOK(parser, "b", 'b');
              CheckParseOK(parser, "ab", 'a');
              CheckParseFail(parser, "c");
        }
        [Test]
        public void TestButnot() {
            Parser parser;
            parser = Hammer.Butnot(Hammer.Ch('a'), Hammer.Token("ab"));
              CheckParseOK(parser, "a", 'a');
              CheckParseFail(parser, "ab");
              CheckParseOK(parser, "aa", 'a');
            parser = Hammer.Butnot(Hammer.Ch_range('0', '9'), Hammer.Ch('6'));
              CheckParseOK(parser, "5", '5');
              CheckParseFail(parser, "6");
        }
        [Test]
        public void TestDifference() {
            Parser parser;
            parser = Hammer.Difference(Hammer.Token("ab"), Hammer.Ch('a'));
              CheckParseOK(parser, "ab", new byte[]{ 0x61, 0x62});
              CheckParseFail(parser, "a");
        }
        [Test]
        public void TestXor() {
            Parser parser;
            parser = Hammer.Xor(Hammer.Ch_range('0', '6'), Hammer.Ch_range('5', '9'));
              CheckParseOK(parser, "0", '0');
              CheckParseOK(parser, "9", '9');
              CheckParseFail(parser, "5");
              CheckParseFail(parser, "a");
        }
        [Test]
        public void TestMany() {
            Parser parser;
            parser = Hammer.Many(Hammer.Choice(Hammer.Ch('a'), Hammer.Ch('b')));
              CheckParseOK(parser, "", new object[]{ });
              CheckParseOK(parser, "a", new object[]{ 'a'});
              CheckParseOK(parser, "b", new object[]{ 'b'});
              CheckParseOK(parser, "aabbaba", new object[]{ 'a', 'a', 'b', 'b', 'a', 'b', 'a'});
        }
        [Test]
        public void TestMany1() {
            Parser parser;
            parser = Hammer.Many1(Hammer.Choice(Hammer.Ch('a'), Hammer.Ch('b')));
              CheckParseFail(parser, "");
              CheckParseOK(parser, "a", new object[]{ 'a'});
              CheckParseOK(parser, "b", new object[]{ 'b'});
              CheckParseOK(parser, "aabbaba", new object[]{ 'a', 'a', 'b', 'b', 'a', 'b', 'a'});
              CheckParseFail(parser, "daabbabadef");
        }
        [Test]
        public void TestRepeat_n() {
            Parser parser;
            parser = Hammer.Repeat_n(Hammer.Choice(Hammer.Ch('a'), Hammer.Ch('b')), 0x2);
              CheckParseFail(parser, "adef");
              CheckParseOK(parser, "abdef", new object[]{ 'a', 'b'});
              CheckParseFail(parser, "dabdef");
        }
        [Test]
        public void TestOptional() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Optional(Hammer.Choice(Hammer.Ch('b'), Hammer.Ch('c'))), Hammer.Ch('d'));
              CheckParseOK(parser, "abd", new object[]{ 'a', 'b', 'd'});
              CheckParseOK(parser, "acd", new object[]{ 'a', 'c', 'd'});
              CheckParseOK(parser, "ad", new object[]{ 'a', null, 'd'});
              CheckParseFail(parser, "aed");
              CheckParseFail(parser, "ab");
              CheckParseFail(parser, "ac");
        }
        [Test]
        public void TestIgnore() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Ignore(Hammer.Ch('b')), Hammer.Ch('c'));
              CheckParseOK(parser, "abc", new object[]{ 'a', 'c'});
              CheckParseFail(parser, "ac");
        }
        [Test]
        public void TestSepBy() {
            Parser parser;
            parser = Hammer.SepBy(Hammer.Choice(Hammer.Ch('1'), Hammer.Ch('2'), Hammer.Ch('3')), Hammer.Ch(','));
              CheckParseOK(parser, "1,2,3", new object[]{ '1', '2', '3'});
              CheckParseOK(parser, "1,3,2", new object[]{ '1', '3', '2'});
              CheckParseOK(parser, "1,3", new object[]{ '1', '3'});
              CheckParseOK(parser, "3", new object[]{ '3'});
              CheckParseOK(parser, "", new object[]{ });
        }
        [Test]
        public void TestSepBy1() {
            Parser parser;
            parser = Hammer.SepBy1(Hammer.Choice(Hammer.Ch('1'), Hammer.Ch('2'), Hammer.Ch('3')), Hammer.Ch(','));
              CheckParseOK(parser, "1,2,3", new object[]{ '1', '2', '3'});
              CheckParseOK(parser, "1,3,2", new object[]{ '1', '3', '2'});
              CheckParseOK(parser, "1,3", new object[]{ '1', '3'});
              CheckParseOK(parser, "3", new object[]{ '3'});
              CheckParseFail(parser, "");
        }
        [Test]
        public void TestAnd() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.And(Hammer.Ch('0')), Hammer.Ch('0'));
              CheckParseOK(parser, "0", new object[]{ '0'});
              CheckParseFail(parser, "1");
            parser = Hammer.Sequence(Hammer.And(Hammer.Ch('0')), Hammer.Ch('1'));
              CheckParseFail(parser, "0");
              CheckParseFail(parser, "1");
            parser = Hammer.Sequence(Hammer.Ch('1'), Hammer.And(Hammer.Ch('2')));
              CheckParseOK(parser, "12", new object[]{ '1'});
              CheckParseFail(parser, "13");
        }
        [Test]
        public void TestNot() {
            Parser parser;
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Choice(Hammer.Token("+"), Hammer.Token("++")), Hammer.Ch('b'));
              CheckParseOK(parser, "a+b", new object[]{ 'a', new byte[]{ 0x2b}, 'b'});
              CheckParseFail(parser, "a++b");
            parser = Hammer.Sequence(Hammer.Ch('a'), Hammer.Choice(Hammer.Sequence(Hammer.Token("+"), Hammer.Not(Hammer.Ch('+'))), Hammer.Token("++")), Hammer.Ch('b'));
              CheckParseOK(parser, "a+b", new object[]{ 'a', new object[]{ new byte[]{ 0x2b}}, 'b'});
              CheckParseOK(parser, "a++b", new object[]{ 'a', new byte[]{ 0x2b, 0x2b}, 'b'});
        }
        [Test]
        public void TestRightrec() {
            Parser parser;
            IndirectParser sp_rr = Hammer.Indirect();
            sp_rr.Bind(Hammer.Choice(Hammer.Sequence(Hammer.Ch('a'), sp_rr), Hammer.Epsilon_p()));
            parser = sp_rr;
              CheckParseOK(parser, "a", new object[]{ 'a'});
              CheckParseOK(parser, "aa", new object[]{ 'a', new object[]{ 'a'}});
              CheckParseOK(parser, "aaa", new object[]{ 'a', new object[]{ 'a', new object[]{ 'a'}}});
        }
        [Test]
        public void TestAmbiguous() {
            Parser parser;
            IndirectParser sp_d = Hammer.Indirect();
            IndirectParser sp_p = Hammer.Indirect();
            IndirectParser sp_e = Hammer.Indirect();
            sp_d.Bind(Hammer.Ch('d'));
            sp_p.Bind(Hammer.Ch('+'));
            sp_e.Bind(Hammer.Choice(Hammer.Sequence(sp_e, sp_p, sp_e), sp_d));
            parser = sp_e;
              CheckParseOK(parser, "d", 'd');
              CheckParseOK(parser, "d+d", new object[]{ 'd', '+', 'd'});
              CheckParseOK(parser, "d+d+d", new object[]{ new object[]{ 'd', '+', 'd'}, '+', 'd'});
        }
    }
}
