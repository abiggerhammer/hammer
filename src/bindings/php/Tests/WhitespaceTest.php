<?php
include_once 'hammer.php';

class WhitespaceTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    
    protected function setUp()
    {
        $this->parser1 = hammer_whitespace(hammer_ch("a"));
        $this->parser2 = hammer_whitespace(hammer_end());
    }
    public function testSuccess1()
    {
        $result1 = hammer_parse($this->parser1, "a");
        $result2 = hammer_parse($this->parser1, " a");
        $result3 = hammer_parse($this->parser1, "  a");
        $result4 = hammer_parse($this->parser1, "\ta");
        $this->assertEquals("a", $result1);
        $this->assertEquals("a", $result2);
        $this->assertEquals("a", $result3);
        $this->assertEquals("a", $result4);
    }
    public function testFailure1()
    {
        $result = hammer_parse($this->parser1, "_a");
        $this->assertEquals(NULL, $result);
    }
    public function testSuccess2()
    {
        $result1 = hammer_parse($this->parser2, "");
        $result2 = hammer_parse($this->parser2, "  ");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }
    public function testFailure2()
    {
        $result = hammer_parse($this->parser2, "  x");
        $this->assertEquals(NULL, $result);
    }
}
?>