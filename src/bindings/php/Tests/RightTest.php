<?php
include_once 'hammer.php';

class RightTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_right(hammer_ch(" "), hammer_ch("a"));
    }
    public function testSuccess()
    {
        $result = hammer_parse($this->parser, " a");
        $this->assertEquals("a", $result);
    }
    public function testFailure()
    {
        $result1 = hammer_parse($this->parser, "a");
        $result2 = hammer_parse($this->parser, " ");
        $result3 = hammer_parse($this->parser, "ba");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
    }
}
?>