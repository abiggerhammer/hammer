import unittest
import hammer as h

class TestTokenParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.token("95\xa2")
    def test_success(self):
        self.assertEqual(self.parser.parse("95\xa2"), "95\xa2")
    def test_partial_fails(self):
        self.assertEqual(self.parser.parse("95"), None)

class TestChParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser_int = h.ch(0xa2)
        cls.parser_chr = h.ch("\xa2")
    def test_success(self):
        self.assertEqual(self.parser_int.parse("\xa2"), 0xa2)
        self.assertEqual(self.parser_chr.parse("\xa2"), "\xa2")
    def test_failure(self):
        self.assertEqual(self.parser_int.parse("\xa3"), None)
        self.assertEqual(self.parser_chr.parse("\xa3"), None)

class TestChRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = 
        
