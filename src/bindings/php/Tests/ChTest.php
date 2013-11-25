<?php

include_once 'hammer.php';

class ChTest extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_ch("\xa2");
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "\xa2");
        $this->assertEquals("\xa2", $result);
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "95");
        $this->assertEquals(NULL, $result);
    }
}
?>