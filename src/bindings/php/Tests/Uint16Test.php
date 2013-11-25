<?php

include_once 'hammer.php';

class Uint16Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_uint16();
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "\x02\x00");
        $this->assertEquals(0x200, $result);
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "\x02");
        $this->assertEquals(NULL, $result);
    }
}
?>