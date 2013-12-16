<?php

include_once 'hammer.php';

class ChRangeTest extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = ch_range("a", "c");
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "b");
        $this->assertEquals("b", $result);
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "d");
        $this->assertEquals(NULL, $result);
    }
}
?>