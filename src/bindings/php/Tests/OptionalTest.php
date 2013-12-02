<?php
include_once 'hammer.php';

class OptionalTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = sequence(ch("a"), h_optional(choice(ch("b"), ch("c"))), ch("d"));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "abd");
        $result2 = h_parse($this->parser, "acd");
        $result3 = h_parse($this->parser, "ad");
        $this->assertEquals(array("a", "b", "d"), $result1);
        $this->assertEquals(array("a", "c", "d"), $result2);
        $this->assertEquals(array("a", NULL, "d"), $result3);
    }

    public function testFailure()
    {
        $result1 = h_parse($this->parser, "aed");
        $result2 = h_parse($this->parser, "ab");
        $result3 = h_parse($this->parser, "ac");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
    }
}
?>