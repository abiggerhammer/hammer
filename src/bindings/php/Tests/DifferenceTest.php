<?php
include_once 'hammer.php';

class DifferenceTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_difference(hammer_token("ab"), hammer_ch("a"));
    }

    public function testSuccess()
    {
        $result = hammer_parse($this->parser, "ab");
        $this->assertEquals("ab", $result);
    }

    public function testFailure()
    {
        $result = hammer_parse($this->parser, "a");
        $this->assertEquals(NULL, $result);
    }
}
?>