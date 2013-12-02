<?php
include_once 'hammer.php';

class RepeatNTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_repeat_n(choice(ch("a"), ch("b")), 2);
    }

    public function testSuccess()
    {
        $result = h_parse($this->parser, "abdef");
        $this->assertEquals(array("a", "b"), $result);
    }

    public function testFailure()
    {
        $result1 = h_parse($this->parser, "adef");
        $result2 = h_parse($this->parser, "dabdef");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
}
?>