<?php
include_once 'hammer.php';

class RightTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_right(h_ch(" "), h_ch("a"));
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, " a");
        // TODO fix these tests when h_ch is fixed
        $this->assertEquals(97, $result);
    }
    public function testFailure()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, " ");
        $result3 = h_parse($this->parser, "ba");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
    }
}
?>