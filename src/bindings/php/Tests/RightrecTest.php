<?php
include_once 'hammer.php';

class RightrecTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = h_indirect();
        h_bind_indirect($this->parser, choice(sequence(ch("a"), $this->parser), h_epsilon_p()));
    }

    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "a");
        $result2 = h_parse($this->parser, "aa");
        $result3 = h_parse($this->parser, "aaa");
        $this->assertEquals(array("a"), $result1);
        $this->assertEquals(array("a", array("a")), $result2);
        $this->assertEquals(array("a", array("a", array("a"))), $result3);
    }
}
?>