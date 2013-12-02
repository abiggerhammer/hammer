<?php
include_once 'hammer.php';

class NothingPTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_nothing_p();
    }

    public function testFailure()
    {
        $result = h_parse($this->parser, "a");
        $this->assertEquals(NULL, $result);
    }
}
?>