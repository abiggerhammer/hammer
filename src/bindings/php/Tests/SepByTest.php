<?php
include_once 'hammer.php';

class SepByTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_sepBy(choice(ch("1"), ch("2"), ch("3")), ch(","));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "1,2,3");
        $result2 = h_parse($this->parser, "1,3,2");
        $result3 = h_parse($this->parser, "1,3");
        $result4 = h_parse($this->parser, "3");
        $result5 = h_parse($this->parser, "");
        $this->assertEquals(array("1", "2", "3"), $result1);
        $this->assertEquals(array("1", "3", "2"), $result2);
        $this->assertEquals(array("1", "3"), $result3);
        $this->assertEquals(array("3"), $result4);
        $this->assertEquals(array(), $result5);
    }
}
?>