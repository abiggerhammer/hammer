<?php

include_once 'hammer.php';

class Uint64Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_uint64();
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "\x00\x00\x00\x02\x00\x00\x00\x00");
        $this->assertEquals(0x200000000, $result);
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "\x00\x00\x00\x02\x00\x00\x00");
        $this->assertEquals(NULL, $result);
    }
}
?>