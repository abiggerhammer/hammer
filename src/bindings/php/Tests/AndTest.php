<?php
include_once 'hammer.php';

class AndTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    protected $parser3;

    protected function setUp()
    {
        $this->parser1 = hammer_sequence(hammer_and(hammer_ch("0")), hammer_ch("0"));
        $this->parser2 = hammer_sequence(hammer_and(hammer_ch("0")), hammer_ch("1"));
        $this->parser3 = hammer_sequence(hammer_ch("1"), hammer_and(hammer_ch("2")));
    }

    public function testSuccess1()
    {
        $result = hammer_parse($this->parser1, "0");
        $this->assertEquals(array("0"), $result);
    }

    public function testFailure2()
    {
        $result = hammer_parse($this->parser2, "0");
        $this->assertEquals(NULL, $result);
    }

    public function testSuccess3()
    {
        $result = hammer_parse($this->parser3, "12");
        $this->assertEquals(array("1"), $result);
    }
}
?>