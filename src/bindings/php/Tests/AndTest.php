<?php
include_once 'hammer.php';

class AndTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;
    protected $parser3;

    protected function setUp()
    {
        $this->parser1 = sequence(h_and(ch("0")), ch("0"));
        $this->parser2 = sequence(h_and(ch("0")), ch("1"));
        $this->parser3 = sequence(ch("1"), h_and(ch("2")));
    }

    public function testSuccess1()
    {
        $result = h_parse($this->parser1, "0");
        $this->assertEquals(array("0"), $result);
    }

    public function testFailure2()
    {
        $result = h_parse($this->parser2, "0");
        $this->assertEquals(NULL, $result);
    }

    public function testSuccess3()
    {
        $result = h_parse($this->parser3, "12");
        $this->assertEquals(array("1"), $result);
    }
}
?>