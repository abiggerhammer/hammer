<?php
include_once 'hammer.php';

class EndPTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_sequence(hammer_ch("a"), hammer_end());
    }

    public function testSuccess()
    {
        $result = hammer_parse($this->parser, "a");
        $this->assertEquals(array("a"), $result);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "aa");
        $this->assertEquals(NULL, $result);
    }
}
?>