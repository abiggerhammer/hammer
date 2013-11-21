<?php

include 'hammer.php';

class TokenTest extends PHPUnit_Framework_TestCase 
{
    protected $parser;

    protected function setUp() 
    {
        $this->parser = h_token("95\xa2");
    }
    public function testSuccess() 
    {
        $result = h_parse($this->parser, "95\xa2");
        //var_dump($result);
        $ast = hparseresult_ast_get($result);
        //var_dump($ast);
        $token_data = hparsedtoken_token_data_get($ast);
        //var_dump($token_data);
        $bytes = htokendata_bytes_get($token_data);
        //var_dump($bytes);
        $this->assertEquals($bytes, "95\xa2");
    }     
    public function testFailure()
    {
        $result = h_parse($this->parser, "95");
        $this->assertEquals($result, NULL);
    }
}
?>