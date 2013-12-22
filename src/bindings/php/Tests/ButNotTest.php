<?php
include_once 'hammer.php';

class ButNotTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;

    protected function setUp()
    {
        $this->parser1 = hammer_butnot(hammer_ch("a"), hammer_token("ab"));
        $this->parser2 = hammer_butnot(hammer_ch_range('0', '9'), hammer_ch('6'));
    }

    public function testSuccess1()
    {
        $result1 = hammer_parse($this->parser1, "a");
        $result2 = hammer_parse($this->parser1, "aa");
        $this->assertEquals("a", $result1);
        $this->assertEquals("a", $result1);
    }

    public function testFailure1()
    {
        $result = hammer_parse($this->parser1, "ab");
        $this->assertEquals(NULL, $result);
    }

    public function testFailure2()
    {
        $result = hammer_parse($this->parser2, "6");
        $this->assertEquals(NULL, $result);
    }
}
?>