local ffi = require("ffi")
ffi.cdef[[
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

typedef struct HAllocator_ HAllocator;

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
HParser* h_sequence(HParser* p, ...);
HParser* h_choice(HParser* p, ...);
HParser* h_permutation(HParser* p, ...);
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

static const uint8_t BYTE_BIG_ENDIAN = 0x1;
static const uint8_t BIT_BIG_ENDIAN = 0x2;
static const uint8_t BYTE_LITTLE_ENDIAN = 0x0;
static const uint8_t BIT_LITTLE_ENDIAN = 0x0;
]]
local h = ffi.load("hammer")

local function helper(a, n, b, ...)
  if   n == 0 then return a
  else             return b, helper(a, n-1, ...) end
end
local function append(a, ...)
  return helper(a, select('#', ...), ...)
end

local mt = {
  __index = {
    parse = function(p, str) return h.h_parse(p, str, #str) end,
  },
}
local hammer = {}
hammer.parser = ffi.metatype("HParser", mt)

local counted_array
local arr_mt = {
  __index = function(table, key)
    return table.elements[key]
  end,
  __len = function(table) return table.used end,
  __ipairs = function(table)
    local i, n = 0, #table
    return function()
      i = i + 1
      if i <= n then
        return i, table.elements[i]
      end
    end
  end,
  __call = function(self)
    ret = {}
    for i, v in ipairs(self)
      do ret[#ret+1] = v
    end
    return ret
  end
}
counted_array = ffi.metatype("HCountedArray", arr_mt)

local bytes_mt = {
  __call = function(self)
    local ret = ""
    for i = 0, tonumber(ffi.cast("uintptr_t", ffi.cast("void *", self.len)))-1
      do ret = ret .. string.char(self.token[i])
    end
    return ret
  end
}
local byte_string = ffi.metatype("HBytes", bytes_mt)

local token_types = ffi.new("HTokenType")

local parsed_token
local tok_mt = {
  __call = function(self)
     if self.token_type == ffi.C.TT_BYTES then
       return self.bytes()
     elseif self.token_type == ffi.C.TT_SINT then
       return tonumber(ffi.cast("intptr_t", ffi.cast("void *", self.sint)))
     elseif self.token_type == ffi.C.TT_UINT then
       return tonumber(ffi.cast("uintptr_t", ffi.cast("void *", self.uint)))
     elseif self.token_type == ffi.C.TT_SEQUENCE then
       return self.seq()
     end
  end
}
parsed_token = ffi.metatype("HParsedToken", tok_mt)

function hammer.token(str)
  return h.h_token(str, #str)
end
function hammer.ch(c)
  if type(c) == "number" then
    return h.h_ch(c)
  else
    return h.h_ch(c:byte())
  end
end
function hammer.ch_range(lower, upper)
  if type(lower) == "number" and type(upper) == "number" then
    return h.h_ch_range(lower, upper)
  -- FIXME this is really not thorough type checking
  else
    return h.h_ch_range(lower:byte(), upper:byte())
  end
end
function hammer.int_range(parser, lower, upper)
  return h.h_int_range(parser, lower, upper)
end
function hammer.bits(len, sign)
  return h.h_bits(len, sign)
end
function hammer.int64()
  return h.h_int64()
end
function hammer.int32()
  return h.h_int32()
end
function hammer.int16()
  return h.h_int16()
end
function hammer.int8()
  return h.h_int8()
end
function hammer.uint64()
  return h.h_uint64()
end
function hammer.uint32()
  return h.h_uint32()
end
function hammer.uint16()
  return h.h_uint16()
end
function hammer.uint8()
  return h.h_uint8()
end
function hammer.whitespace(parser)
  return h.h_whitespace(parser)
end
function hammer.left(parser1, parser2)
  return h.h_left(parser1, parser2)
end
function hammer.right(parser1, parser2)
  return h.h_right(parser1, parser2)
end
function hammer.middle(parser1, parser2, parser3)
  return h.h_middle(parser1, parser2, parser3)
end
-- There could also be an overload of this that doesn't
-- bother with the env pointer, and passes it as NIL by
-- default, but I'm not going to deal with overloads now.
function hammer.action(parser, action, user_data)
  local cb = ffi.cast("HAction", action)
  return h.h_action(parser, cb, user_data)
end
function hammer.in_(charset)
  local cs = ffi.new("const unsigned char[" .. #charset .. "]", charset)
  return h.h_in(cs, #charset)
end
function hammer.not_in(charset)
  return h.h_not_in(charset, #charset)
end
function hammer.end_p()
  return h.h_end_p()
end
function hammer.nothing_p()
  return h.h_nothing_p()
end
function hammer.sequence(parser, ...)
  local parsers = append(nil, ...)
  return h.h_sequence(parser, parsers)
end
function hammer.choice(parser, ...)
  local parsers = append(nil, ...)
  return h.h_choice(parser, parsers)
end
function hammer.permutation(parser, ...)
  local parsers = append(nil, ...)
  return h.h_permutation(parser, parsers)
end
function hammer.butnot(parser1, parser2)
  return h.h_butnot(parser1, parser2)
end
function hammer.difference(parser1, parser2)
  return h.h_difference(parser1, parser2)
end
function hammer.xor(parser1, parser2)
  return h.h_xor(parser1, parser2)
end
function hammer.many(parser)
  return h.h_many(parser)
end
function hammer.many1(parser)
  return h.h_many1(parser)
end
function hammer.repeat_n(parser, n)
  return h.h_repeat_n(parser, n)
end
function hammer.optional(parser)
  return h.h_optional(parser)
end
function hammer.ignore(parser)
  return h.h_ignore(parser)
end
function hammer.sepBy(parser)
  return h.h_sepBy(parser)
end
function hammer.sepBy1(parser)
  return h.h_sepBy1(parser)
end
function hammer.epsilon_p()
  return h.h_epsilon_p()
end
function hammer.length_value(length, value)
  return h.h_length_value(length, value)
end
function hammer.attr_bool(parser, predicate, user_data)
  local cb = ffi.cast("HPredicate", predicate)
  return h.h_attr_bool(parser, cb, user_data)
end
function hammer.and_(parser)
  return h.h_and(parser)
end
function hammer.not_(parser)
  return h.h_not(parser)
end
function hammer.indirect(parser)
  return h.h_indirect(parser)
end
function hammer.bind_indirect(indirect, inner)
  return h.h_bind_indirect(indirect, inner)
end
function hammer.with_endianness(endianness, parser)
  return h.h_with_endianness(endianness, parser)
end
function hammer.put_value(parser, name)
  return h.h_put_value(parser, name)
end
function hammer.get_value(name)
  return h.h_get_value(name)
end
function hammer.bind(parser, continuation, env)
  local cb = ffi.cast("HContinuation", continuation)
  return h.h_bind(parser, cb, env)
end

function hammer.compile(parser, backend, params)
  return h.h_compile(parser, backend, params)
end

hammer.BYTE_BIG_ENDIAN = 0x1;
hammer.BIT_BIG_ENDIAN = 0x2;
hammer.BYTE_LITTLE_ENDIAN = 0x0;
hammer.BIT_LITTLE_ENDIAN = 0x0;
return hammer