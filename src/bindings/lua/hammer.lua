local ffi = require("ffi")
ffi.cdef[[
static const BYTE_BIG_ENDIAN = 0x1
static const BIT_BIG_ENDIAN = 0x2
static const BYTE_LITTLE_ENDIAN = 0x0
static const BIT_LITTLE_ENDIAN = 0x0

typedef enum HParserBackend_ {
  PB_MIN = 0,
  PB_PACKRAT = PB_MIN, // PB_MIN is always the default.
  PB_REGULAR,
  PB_LLk,
  PB_LALR,
  PB_GLR,
  PB_MAX = PB_GLR
} HParserBackend;

typedef enum HTokenType_ {
  TT_NONE = 1,
  TT_BYTES = 2,
  TT_SINT = 4,
  TT_UINT = 8,
  TT_SEQUENCE = 16,
  TT_RESERVED_1, // reserved for backend-specific internal use
  TT_ERR = 32,
  TT_USER = 64,
  TT_MAX
} HTokenType;

typedef struct HBytes_ {
  const uint8_t *token;
  size_t len;
} HBytes;

typedef struct HArena_ HArena ; // hidden implementation

typedef struct HCountedArray_ {
  size_t capacity;
  size_t used;
  HArena * arena;
  struct HParsedToken_ **elements;
} HCountedArray;

typedef struct HParsedToken_ {
  HTokenType token_type;
  union {
    HBytes bytes;
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    HCountedArray *seq; // a sequence of HParsedToken's
    void *user;
  };
  size_t index;
  size_t bit_length;
  char bit_offset;
} HParsedToken;

typedef struct HParseResult_ {
  const HParsedToken *ast;
  int64_t bit_length;
  HArena * arena;
} HParseResult;

typedef struct HParserVtable_ HParserVtable;
typedef struct HCFChoice_ HCFChoice;

typedef struct HParser_ {
  const HParserVtable *vtable;
  HParserBackend backend;
  void* backend_data;
  void *env;
  HCFChoice *desugared;
} HParser;

typedef HParsedToken* (*HAction)(const HParseResult *p, void* user_data);
typedef bool (*HPredicate)(HParseResult *p, void* user_data);
typedef HParser* (*HContinuation)(HAllocator *mm__, const HParsedToken *x, void *env);

HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length);
HParser* h_token(const uint8_t *str, const size_t len);
HParser* h_ch(const uint8_t c);
HParser* h_ch_range(const uint8_t lower, const uint8_t upper);
HParser* h_int_range(const HParser *p, const int64_t lower, const int64_t upper);
HParser* h_bits(size_t len, bool sign);
HParser* h_int64();
HParser* h_int32();
HParser* h_int16();
HParser* h_int8();
HParser* h_uint64();
HParser* h_uint32();
HParser* h_uint16();
HParser* h_uint8();
HParser* h_whitespace(const HParser* p);
HParser* h_left(const HParser* p, const HParser* q);
HParser* h_right(const HParser* p, const HParser* q);
HParser* h_middle(const HParser* p, const HParser* x, const HParser* q);
HParser* h_action(const HParser* p, const HAction a, void* user_data);
HParser* h_in(const uint8_t *charset, size_t length);
HParser* h_not_in(const uint8_t *charset, size_t length);
HParser* h_end_p();
HParser* h_nothing_p();
HParser* h_sequence__a(void *args[]);
HParser* h_choice__a(void *args[]);
HParser* h_permutation__a(void *args[]);
HParser* h_butnot(const HParser* p1, const HParser* p2);
HParser* h_difference(const HParser* p1, const HParser* p2);
HParser* h_xor(const HParser* p1, const HParser* p2);
HParser* h_many(const HParser* p);
HParser* h_many1(const HParser* p);
HParser* h_repeat_n(const HParser* p, const size_t n);
HParser* h_optional(const HParser* p);
HParser* h_ignore(const HParser* p);
HParser* h_sepBy(const HParser* p);
HParser* h_sepBy1(const HParser* p);
HParser* h_epsilon_p();
HParser* h_length_value(const HParser* length, const HParser* value);
HParser* h_attr_bool(const HParser* p, HPredicate pred, void* user_data);
HParser* h_and(const HParser* p);
HParser* h_not(const HParser* p);
HParser* h_indirect(const HParser* p);
void h_bind_indirect(HParser* indirect, const HParser* inner);
HParser* h_with_endianness(char endianness, const HParser* p);
HParser* h_put_value(const HParser* p, const char* name);
HParser* h_get_value(const char* name);
HParser* h_bind(const HParser *p, HContinuation k, void *env);

int h_compile(HParser* parser, HParserBackend backend, const void* params);
]]
local h = ffi.load("hammer")

