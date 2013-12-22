<?php
include_once 'hammer.php';

class OptionalTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_sequence(hammer_ch("a"), hammer_optional(hammer_choice(hammer_ch("b"), hammer_ch("c"))), hammer_ch("d"));
    }

    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "abd");
        $result2 = hammer_parse($this->parser, "acd");
        $result3 = hammer_parse($this->parser, "ad");
        $this->assertEquals(array("a", "b", "d"), $result1);
        $this->assertEquals(array("a", "c", "d"), $result2);
        $this->assertEquals(array("a", NULL, "d"), $result3);
    }

    public function testFailure()
    {
        $result1 = hammer_parse($this->parser, "aed");
        $result2 = hammer_parse($this->parser, "ab");
        $result3 = hammer_parse($this->parser, "ac");
        $this->assertEquals(NULL, $result1);
        $this->assertEquals(NULL, $result2);
        $this->assertEquals(NULL, $result3);
    }
}
?>