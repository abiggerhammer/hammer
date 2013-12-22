<?php
include_once 'hammer.php';

class RightrecTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_indirect();
        hammer_bind_indirect($this->parser, hammer_choice(hammer_sequence(hammer_ch("a"), $this->parser), hammer_epsilon()));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "a");
        $result2 = hammer_parse($this->parser, "aa");
        $result3 = hammer_parse($this->parser, "aaa");
        $this->assertEquals(array("a"), $result1);
        $this->assertEquals(array("a", array("a")), $result2);
        $this->assertEquals(array("a", array("a", array("a"))), $result3);
    }
}
?>