<?php
include_once 'hammer.php';

class RepeatNTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_repeat_n(hammer_choice(hammer_ch("a"), hammer_ch("b")), 2);
    }

    public function testSuccess()
    {
        $result = hammer_parse($this->parser, "abdef");
        $this->assertEquals(array("a", "b"), $result);
    }

    public function testFailure()
    {
        $result1 = hammer_parse($this->parser, "adef");
        $result2 = hammer_parse($this->parser, "dabdef");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
}
?>