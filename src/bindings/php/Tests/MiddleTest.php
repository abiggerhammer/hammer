<?php
include_once 'hammer.php';

class MiddleTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_middle(hammer_ch(" "), hammer_ch("a"), hammer_ch(" "));
    }
    public function testSuccess()
    {
        $result = hammer_parse($this->parser, " a ");
        $this->assertEquals("a", $result);
    }
    public function testFailure()
    {
        $result1 = hammer_parse($this->parser, "a");
        $result2 = hammer_parse($this->parser, " ");
        $result3 = hammer_parse($this->parser, " a");
        $result4 = hammer_parse($this->parser, "a ");
        $result5 = hammer_parse($this->parser, " b ");
        $result6 = hammer_parse($this->parser, "ba ");
        $result7 = hammer_parse($this->parser, " ab");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
        $this->assertEquals(NULL, $result4);
        $this->assertEquals(NULL, $result5);
        $this->assertEquals(NULL, $result6);
        $this->assertEquals(NULL, $result7);
    }
}
?>