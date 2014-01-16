#ifndef HAMMER_HAMMER__HPP
#define HAMMER_HAMMER__HPP

#include <hammer/hammer.h>
#include <string>
#include <stdint.h>

namespace hammer {
  class Parser {
    
  };

  class ParsedToken {
    // This object can suddenly become invalid if the underlying parse
    // tree is destroyed.
    
    // This object should serve as a very thin wrapper around an HParsedToken*.
    // In particular sizeof(ParsedToken) should== sizeof(HParsedToken*)
    // This means that we only get one member variable and no virtual functions.
  protected:
    HParsedToken *token;
    
  public:
      
    ParsedToken(HParsedToken *inner) : token(inner) {}
    ParsedToken(const ParsedToken &other) : token(other.token) {}
    
    inline TokenType getType() {
      return type;
    }

    void* getUser() {
      return token->user;
    }
    // TODO: add accessors.
    
    
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

    ParsedToken getAST() {
      return ParsedToken(_result);
    }
    inline string asUnambiguous() {
      return getAST().asUnambiguous();
    }

    bool operator bool() {
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

  inline Parser token(const std::string &str) {
    std::string *str_clone = new std::string(str);
    return Parser(h_token(str_clone->data(), str.
  }
  Parser token(const uint8_t *buf, size_t len);

  Parser ch(char ch);

  Parser ch_range(uint8_t lower, uint8_t upper);

  Parser int64();
  Parser int32();
  Parser int16();
  Parser int8();

  Parser uint64();
  Parser uint32();
  Parser uint16();
  Parser uint8();

  Parser int_range(Parser p, int64_t lower, int64_t upper);

  Parser bits(size_t len, bool sign);

  Parser whitespace(Parser p);
  
  Parser left(Parser p, Parser q);

  Parser right(Parser p, Parser q);

  Parser middle(Parser p, Parser q, Parser r);

  // TODO: Define Action
  //Parser action(Parser p, Action a);

  Parser in(string charset);
  Parser in(const uint8_t *charset, size_t length);
  Parser in(std::set<uint8_t> charset);

  Parser not_in(string charset);
  Parser not_in(const uint8_t *charset, size_t length);
  Parser not_in(std::set<uint8_t> charset);

  Parser end();

  Parser nothing();

  // TODO: figure out varargs
  //Parser sequence();
  //
  //Parser choice();

  Parser butnot(Parser p1, Parser p2);

  Parser difference(Parser p1, Parser p2);

  Parser xor_(Parser p1, Parser p2);

  Parser many(Parser p);

  Parser many1(Parser p);

  Parser repeat_n(Parser p, size_t n);

  Parser optional(Parser p);

  Parser ignore(Parser p);

  Parser sepBy(Parser p, Parser sep);

  Parser sepBy1(Parser p, Parser sep);

  Parser epsilon();

  Parser length_value(Parser length, Parser value);

  // Was attr_bool in the old C bindings.
  // TODO: Figure out closure
  //Parser validate(Parser p, Predicate pred);

  Parser and_(Parser p);

  Parser not_(Parser p);

  IndirectParser indirect();
  static inline void bind_indirect(IndirectParser &indirect, Parser p) {
    indirect.bind(p);
  }
}
#endif
