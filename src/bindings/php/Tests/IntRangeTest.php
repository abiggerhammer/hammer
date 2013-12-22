<?php
include_once 'hammer.php';

class IntRangeTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_int_range(hammer_uint8(), 3, 10);
    }
    public function testSuccess()
    {
        $result = hammer_parse($this->parser, "\x05");
        $this->assertEquals(5, $result);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "\xb");
        $this->assertEquals(NULL, $result);
    }
}
?>