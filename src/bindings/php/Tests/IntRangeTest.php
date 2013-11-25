<?php
include_once 'hammer.php';

class IntRangeTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_int_range(h_uint8(), 3, 10);
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, "\x05");
        $this->assertEquals(5, $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "\xb");
        $this->assertEquals(NULL, $result);
    }
}
?>