local function helper(a, n, b, ...)
  if   n == 0 then return a
  else             return b, helper(a, n-1, ...) end
end
local function append(a, ...)
  return helper(a, select('#', ...), ...)
end

local parser
local mt = {
  __index = {
    parse = function(p, str) return h.h_parse(p, str, #str) end,
  },
}
parser = ffi.metatype("HParser", mt)

local function token(str)
  return h.h_token(str, #str)
end
local function ch(c)
  if type(c) == "number" then
    return h.h_ch(c)
  else
    return h.h_ch(c:byte())
  end
end
local function ch_range(lower, upper)
  if type(lower) == "number" and type(upper) == "number" then
    return h.h_ch_range(lower, upper)
  -- FIXME this is really not thorough type checking
  else
    return h.h_ch_range(lower:byte(), upper:byte())
  end
end
local function int_range(parser, lower, upper)
  return h.h_int_range(parser, lower, upper)
end
local function bits(len, sign)
  return h.h_bits(len, sign)
end
local function int64()
  return h.h_int64()
end
local function int32()
  return h.h_int32()
end
local function int16()
  return h.h_int16()
end
local function int8()
  return h.h_int8()
end
local function uint64()
  return h.h_uint64()
end
local function uint32()
  return h.h_uint32()
end
local function uint16()
  return h.h_uint16()
end
local function uint8()
  return h.h_uint8()
end
local function whitespace(parser)
  return h.h_whitespace(parser)
end
local function left(parser1, parser2)
  return h.h_left(parser1, parser2)
end
local function right(parser1, parser2)
  return h.h_right(parser1, parser2)
end
local function middle(parser1, parser2, parser3)
  return h.h_middle(parser1, parser2, parser3)
end
-- There could also be an overload of this that doesn't
-- bother with the env pointer, and passes it as NIL by
-- default, but I'm not going to deal with overloads now.
local function action(parser, action, user_data)
  local cb = ffi.cast("HAction", action)
  return h.h_action(parser, cb, user_data)
end
local function in_(charset)
  return h.h_in(charset, #charset)
end
local function not_in(charset)
  return h.h_not_in(charset, #charset)
end
local function end_p()
  return h.h_end_p()
end
local function nothing_p()
  return h.h_nothing_p()
end
local function sequence(...)
  local parsers = append(nil, ...)
  return h.h_sequence__a(parsers)
end
local function choice(...)
  local parsers = append(nil, ...)
  return h.h_choice__a(parsers)
end
local function permutation(...)
  local parsers = append(nil, ...)
  return h.h_permutation__a(parsers)
end
local function butnot(parser1, parser2)
  return h.h_butnot(parser1, parser2)
end
local function difference(parser1, parser2)
  return h.h_difference(parser1, parser2)
end
local function xor(parser1, parser2)
  return h.h_xor(parser1, parser2)
end
local function many(parser)
  return h.h_many(parser)
end
local function many1(parser)
  return h.h_many1(parser)
end
local function repeat_n(parser, n)
  return h.h_repeat_n(parser, n)
end
local function optional(parser)
  return h.h_optional(parser)
end
local function ignore(parser)
  return h.h_ignore(parser)
end
local function sepBy(parser)
  return h.h_sepBy(parser)
end
local function sepBy1(parser)
  return h.h_sepBy1(parser)
end
local function epsilon_p()
  return h.h_epsilon_p()
end
local function length_value(length, value)
  return h.h_length_value(length, value)
end
local function attr_bool(parser, predicate, user_data)
  local cb = ffi.cast("HPredicate", predicate)
  return h.h_attr_bool(parser, cb, user_data)
end
local function and_(parser)
  return h.h_and(parser)
end
local function not_(parser)
  return h.h_not(parser)
end
local function indirect(parser)
  return h.h_indirect(parser)
end
local function bind_indirect(indirect, inner)
  return h.h_bind_indirect(indirect, inner)
end
local function with_endianness(endianness, parser)
  return h.h_with_endianness(endianness, parser)
end
local function put_value(parser, name)
  return h.h_put_value(parser, name)
end
local function get_value(name)
  return h.h_get_value(parser, name)
end
local function bind(parser, continuation, env)
  local cb = ffi.cast("HContinuation", continuation)
  return h.h_bind(parser, cb, env)
end

local function compile(parser, backend, params)
  return h.h_compile(parser, backend, params)
end
