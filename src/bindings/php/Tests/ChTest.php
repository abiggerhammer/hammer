<?php

include_once 'hammer.php';

class ChTest extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer_ch("\xa2");
    }
    public function testSuccess() 
    {
        $result = hammer_parse($this->parser, "\xa2");
        $this->assertEquals("\xa2", $result);
    }     
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "95");
        $this->assertEquals(NULL, $result);
    }
}
?>