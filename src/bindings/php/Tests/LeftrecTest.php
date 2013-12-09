<?php
include_once 'hammer.php';

class LeftrecTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_indirect();
        h_bind_indirect($this->parser, choice(sequence($this->parser, ch("a")), ch("a")));
    }

    public function testSuccess1()
    {
        $result = h_parse($this->parser, "a");
        $this->assertEquals("a", $result);
    }

    public function testSuccess2()
    {
        $result = h_parse($this->parser, "aa");
        var_dump($result);
        $this->assertEquals(array("a", "a"), $result);
    }

    public function testSuccess3()
    {
        $result = h_parse($this->parser, "aaa");
        var_dump($result);
        $this->assertEquals(array(array("a", "a"), "a"), $result);
    }
}
?>