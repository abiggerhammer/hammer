<?php
include_once 'hammer.php';

class IgnoreTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = sequence(ch("a"), h_ignore(ch("b")), ch("c"));
    }

    public function testSuccess()
    {
        $result = h_parse($this->parser, "abc");
        $this->assertEquals(array("a", "c"), $result);
    }

    public function testFailure()
    {
        $result = h_parse($this->parser, "ac");
        $this->assertEquals(NULL, $result);
    }
}
?>