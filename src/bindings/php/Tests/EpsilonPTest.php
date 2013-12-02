<?php
include_once 'hammer.php';

class EpsilonPTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    protected $parser3;

    protected function setUp()
    {
        $this->parser1 = sequence(ch("a"), h_epsilon_p(), ch("b"));
        $this->parser2 = sequence(h_epsilon_p(), ch("a"));
        $this->parser3 = sequence(ch("a"), h_epsilon_p());
    }

    public function testSuccess1()
    {
        $result = h_parse($this->parser1, "ab");
        $this->assertEquals(array("a", "b"), $result);
    }

    public function testSuccess2()
    {
        $result = h_parse($this->parser2, "a");
        $this->assertEquals(array("a"), $result);
    }

    public function testSuccess3()
    {
        $result = h_parse($this->parser3, "ab");
        $this->assertEquals(array("a"), $result);
    }
}
?>