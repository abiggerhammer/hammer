<?php
include_once 'hammer.php';

class ManyTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_many(choice(ch("a"), ch("b")));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "");
        $result2 = h_parse($this->parser, "a");
        $result3 = h_parse($this->parser, "b");
        $result4 = h_parse($this->parser, "aabbaba");
        $this->assertEquals(array(), $result1);
        $this->assertEquals(array("a"), $result2);
        $this->assertEquals(array("b"), $result3);
        $this->assertEquals(array("a", "a", "b", "b", "a", "b", "a"), $result4);
    }
}
?>