<?php

include_once 'hammer.php';

class Int64Test extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_int64();
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "\xff\xff\xff\xfe\x00\x00\x00\x00");
        $this->assertEquals(-0x200000000, $result);
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "\xff\xff\xff\xfe\x00\x00\x00");
        $this->assertEquals(NULL, $result);
    }
}
?>