#include "hammer.hxx"

namespace hammer {

  typedef variant<BytesResult, UintResult, IntResult, NullResult, SequenceResult> AnyResult;

  const BytesResult::result_type BytesResult::result() { return _bytes; }
  const UintResult::result_type UintResult::result() { return _uint; }
  const IntResult::result_type IntResult::result() { return _sint; }
  const SequenceResult::result_type SequenceResult::result() { return _seq; }

  template<>
  BytesResult Parser<BytesResult>::parse(const string &input) {
    HParseResult *res = h_parse(_parser, reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    return BytesResult(vector<uint8_t>(res->ast->bytes.token, res->ast->bytes.token+res->ast->bytes.len));
  }

  template<>
  BytesResult Parser<BytesResult>::parse(const uint8_t *input, size_t length) {
    HParseResult *res = h_parse(_parser, input, length);
    return BytesResult(vector<uint8_t>(res->ast->bytes.token, res->ast->bytes.token+res->ast->bytes.len));
  }

  template<>
  UintResult Parser<UintResult>::parse(const string &input) {
    HParseResult *res = h_parse(_parser, reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    return UintResult(res->ast->uint);
  }

  template<>
  UintResult Parser<UintResult>::parse(const uint8_t *input, size_t length) {
    HParseResult *res = h_parse(_parser, input, length);
    return UintResult(res->ast->uint);
  }

  template<>
  IntResult Parser<IntResult>::parse(const string &input) {
    HParseResult *res = h_parse(_parser, reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    return IntResult(res->ast->sint);
  }

  template<>
  IntResult Parser<IntResult>::parse(const uint8_t *input, size_t length) {
    HParseResult *res = h_parse(_parser, input, length);
    return IntResult(res->ast->sint);
  }

  template<>
  NullResult Parser<NullResult>::parse(const string &input) {
    HParseResult *res = h_parse(_parser, reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    return NullResult();
  }

  template<>
  NullResult Parser<NullResult>::parse(const uint8_t *input, size_t length) {
    HParseResult *res = h_parse(_parser, input, length);
    return NullResult();
  }

  vector<AnyResult> make_seq(HCountedArray *seq) {
    vector<AnyResult> ret;
    for (size_t i=0; i<seq->used; ++i) {
      switch(seq->elements[i]->token_type) {
      case TT_NONE:
	ret.push_back(NullResult());
	break;
      case TT_BYTES:
	ret.push_back(BytesResult(vector<uint8_t>(seq->elements[i]->bytes.token, seq->elements[i]->bytes.token+seq->elements[i]->bytes.len)));
	break;
      case TT_SINT:
	ret.push_back(IntResult(seq->elements[i]->sint));
	break;
      case TT_UINT:
	ret.push_back(UintResult(seq->elements[i]->uint));
	break;
      case TT_SEQUENCE:
	ret.push_back(make_seq(seq->elements[i]->seq));
	break;
      default:
	//TODO some kind of error
	break;
      }
    }
    return ret;
  }

  template<>
  SequenceResult Parser<SequenceResult>::parse(const string &input) {
    HParseResult *res = h_parse(_parser, reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    return SequenceResult(make_seq(res->ast->seq));
  }

  template<>
  SequenceResult Parser<SequenceResult>::parse(const uint8_t *input, size_t length) {
    HParseResult *res = h_parse(_parser, input, length);
    return SequenceResult(make_seq(res->ast->seq));
  }

  template<class T>
  Many<T> Parser<T>::many() {
    return Many<T>(*this);
  }

  template<>
  Many<UintResult> Parser<UintResult>::many() {
    return Many<UintResult>(*this);
  }

  template<class T>
  RepeatN Parser<T>::many(size_t n) {
    return RepeatN(this, n);
  }

  template<class T>
  Optional<T> Parser<T>::optional() {
    return Optional<T>(this);
  }

  template<class T>
  RepeatN Parser<T>::operator[](size_t n) {
    return RepeatN(this, n);
  }

  IntRange<IntResult> Int64::in_range(const int64_t lower, const int64_t upper) {
    Int64 p = Int64();
    return IntRange<IntResult>(p, lower, upper);
  }

  IntRange<IntResult> Int32::in_range(const int32_t lower, const int32_t upper) {
    Int32 p = Int32();
    return IntRange<IntResult>(p, lower, upper);
  }

  IntRange<IntResult> Int16::in_range(const int16_t lower, const int16_t upper) {
    Int16 p = Int16();
    return IntRange<IntResult>(p, lower, upper);
  }

  IntRange<IntResult> Int8::in_range(const int8_t lower, const int8_t upper) {
    Int8 p = Int8();
    return IntRange<IntResult>(p, lower, upper);
  }

  IntRange<UintResult> Uint64::in_range(const uint64_t lower, const uint64_t upper) {
    Uint64 p = Uint64();
    return IntRange<UintResult>(p, lower, upper);
  }

  IntRange<UintResult> Uint32::in_range(const uint32_t lower, const uint32_t upper) {
    Uint32 p = Uint32();
    return IntRange<UintResult>(p, lower, upper);
  }

  IntRange<UintResult> Uint16::in_range(const uint16_t lower, const uint16_t upper) {
    Uint16 p = Uint16();
    return IntRange<UintResult>(p, lower, upper);
  }

  IntRange<UintResult> Uint8::in_range(const uint8_t lower, const uint8_t upper) {
    Uint8 p = Uint8();
    return IntRange<UintResult>(p, lower, upper);
  }
  
}
