#ifndef HAMMER_HAMMER__HXX
#define HAMMER_HAMMER__HXX

#include "hammer.h"
#include <list>
#include <string>
#include <vector>
#include <boost/variant.hpp>

using std::list; using std::string; using std::vector;
using boost::variant;

namespace hammer {

  template<typename T>
  class ParseResult {
  public:
    typedef T result_type;
  protected:
    ParseResult() { }
  };

  class BytesResult : public ParseResult<vector<uint8_t> > {
  public:
    typedef vector<uint8_t> result_type;
    BytesResult(const vector<uint8_t> res) : _bytes(res) { }
    const result_type result();
  private:
    BytesResult() { }
    result_type _bytes;
  };

  class UintResult : public ParseResult<uint64_t> {
  public:
    typedef uint64_t result_type;
    UintResult(const uint64_t res) : _uint(res) { }
    result_type result();
  private:
    UintResult() { }
    result_type _uint;
  };

  class IntResult : public ParseResult<int64_t> {
  public:
    typedef int64_t result_type;
    IntResult(const int64_t res) : _sint(res) { }
    result_type result();
  private:
    IntResult() { }
    result_type _sint;
  };

  class NullResult: public ParseResult<void*> {
  public:
    NullResult() { }
    typedef void* result_type;
    result_type result() { return NULL; }
  };

  class SequenceResult : public ParseResult<vector<variant<BytesResult, UintResult, IntResult, NullResult, SequenceResult> > > {
  public:
    typedef vector<variant<BytesResult, UintResult, IntResult, NullResult, SequenceResult> > result_type;
    SequenceResult(result_type res) : _seq(res) { }
    const result_type result();
  private:
    SequenceResult() { }
    result_type _seq;
  };

  /* forward declarations */
  template<class T> class Many;
  class Many1;
  template<class T> class Optional;
  class RepeatN;
  class Ignore;
  template<class T> class Indirect;
  template<class T> class IntRange;

  template<typename T>
  class Parser {
  public:
    typedef T result_type;
    result_type parse(const string &input);
    result_type parse(const uint8_t *input, size_t length);
    Many<T> many();
    RepeatN many(size_t n);
    Optional<T> optional();
    Ignore ignore();
    RepeatN operator[](size_t n);
    HParser* parser() { return _parser; }
    int compile(HParserBackend backend, const void* params) {
	return h_compile(_parser, backend, params);
    }
  protected:
    HParser* _parser;
    Parser() { }
    //    Parser(const Parser &p) : _parser(p.parser()) { } // hopefully we don't need a copy constructor...
  };

  class Token : public Parser<BytesResult> {
  public:
    Token(string &str) : _tok(str) { 
      _parser = h_token(reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
    }
    Token(const uint8_t *str, size_t length) : _tok(reinterpret_cast<const char*>(str), length) {
      _parser = h_token(str, length);
    }
  private:
    string _tok;
  };

  class Ch : public Parser<UintResult> {
  public:
    friend class Parser;
    Ch(const uint8_t c) : _c(c) {
      _parser = h_ch(c);
    }
  private:
    uint8_t _c;
  };

  class ChRange : public Parser<UintResult> {
  public:
    ChRange(const uint8_t lower, const uint8_t upper) : _lower(lower), _upper(upper) { 
      _parser = h_ch_range(lower, upper); 
    }
  private:
    uint8_t _lower, _upper;
  };

  class SignedBits : public Parser<IntResult> {
  public:
    SignedBits(size_t len) : _len(len) {
      _parser = h_bits(len, true);
    }
  private:
    size_t _len;
  };

  class UnsignedBits : public Parser<UintResult> {
  public:
    UnsignedBits(size_t len) : _len(len) {
      _parser = h_bits(len, false);
    }
  private:
    size_t _len;
  };

  class Int64 : public Parser<IntResult> {
  public:
    Int64() {
      _parser = h_int64();
    }
    IntRange<IntResult> in_range(const int64_t lower, const int64_t upper);
  };

  class Int32 : public Parser<IntResult> {
  public:
    Int32() {
      _parser = h_int32();
    }
    IntRange<IntResult> in_range(const int32_t lower, const int32_t upper);
  };

  class Int16 : public Parser<IntResult> {
  public:
    Int16() { 
      _parser = h_int16();
    }
    IntRange<IntResult> in_range(const int16_t lower, const int16_t upper);
  };

