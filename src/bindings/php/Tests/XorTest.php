<?php
include_once 'hammer.php';

class XorTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_xor(ch_range("0", "6"), ch_range("5", "9"));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "0");
        $result2 = h_parse($this->parser, "9");
        $this->assertEquals("0", $result1);
        $this->assertEquals("9", $result2);
    }

    public function testFailure()
    {
        $result1 = h_parse($this->parser, "5");
        $result2 = h_parse($this->parser, "a");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
}
?>