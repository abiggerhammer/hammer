<?php
include_once 'hammer.php';

class LeftrecTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_indirect();
        h_bind_indirect($this->parser, choice(sequence($this->parser, ch("a")), ch("a")));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, "aa");
        $result3 = h_parse($this->parser, "aaa");
        $this->assertEquals("a", $result1);
        $this->assertEquals(array("a", "a"), $result);
        $this->assertEquals(array(array("a", "a"), "a"), $result);
    }
}
?>