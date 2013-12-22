<?php
include_once 'hammer.php';

class IgnoreTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_sequence(hammer_ch("a"), hammer_ignore(hammer_ch("b")), hammer_ch("c"));
    }

    public function testSuccess()
    {
        $result = hammer_parse($this->parser, "abc");
        $this->assertEquals(array("a", "c"), $result);
    }

    public function testFailure()
    {
        $result = hammer_parse($this->parser, "ac");
        $this->assertEquals(NULL, $result);
    }
}
?>