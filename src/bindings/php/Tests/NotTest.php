<?php
include_once 'hammer.php';

class NotTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;

    protected function setUp()
    {
        $this->parser1 = sequence(ch("a"), choice(ch("+"), h_token("++")), ch("b"));
        $this->parser2 = sequence(ch("a"), choice(sequence(ch("+"), h_not(ch("+"))), h_token("++")), ch("b"));
    }

    public function testSuccess1()
    {
        $result = h_parse($this->parser1, "a+b");
        $this->assertEquals(array("a", "+", "b"), $result);
    }

    public function testFailure1()
    {
        $result = h_parse($this->parser1, "a++b");
        $this->assertEquals(NULL, $result);
    }

    public function testSuccess2()
    {
        $result1 = h_parse($this->parser2, "a+b");
        $result2 = h_parse($this->parser2, "a++b");
        $this->assertEquals(array("a", array("+"), "b"), $result1);
        $this->assertEquals(array("a", "++", "b"), $result2);
    }
}
?>