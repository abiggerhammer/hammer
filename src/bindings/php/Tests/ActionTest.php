<?php
include_once 'hammer.php';

class ActionTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function actTest(string $token) 
    {
        return strtoupper($token);
    }
    protected function setUp()
    {
        $this->parser = h_action(sequence(choice(ch("a"), ch("A")), choice(ch("b"), ch("B"))), "actTest");
    }
    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "ab");
        $result2 = h_parse($this->parser, "AB");
        $result3 = h_parse($this->parser, "aB");
        $this->assertEquals("AB", $result1);
        $this->assertEquals("AB", $result2);
        $this->assertEquals("AB", $result3);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "XX");
        $this->assertEquals(NULL, $result);
    }
}

?>