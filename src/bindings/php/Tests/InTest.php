<?php
include_once 'hammer.php';

class InTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = in("abc");
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