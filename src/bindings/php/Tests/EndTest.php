<?php
include_once 'hammer.php';

class EndPTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = sequence(ch("a"), h_end_p());
    }

    public function testSuccess()
    {
        $result = h_parse($this->parser, "a");
        $this->assertEquals("a", $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "aa");
        $this->assertEquals(NULL, $result);
    }
}
?>