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
        $this->assertEquals(array(), $result);
        $this->assertEquals(array("a"), $result);
        $this->assertEquals(array("b"), $result);
        $this->assertEquals(array("a", "a", "b", "b", "a", "b", "a"), $result);
    }
}
?>