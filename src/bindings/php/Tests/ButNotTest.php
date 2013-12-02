<?php
include_once 'hammer.php';

class ButNotTest extends PHPUnit_Framework_TestCase
{
    protected $parser1;
    protected $parser2;

    protected function setUp()
    {
        $this->parser1 = h_butnot(ch("a"), h_token("ab"));
        $this->parser2 = h_butnot(h_ch_range('0', '9'), ch('6'));
    }

    public function testSuccess1()
    {
        $result1 = h_parse($this->parser1, "a");
        $result2 = h_parse($this->parser1, "aa");
        $this->assertEquals("a", $result1);
        $this->assertEquals("a", $result1);
    }

    public function testFailure1()
    {
        $result = h_parse($this->parser1, "ab");
        $this->assertEquals(NULL, $result);
    }

    public function testFailure2()
    {
        $result = h_parse($this->parser2, "6");
        $this->assertEquals(NULL, $result);
    }
}
?>