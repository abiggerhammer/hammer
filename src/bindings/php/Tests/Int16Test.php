<?php

include_once 'hammer.php';

class Int16Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_int16();
    }
    public function testNegative() 
    {
        $result = hammer_parse($this->parser, "\xfe\x00");
        $this->assertEquals(-0x200, $result);
    }     
    public function testPositive() {
        $result = hammer_parse($this->parser, "\x02\x00");
        $this->assertEquals(0x200, $result);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "\xfe");
        $this->assertEquals(NULL, $result);
        $result = hammer_parse($this->parser, "\x02");
        $this->assertEquals(NULL, $result);
    }
}
?>