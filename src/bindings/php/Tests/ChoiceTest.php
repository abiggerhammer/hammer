<?php
include_once 'hammer.php';

class ChoiceTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = choice(ch("a"), ch("b"));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, "b");
        $this->assertEquals("a", $result1);
        $this->assertEquals("b", $result2);
    }

    public function testFailure()
    {
        $result = h_parse($this->parser, "c");
        $this->assertEquals(NULL, $result);
    }
}
?>