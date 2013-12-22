<?php
include_once 'hammer.php';

class LeftrecTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_indirect();
        hammer_bind_indirect($this->parser, hammer_choice(hammer_sequence($this->parser, hammer_ch("a")), hammer_ch("a")));
    }

    public function testSuccess1()
    {
        $result = hammer_parse($this->parser, "a");
        $this->assertEquals("a", $result);
    }
    /* These don't work in the underlying C so they won't work here either */
/*
    public function testSuccess2()
    {
        $result = hammer_parse($this->parser, "aa");
        var_dump($result);
        $this->assertEquals(array("a", "a"), $result);
    }

    public function testSuccess3()
    {
        $result = hammer_parse($this->parser, "aaa");
        var_dump($result);
        $this->assertEquals(array(array("a", "a"), "a"), $result);
    }
*/
}
?>