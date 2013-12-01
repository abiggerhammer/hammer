<?php
include_once 'hammer.php';

class EndPTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_sequence(h_ch("a"), h_end_p());
    }
    public function testSuccess()
    {
        $result = h_parse($this->parser, "a");
        // TODO: fixme when h_ch is fixed
        $this->assertEquals(98, $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "aa");
        $this->assertEquals(NULL, $result);
    }
}
?>