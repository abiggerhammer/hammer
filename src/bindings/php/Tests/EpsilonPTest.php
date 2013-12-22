<?php
include_once 'hammer.php';

class EpsilonPTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    protected $parser3;

    protected function setUp()
    {
        $this->parser1 = hammer_sequence(hammer_ch("a"), hammer_epsilon(), hammer_ch("b"));
        $this->parser2 = hammer_sequence(hammer_epsilon(), hammer_ch("a"));
        $this->parser3 = hammer_sequence(hammer_ch("a"), hammer_epsilon());
    }

    public function testSuccess1()
    {
        $result = hammer_parse($this->parser1, "ab");
        $this->assertEquals(array("a", "b"), $result);
    }

    public function testSuccess2()
    {
        $result = hammer_parse($this->parser2, "a");
        $this->assertEquals(array("a"), $result);
    }

    public function testSuccess3()
    {
        $result = hammer_parse($this->parser3, "ab");
        $this->assertEquals(array("a"), $result);
    }
}
?>