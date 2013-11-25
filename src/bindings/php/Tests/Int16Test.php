<?php

include_once 'hammer.php';

class Int16Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_int16();
    }
    public function testNegative() 
    {
        $result = h_parse($this->parser, "\xfe\x00");
        $this->assertEquals(-0x200, $result);
    }     
    public function testPositive() {
        $result = h_parse($this->parser, "\x02\x00");
        $this->assertEquals(0x200, $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "\xfe");
        $this->assertEquals(NULL, $result);
        $result = h_parse($this->parser, "\x02");
        $this->assertEquals(NULL, $result);
    }
}
?>