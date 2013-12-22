<?php

include_once 'hammer.php';

class ChRangeTest extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_ch_range("a", "c");
    }
    public function testSuccess() 
    {
        $result = hammer_parse($this->parser, "b");
        $this->assertEquals("b", $result);
    }     
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "d");
        $this->assertEquals(NULL, $result);
    }
}
?>