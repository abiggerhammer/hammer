<?php

include_once 'hammer.php';

class Uint8Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_uint8();
    }
    public function testSuccess() 
    {
        $result = hammer_parse($this->parser, "\x78");
        $this->assertEquals(0x78, $result);
    }     
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "");
        $this->assertEquals(NULL, $result);
    }
}
?>