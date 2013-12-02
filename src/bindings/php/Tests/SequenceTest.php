<?php
include_once 'hammer.php';

class SequenceTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;

    protected function setUp()
    {
        $this->parser1 = sequence(ch("a"), ch("b"));
        $this->parser2 = sequence(ch("a"), h_whitespace(ch("b")));
    }

    public function testSuccess1()
    {
        $result = h_parse($this->parser1, "ab");
        $this->assertEquals(array("a", "b"), $result);
    }
    
    public function testFailure1()
    {
        $result1 = h_parse($this->parser1, "a");
        $result2 = h_parse($this->parser2, "b");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
    }

    public function testSuccess2()
    {
        $result1 = h_parse($this->parser2, "ab");
        $result2 = h_parse($this->parser2, "a b");
        $result3 = h_parse($this->parser2, "a  b");
        $this->assertEquals(array("a", "b"), $result1);
        $this->assertEquals(array("a", "b"), $result2);
        $this->assertEquals(array("a", "b"), $result3);
    }
}
?>