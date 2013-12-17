<?php
include_once 'hammer.php';

class NotTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;

    protected function setUp()
    {
        $this->parser1 = hammer_sequence(hammer_ch("a"), hammer_choice(hammer_ch("+"), hammer_token("++")), hammer_ch("b"));
        $this->parser2 = hammer_sequence(hammer_ch("a"), hammer_choice(hammer_sequence(hammer_ch("+"), hammer_not(hammer_ch("+"))), hammer_token("++")), hammer_ch("b"));
    }

    public function testSuccess1()
    {
        $result = hammer_parse($this->parser1, "a+b");
        $this->assertEquals(array("a", "+", "b"), $result);
    }

    public function testFailure1()
    {
        $result = hammer_parse($this->parser1, "a++b");
        $this->assertEquals(NULL, $result);
    }

    public function testSuccess2()
    {
        $result1 = hammer_parse($this->parser2, "a+b");
        $result2 = hammer_parse($this->parser2, "a++b");
        $this->assertEquals(array("a", array("+"), "b"), $result1);
        $this->assertEquals(array("a", "++", "b"), $result2);
    }
}
?>