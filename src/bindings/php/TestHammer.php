<?php

require("hammer.php");

class TestHammer extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = hammer::h_token("95\xa2", 3);
    }
    public function testSuccess() 
    {
        $result = hammer::h_parse($this->parser, "95\xa2", 3);
        var_dump($result);
        $this->assertEquals($result->__get("ast")->__get("token_data")->__get("bytes"), "95\xa2");
    }     
    public function testFailure()
    {
        $result = hammer::h_parse($this->parser, "95", 2);
        $this->assertEquals($result, NULL);
    }
}
?>