  class Int8 : public Parser<IntResult> {
  public:
    Int8() {
      _parser = h_int8();
    }
    IntRange<IntResult> in_range(const int8_t lower, const int8_t upper);
  };

  class Uint64 : public Parser<UintResult> {
  public:
    Uint64() {
      _parser = h_uint64(); 
    }
    IntRange<UintResult> in_range(const uint64_t lower, const uint64_t upper);
  };

  class Uint32 : public Parser<UintResult> {
  public:
    Uint32() { 
      _parser = h_uint32();
    }
    IntRange<UintResult> in_range(const uint32_t lower, const uint32_t upper);
  };

  class Uint16 : public Parser<UintResult> {
  public:
    Uint16() {
      _parser = h_uint16(); 
    }
    IntRange<UintResult> in_range(const uint16_t lower, const uint16_t upper);
  };

  class Uint8 : public Parser<UintResult> {
  public:
    Uint8() { 
      _parser = h_uint8();
    } 
    IntRange<UintResult> in_range(const uint8_t lower, const uint8_t upper);
  };

  template<class T>
  class IntRange : public Parser<T> {
  public:
    IntRange(Parser<T> &p, const int64_t lower, const int64_t upper) : _p(p), _lower(lower), _upper(upper) {
      this->_parser = h_int_range(p.parser(), lower, upper);
    }
  private:
    Parser<T> _p;
    int64_t _lower, _upper;
  };

  template<class T>
  class Whitespace : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    Whitespace(Parser<T> &p) : _p(p) {
      this->_parser = h_whitespace(p.parser()); 
    }
  private: 
    Parser<T> _p;
  };

