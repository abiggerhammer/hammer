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
        $this->assertEquals($result->ast->token_data->bytes, "95\xa2");
    }     
    public function testFailure()
    {
        $result = hammer::h_parse($this->parser, "95", 2);
        $this->assertEquals($result, NULL);
    }
}
?>