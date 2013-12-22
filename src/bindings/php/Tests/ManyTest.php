<?php
include_once 'hammer.php';

class ManyTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_many(hammer_choice(hammer_ch("a"), hammer_ch("b")));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "");
        $result2 = hammer_parse($this->parser, "a");
        $result3 = hammer_parse($this->parser, "b");
        $result4 = hammer_parse($this->parser, "aabbaba");
        $this->assertEquals(array(), $result1);
        $this->assertEquals(array("a"), $result2);
        $this->assertEquals(array("b"), $result3);
        $this->assertEquals(array("a", "a", "b", "b", "a", "b", "a"), $result4);
    }
}
?>