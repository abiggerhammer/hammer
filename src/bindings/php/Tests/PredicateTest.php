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
        $this->parser = predicate(h_many1(choice(ch('a'), ch('b'))), "predTest");
    }
    public function testSuccess()
    {
        $result1 = h_parse($this->parser, "aa");
        $result2 = h_parse($this->parser, "bb");
        $this->assertEquals(["a", "a"], $result1);
        $this->assertEquals(["b", "b"], $result2);
    }
    public function testFailure()
    {
        $result = h_parse($this->parser, "ab");
        $this->assertEquals(NULL, $result);
    }
}

?>