<?php
include_once 'hammer.php';

class Many1Test extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_many1(hammer_choice(hammer_ch("a"), hammer_ch("b")));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "a");
        $result2 = hammer_parse($this->parser, "b");
        $result3 = hammer_parse($this->parser, "aabbaba");
        $this->assertEquals(array("a"), $result1);
        $this->assertEquals(array("b"), $result2);
        $this->assertEquals(array("a", "a", "b", "b", "a", "b", "a"), $result3);
    }

    public function testFailure()
    {
        $result1 = hammer_parse($this->parser, "");
        $result2 = hammer_parse($this->parser, "daabbabadef");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
}
?>