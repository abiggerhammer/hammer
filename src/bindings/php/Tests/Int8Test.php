<?php

include_once 'hammer.php';

class Int8Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_int8();
    }
    public function testSuccess() 
    {
        $result = hammer_parse($this->parser, "\x88");
        $this->assertEquals(-0x78, $result);
    }     
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "");
        $this->assertEquals(NULL, $result);
    }
}
?>