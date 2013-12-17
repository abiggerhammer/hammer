<?php

include_once 'hammer.php';

class Uint64Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_uint64();
    }
    public function testSuccess() 
    {
        $result = hammer_parse($this->parser, "\x00\x00\x00\x02\x00\x00\x00\x00");
        $this->assertEquals(0x200000000, $result);
    }     
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "\x00\x00\x00\x02\x00\x00\x00");
        $this->assertEquals(NULL, $result);
    }
}
?>