<?php

include_once 'hammer.php';

class Int32Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_int32();
    }
    public function testNegative() 
    {
        $result = hammer_parse($this->parser, "\xff\xfe\x00\x00");
        $this->assertEquals(-0x20000, $result);
    }     
    public function testPositive() 
    {
        $result = hammer_parse($this->parser, "\x00\x02\x00\x00");
        $this->assertEquals(0x20000, $result);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "\xff\xfe\x00");
        $this->assertEquals(NULL, $result);
        $result = hammer_parse($this->parser, "\x00\x02\x00");
        $this->assertEquals(NULL, $result);
    }
}
?>