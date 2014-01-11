module Minitest

  module Assertions
    HAMMER_JUST_PARSE = Object.new
    def assert_parse_ok(parser, probe, expected=HAMMER_JUST_PARSE)
      refute_nil parser, "Parser must not be nil (this is a problem with your test)"
      parse_result = parser.parse(probe)
      refute_nil parse_result, "Parse failed"
      if HAMMER_JUST_PARSE != expected
        if parse_result.ast == nil
          assert_nil expected, "Parser returned nil AST; expected #{expected}"
        else
          assert_equal parse_result.ast.unmarshal, expected
        end
      end
    end

    def refute_parse_ok(parser, probe)
      refute_nil parser, "Parser must not be nil (this is a problem with your test)"
      parse_result = parser.parse(probe)

      if not parse_result.nil?
        assert_nil parse_result, "Parse succeeded unexpectedly with " + parse_result.ast.inspect
      end
    end
  end
  
  
 #def self.plugin_hammer-parser_init(options)
end
    
