<?php
include_once 'hammer.php';

class WhitespaceTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    
    protected function setUp()
    {
        $this->parser1 = h_whitespace(h_ch("a"));
        $this->parser2 = h_whitespace(h_end_p());
    }
    public function testSuccess1()
    {
        $result1 = h_parse($this->parser1, "a");
        $result2 = h_parse($this->parser1, " a");
        $result3 = h_parse($this->parser1, "  a");
        $result4 = h_parse($this->parser1, "\ta");
        // TODO fix these tests when h_ch is fixed
        $this->assertEquals(97, $result1);
        $this->assertEquals(97, $result2);
        $this->assertEquals(97, $result3);
        $this->assertEquals(97, $result4);
    }
    public function testFailure1()
    {
        $result = h_parse($this->parser1, "_a");
        $this->assertEquals(NULL, $result);
    }
    public function testSuccess2()
    {
        $result1 = h_parse($this->parser2, "");
        $result2 = h_parse($this->parser2, "  ");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
    public function testFailure2()
    {
        $result = h_parse($this->parser2, "  x");
        $this->assertEquals(NULL, $result);
    }
}
?>