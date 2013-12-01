<?php
include_once 'hammer.php';

class MiddleTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_middle(h_ch(" "), h_ch("a"), h_ch(" "));
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, " a ");
        // TODO fix these tests when h_ch is fixed
        $this->assertEquals(97, $result);
    }
    public function testFailure()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, " ");
        $result3 = h_parse($this->parser, " a");
        $result4 = h_parse($this->parser, "a ");
        $result5 = h_parse($this->parser, " b ");
        $result6 = h_parse($this->parser, "ba ");
        $result7 = h_parse($this->parser, " ab");
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