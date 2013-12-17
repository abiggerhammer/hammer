<?php
include_once 'hammer.php';

class InTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_in("abc");
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