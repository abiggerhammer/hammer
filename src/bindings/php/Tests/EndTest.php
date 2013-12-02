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
        var_dump($result);
        $this->assertEquals(array("a"), $result);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "aa");
        var_dump($result);
        $this->assertEquals(NULL, $result);
    }
}
?>