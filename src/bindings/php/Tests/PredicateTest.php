<?php
include_once 'hammer.php';

function predTest($token)
{
    return ($token[0] === $token[1]);
}

class PredicateTest extends PHPUnit_Framework_TestCase
{
    protected $parser;

    protected function setUp()
    {
        $this->parser = hammer_predicate(hammer_many1(hammer_choice(hammer_ch('a'), hammer_ch('b'))), "predTest");
    }
    public function testSuccess()
    {
        $result1 = hammer_parse($this->parser, "aa");
        $result2 = hammer_parse($this->parser, "bb");
        $this->assertEquals(["a", "a"], $result1);
        $this->assertEquals(["b", "b"], $result2);
    }
    public function testFailure()
    {
        $result = hammer_parse($this->parser, "ab");
        $this->assertEquals(NULL, $result);
    }
}

?>