  template<class T, class U>
  class Left : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    Left(Parser<T> &p, Parser<U> &q) : _p(p), _q(q) {
      this->_parser = h_left(p.parser(), q.parser());
    }
  private:
    Parser<T> _p;
    Parser<U> _q;
  };

  template<class T, class U>
  class Right : public Parser<U> {
  public:
    typedef typename U::result_type result_type;
    Right(Parser<T> &p, Parser<U> &q) : _p(p), _q(q) {
      this->_parser = h_right(p.parser(), q.parser());
    }
  private:
    Parser<T> _p;
    Parser<U> _q;
  };

  template <class T, class U, class V>
  class Middle : public Parser<U> {
  public:
    typedef typename U::result_type result_type;
    Middle(Parser<T> &p, Parser<U> &x, Parser<V> &q) : _p(p), _x(x), _q(q) {
      this->_parser = h_middle(p.parser(), x.parser(), q.parser()); 
    }
  private:
    Parser<T> _p;
    Parser<U> _x;
    Parser<V> _q;
  };

  /* what are we doing about h_action? */

  class In : public Parser<UintResult> {
  public:
    In(string &charset) : _charset(charset) {
      _parser = h_in(reinterpret_cast<const uint8_t*>(charset.c_str()), charset.size());
    }
    In(const uint8_t *charset, size_t length) : _charset(reinterpret_cast<const char*>(charset), length) {
	_parser = h_in(charset, length);
      }
  private:
    string _charset;
  };

  class NotIn : public Parser<UintResult> {
  public:
    NotIn(string &charset) : _charset(charset) {
      _parser = h_not_in(reinterpret_cast<const uint8_t*>(charset.c_str()), charset.size());
    }
    NotIn(const uint8_t *charset, size_t length) : _charset(reinterpret_cast<const char*>(charset), length) {
      _parser = h_not_in(charset, length);
    }
  private:
    string _charset;
  };

  class End : public Parser<NullResult> {
  public:
    End() {
      _parser = h_end_p();
    }
  };

  class Nothing : public Parser<NullResult> {
  public:
    Nothing() {
      _parser = h_nothing_p();
    } 
  };

  class Sequence : public Parser<SequenceResult> {
    friend class Parser;
  public:
    Sequence(list<Parser> &ps) : _ps(ps) { 
      void *parsers[ps.size()];
      size_t i = 0;
      for (list<Parser>::iterator it=ps.begin(); it != ps.end(); ++it, ++i) {
	parsers[i] = const_cast<HParser*>(it->parser());
      }
      _parser = h_sequence__a(parsers);
    }
    // maybe also a begin and end iterator version
  private:
    list<Parser> _ps;
  };

  class Choice : public Parser<SequenceResult> {
  public:
    Choice(list<Parser<SequenceResult> > &ps) : _ps(ps) { 
      void *parsers[ps.size()];
      size_t i = 0;
      for (list<Parser<SequenceResult> >::iterator it=ps.begin(); it != ps.end(); ++it, ++i) {
	parsers[i] = const_cast<HParser*>(it->parser());
      }
      _parser = h_choice__a(parsers);      
    }
  private:
    list<Parser<SequenceResult> > _ps;
  };

  template<class T, class U>
  class ButNot : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    ButNot(Parser<T> &p, Parser<U> &q) : _p(p), _q(q) {
      this->_parser = h_butnot(p.parser(), q.parser());
    }
  private:
    Parser<T> _p;
    Parser<U> _q;
  };

  template<class T, class U>
  class Difference : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    Difference(Parser<T> &p, Parser<U> &q) : _p(p), _q(q) {
      this->_parser = h_difference(p.parser(), q.parser());
    }
  private:
    Parser<T> _p;
    Parser<U> _q;
  };

  template<class T, class U>
  class Xor : public Parser<variant<T, U> > {
  public:
    typedef variant<T, U> result_type;
    Xor(Parser<T> &p, Parser<U> &q) : _p(p), _q(q) {
      this->_parser = h_xor(p.parser(), q.parser());
    }
  private:
    Parser<T> _p;
    Parser<U> _q;
  };

  template<class T>
  class Many : public Parser<SequenceResult> {
  public:
    Many(Parser<T> &p) : _p(p) {
      _parser = h_many(p.parser());
    }
  private:
    Parser<T> _p;
  };

  class Many1 : public Parser<SequenceResult> {
  public:
    Many1(Parser &p) : _p(p) {
      _parser = h_many1(p.parser());
    }
  private:
    Parser _p;
  };

  class RepeatN: public Parser<SequenceResult> {
  public:
    RepeatN(Parser &p, const size_t n) : _p(p), _n(n) {
      _parser = h_repeat_n(p.parser(), n);
    }
  private:
    Parser _p;
    size_t _n;
  };
  
  template<class T>
  class Optional : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    Optional(Parser<T> &p) : _p(p) { 
      this->_parser = h_optional(p.parser());
    }
  private:
    Parser<T> _p;
  };

  class Ignore : public Parser<NullResult> {
  public:
    Ignore(Parser &p) : _p(p) {
      _parser = h_ignore(p.parser());
    }
  private:
    Parser _p;
  };

  class SepBy : public Parser<SequenceResult> {
  public:
    SepBy(Parser &p, Parser &sep) : _p(p), _sep(sep) {
      _parser = h_sepBy(p.parser(), sep.parser()); 
    }
  private:
    Parser _p, _sep;
  };

  class SepBy1 : public Parser<SequenceResult> {
  public:
    SepBy1(Parser &p, Parser &sep) : _p(p), _sep(sep) {
      _parser = h_sepBy1(p.parser(), sep.parser());
    }
  private:
    Parser _p, _sep;
  };

  class Epsilon : public Parser<NullResult> {
  public:
    Epsilon() {
      _parser = h_epsilon_p();
    }
  };

  template<class T>
  class LengthValue : public Parser<SequenceResult> {
  public:
    LengthValue(Parser<UintResult> &length, Parser<T> &value) : _length(length), _value(value) {
      _parser = h_length_value(length.parser(), value.parser());
    }
  private:
    Parser<UintResult> _length;
    Parser<T> _value;
  };

  /* FIXME attr_bool */

  class And : public Parser<NullResult> {
  public:
    And(Parser &p) : _p(p) {
      _parser = h_and(p.parser());
    }
  private:
    Parser _p;
  };

  class Not : public Parser<NullResult> {
  public:
    Not(Parser &p) : _p(p) {
      _parser = h_not(p.parser());
    }
  private:
    Parser _p;
  };

  template<class T>
  class Indirect : public Parser<T> {
  public:
    typedef typename T::result_type result_type;
    /*
    Indirect(Parser<T> &p) : _p(p) {
      this->_parser = h_indirect();
      h_bind_indirect(this->_parser, p.parser());
    }
    */
    Indirect() : _p(0) {}
    Indirect bind(Parser<T> &p) {
      this->_parser = h_indirect();
      this->_p = p;
      h_bind_indirect(this->_parser, p.parser());
      return *this;
    }
  private:
    Parser<T> _p;
  };

}

#endif
