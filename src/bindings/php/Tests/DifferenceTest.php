<?php
include_once 'hammer.php';

class DifferenceTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_difference(h_token("ab"), ch("a"));
    }

    public function testSuccess()
    {
        $result = h_parse($this->parser, "ab");
        $this->assertEquals("ab", $result);
    }

    public function testFailure()
    {
        $result = h_parse($this->parser, "a");
        $this->assertEquals(NULL, $result);
    }
}
?>