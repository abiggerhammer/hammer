<?php
include_once 'hammer.php';

class NothingPTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_nothing();
    }

    public function testFailure()
    {
        $result = hammer_parse($this->parser, "a");
        $this->assertEquals(NULL, $result);
    }
}
?>