<?php
include_once 'hammer.php';

class SepBy1Test extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_sep_by1(hammer_choice(hammer_ch("1"), hammer_ch("2"), hammer_ch("3")), hammer_ch(","));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "1,2,3");
        $result2 = hammer_parse($this->parser, "1,3,2");
        $result3 = hammer_parse($this->parser, "1,3");
        $result4 = hammer_parse($this->parser, "3");
        $this->assertEquals(array("1", "2", "3"), $result1);
        $this->assertEquals(array("1", "3", "2"), $result2);
        $this->assertEquals(array("1", "3"), $result3);
        $this->assertEquals(array("3"), $result4);
    }

    public function testFailure()
    {
        $result = hammer_parse($this->parser, "");
        $this->assertEquals(NULL, $result);
    }
}
?>