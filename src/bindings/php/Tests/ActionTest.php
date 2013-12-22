<?php
include_once 'hammer.php';

function actTest($token) 
{
    if (is_array($token) === true) {
        foreach($token as $chr) {
            $ret[] = strtoupper($chr);
        }
        return $ret;
    } else {
        return strtoupper($token);
    }
}

class ActionTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_action(hammer_sequence(hammer_choice(hammer_ch("a"), hammer_ch("A")), hammer_choice(hammer_ch("b"), hammer_ch("B"))), "actTest");
    }
    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "ab");
        $result2 = hammer_parse($this->parser, "AB");
        $result3 = hammer_parse($this->parser, "aB");
        $this->assertEquals(["A", "B"], $result1);
        $this->assertEquals(["A", "B"], $result2);
        $this->assertEquals(["A", "B"], $result3);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "XX");
        $this->assertEquals(NULL, $result);
    }
}

?>