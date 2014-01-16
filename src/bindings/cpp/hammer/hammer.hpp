#ifndef HAMMER_HAMMER__HPP
#define HAMMER_HAMMER__HPP

#include <hammer/hammer.h>
#include <string>
#include <stdint.h>

namespace hammer {
  class ParseResult;
  class Parser {
  public:
    const HParser *parser;

    Parser(const HParser* inner) : parser(inner) {}
    Parser(const Parser &other) : parser(other.parser) {}

    ParseResult parse(std::string string);
  };

  class ParsedToken {
    // This object can suddenly become invalid if the underlying parse
    // tree is destroyed.
    
    // This object should serve as a very thin wrapper around an HParsedToken*.
    // In particular sizeof(ParsedToken) should== sizeof(HParsedToken*)
    // This means that we only get one member variable and no virtual functions.
  protected:
    const HParsedToken *token;
    
  public:

    ParsedToken(const HParsedToken *inner) : token(inner) {}
    ParsedToken(const ParsedToken &other) : token(other.token) {}
    
    inline HTokenType getType() {
      return token->token_type;
    }

    void* getUser() const {return token->user;}
    uint64_t getUint() const {return token->uint;}
    int64_t getSint() const {return token->sint;}
    // TODO: Sequence GetSeq() const {return Sequence(token->seq);}
    std::string getBytes() const {return std::string((char*)token->bytes.token, token->bytes.len); }
    
    
    std::string asUnambiguous() {
      char* buf = h_write_result_unamb(token);
      std::string s = std::string(buf);
      free(buf);
      return s;
    }
  };
  
  class ParseResult {
  protected:
    HParseResult *_result;
  public:

    ParseResult(HParseResult *result) : _result(result) {}
    
    ParsedToken getAST() {
      return ParsedToken(_result->ast);
    }
    inline std::string asUnambiguous() {
      return getAST().asUnambiguous();
    }

    operator bool() {
      return _result != NULL;
    }
    bool operator !() {
      return _result == NULL;
    }
    
    ~ParseResult() {
      h_parse_result_free(_result);
      _result = NULL;
    }
  };

  inline ParseResult Parser::parse(std::string string) {
    return ParseResult(h_parse(parser, (uint8_t*)string.data(), string.length()));
  }

  
  static inline Parser Token(const std::string &str) {
    return Parser(h_token((const uint8_t*)str.data(), str.length()));
  }
  static inline Parser Token(const uint8_t *buf, size_t len) {
    return Parser(h_token(buf, len));
  }
  static inline Parser Ch(char ch) {
    return Parser(h_ch(ch));
  }
  static inline Parser ChRange(uint8_t lower, uint8_t upper) {
    return Parser(h_ch_range(lower,upper));
  }

  static inline Parser int64() { return Parser(h_int64()); }
  static inline Parser int32() { return Parser(h_int32()); }
  static inline Parser int16() { return Parser(h_int16()); }
  static inline Parser int8 () { return Parser(h_int8 ()); }

  static inline Parser uint64() { return Parser(h_uint64()); }
  static inline Parser uint32() { return Parser(h_uint32()); }
  static inline Parser uint16() { return Parser(h_uint16()); }
  static inline Parser uint8 () { return Parser(h_uint8 ()); }

  static inline Parser int_range(Parser p, int64_t lower, int64_t upper) {
    return Parser(h_int_range(p.parser, lower, upper));
  }

  static inline Parser bits(size_t len, bool sign) { return Parser(h_bits(len, sign)); }
  static inline Parser whitespace(Parser p) { return Parser(h_whitespace(p.parser)); }
  static inline Parser left(Parser p, Parser q) { return Parser(h_left(p.parser, q.parser)); }
  static inline Parser right(Parser p, Parser q) { return Parser(h_right(p.parser, q.parser)); }
  static inline Parser middle(Parser p, Parser q, Parser r) {
    return Parser(h_middle(p.parser, q.parser, r.parser));
  }

  // TODO: Define Action
  //Parser action(Parser p, Action a);

  static inline Parser in(std::string charset);
  static inline Parser in(const uint8_t *charset, size_t length);
  static inline Parser in(std::set<uint8_t> charset);

  static inline Parser not_in(std::string charset);
  static inline Parser not_in(const uint8_t *charset, size_t length);
  static inline Parser not_in(std::set<uint8_t> charset);

  static inline Parser end() { return Parser(h_end_p()); }
  static inline Parser nothing() { return Parser(h_nothing_p()); }

  // TODO: figure out varargs
  //Parser sequence();
  //
  //Parser choice();

  static inline Parser butnot(Parser p1, Parser p2) { return Parser(h_butnot(p1.parser, p2.parser)); }
  static inline Parser difference(Parser p1, Parser p2) { return Parser(h_difference(p1.parser, p2.parser)); }
  static inline Parser xor_(Parser p1, Parser p2) { return Parser(h_xor(p1.parser, p2.parser)); }
  static inline Parser many(Parser p) { return Parser(h_many(p.parser)); }
  static inline Parser many1(Parser p) { return Parser(h_many1(p.parser)); }
  static inline Parser repeat_n(Parser p, size_t n) { return Parser(h_repeat_n(p.parser, n)); }
  static inline Parser optional(Parser p) { return Parser(h_optional(p.parser)); }
  static inline Parser ignore(Parser p) { return Parser(h_ignore(p.parser)); }
  static inline Parser sepBy(Parser p, Parser sep) { return Parser(h_sepBy(p.parser, sep.parser)); }
  static inline Parser sepBy1(Parser p, Parser sep) { return Parser(h_sepBy1(p.parser, sep.parser)); }
  static inline Parser epsilon() { return Parser(h_epsilon_p()); }
  static inline Parser length_value(Parser length, Parser value) { return Parser(h_length_value(length.parser, value.parser)); }

  // Was attr_bool in the old C bindings.
  // TODO: Figure out closure
  //Parser validate(Parser p, Predicate pred);

  static inline Parser and_(Parser p) { return Parser(h_and(p.parser)); }
  static inline Parser not_(Parser p) { return Parser(h_not(p.parser)); }

  class Indirect : public Parser {
  public:
    Indirect() : Parser(h_indirect()) {}
    void bind(Parser p) {
      h_bind_indirect((HParser*)parser, p.parser);
    }
  };

  //inline operator Parser(HParser* foo) {
  //  return Parser(foo);
  //}
}
#endif
