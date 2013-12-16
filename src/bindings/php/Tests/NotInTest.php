<?php
include_once 'hammer.php';

class NotInTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = not_in("abc");
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, "d");
        $this->assertEquals("d", $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "b");
        $this->assertEquals(NULL, $result);
    }
}
?>