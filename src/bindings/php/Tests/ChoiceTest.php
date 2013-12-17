<?php
include_once 'hammer.php';

class ChoiceTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_choice(hammer_ch("a"), hammer_ch("b"));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "a");
        $result2 = hammer_parse($this->parser, "b");
        $this->assertEquals("a", $result1);
        $this->assertEquals("b", $result2);
    }

    public function testFailure()
    {
        $result = hammer_parse($this->parser, "c");
        $this->assertEquals(NULL, $result);
    }
}
?>