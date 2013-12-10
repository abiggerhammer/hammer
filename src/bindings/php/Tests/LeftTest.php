<?php
include_once 'hammer.php';

class LeftTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_left(ch("a"), ch(" "));
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, "a ");
        $this->assertEquals("a", $result);
    }
    public function testFailure()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, " ");
        $result3 = h_parse($this->parser, "ab");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
    }
}
?>