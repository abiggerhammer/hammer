#include <glib.h>
#include <stdint.h>
#include "glue.h"
#include "hammer.h"
#include "test_suite.h"
#include "internal.h"

static void test_bug118(void) {
  // https://github.com/UpstandingHackers/hammer/issues/118
  // Adapted from https://gist.github.com/mrdomino/c6bc91a7cb3b9817edb5

  HParseResult* p;
  const uint8_t *input = (uint8_t*)"\x69\x5A\x6A\x7A\x8A\x9A";
 
#define MY_ENDIAN (BIT_BIG_ENDIAN | BYTE_LITTLE_ENDIAN)
    H_RULE(nibble, h_with_endianness(MY_ENDIAN, h_bits(4, false)));
    H_RULE(sample, h_with_endianness(MY_ENDIAN, h_bits(10, false)));
#undef MY_ENDIAN
 
    H_RULE(samples, h_sequence(h_repeat_n(sample, 3), h_ignore(h_bits(2, false)), NULL));
 
    H_RULE(header_ok, h_sequence(nibble, nibble, NULL));
    H_RULE(header_weird, h_sequence(nibble, nibble, nibble, NULL));
 
    H_RULE(parser_ok, h_sequence(header_ok, samples, NULL));
    H_RULE(parser_weird, h_sequence(header_weird, samples, NULL));
 
 
    p = h_parse(parser_weird, input, 6);
    g_check_cmp_int32(p->bit_length, ==, 44);
    h_parse_result_free(p);
    p = h_parse(parser_ok, input, 6);
    g_check_cmp_int32(p->bit_length, ==, 40);
    h_parse_result_free(p);
}

static void test_seq_index_path(void) {
  HArena *arena = h_new_arena(&system_allocator, 0);

  HParsedToken *seq = h_make_seqn(arena, 1);
  HParsedToken *seq2 = h_make_seqn(arena, 2);
  HParsedToken *tok1 = h_make_uint(arena, 41);
  HParsedToken *tok2 = h_make_uint(arena, 42);

  seq->seq->elements[0] = seq2;
  seq->seq->used = 1;
  seq2->seq->elements[0] = tok1;
  seq2->seq->elements[1] = tok2;
  seq2->seq->used = 2;

  g_check_cmp_int(h_seq_index_path(seq, 0, -1)->token_type, ==, TT_SEQUENCE);
  g_check_cmp_int(h_seq_index_path(seq, 0, 0, -1)->token_type, ==, TT_UINT);
  g_check_cmp_int64(h_seq_index_path(seq, 0, 0, -1)->uint, ==, 41);
  g_check_cmp_int64(h_seq_index_path(seq, 0, 1, -1)->uint, ==, 42);
}

#define MK_INPUT_STREAM(buf,len,endianness_)  \
  {					      \
      .input = (uint8_t*)buf,		      \
      .length = len,			      \
      .index = 0,			      \
      .bit_offset = 0,			      \
      .endianness = endianness_		      \
  }

static void test_read_bits_48(void) {
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 32, false), ==, 0x78563412);
    g_check_cmp_int64(h_read_bits(&is, 16, false), ==, 0xBC9A);
  }
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 31, false), ==, 0x78563412);
    g_check_cmp_int64(h_read_bits(&is, 17, false), ==, 0x17934);
  }
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 33, false), ==, 0x78563412);
    g_check_cmp_int64(h_read_bits(&is, 17, false), ==, 0x5E4D);
  }
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 36, false), ==, 0xA78563412);
    g_check_cmp_int64(h_read_bits(&is, 12, false), ==, 0xBC9);
  }
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 40, false), ==, 0x9A78563412);
    g_check_cmp_int64(h_read_bits(&is, 8, false), ==, 0xBC);
  }
  {
    HInputStream is = MK_INPUT_STREAM("\x12\x34\x56\x78\x9A\xBC", 6, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
    g_check_cmp_int64(h_read_bits(&is, 48, false), ==, 0xBC9A78563412);
  }
}

static void test_llk_zero_end(void) {
    HParserBackend be = PB_LLk;
    HParser *z = h_ch('\x00');
    HParser *az = h_sequence(h_ch('a'), z, NULL);
    HParser *ze = h_sequence(z, h_end_p(), NULL);
    HParser *aze = h_sequence(h_ch('a'), z, h_end_p(), NULL);

    // some cases surrounding the bug
    g_check_parse_match (z, be, "\x00", 1, "u0");
    g_check_parse_failed(z, be, "", 0);
    g_check_parse_match (ze, be, "\x00", 1, "(u0)");
    g_check_parse_failed(ze, be, "\x00b", 2);
    g_check_parse_failed(ze, be, "", 0);
    g_check_parse_match (az, be, "a\x00", 2, "(u0x61 u0)");
    g_check_parse_match (aze, be, "a\x00", 2, "(u0x61 u0)");
    g_check_parse_failed(aze, be, "a\x00b", 3);

    // the following should not parse but did when the LL(k) backend failed to
    // check for the end of input, mistaking it for a zero character.
    g_check_parse_failed(az, be, "a", 1);
    g_check_parse_failed(aze, be, "a", 1);
}

static void test_lalr_charset_lhs(void) {
    HParserBackend be = PB_LALR;

    HParser *p = h_many(h_choice(h_sequence(h_ch('A'), h_ch('B'), NULL),
                                 h_in((uint8_t*)"AB",2), NULL));

    // the above would abort because of an unhandled case in trying to resolve
    // a conflict where an item's left-hand-side was an HCF_CHARSET.
    // however, the compile should fail - the conflict cannot be resolved.

    if(h_compile(p, be, NULL) == 0) {
        g_test_message("LALR compile didn't detect ambiguous grammar");

        // it says it compiled it - well, then it should parse it!
        // (this helps us see what it thinks it should be doing.)
        g_check_parse_match(p, be, "AA",2, "(u0x41 u0x41)");
        g_check_parse_match(p, be, "AB",2, "((u0x41 u0x42))");

        g_test_fail();
        return;
    }
}

static void test_cfg_many_seq(void) {
    HParser *p = h_many(h_sequence(h_ch('A'), h_ch('B'), NULL));

    g_check_parse_match(p, PB_LLk,  "ABAB",4, "((u0x41 u0x42) (u0x41 u0x42))");
    g_check_parse_match(p, PB_LALR, "ABAB",4, "((u0x41 u0x42) (u0x41 u0x42))");
    g_check_parse_match(p, PB_GLR,  "ABAB",4, "((u0x41 u0x42) (u0x41 u0x42))");
    // these would instead parse as (u0x41 u0x42 u0x41 u0x42) due to a faulty
    // reshape on h_many.
}

static uint8_t test_charset_bits__buf[256];
static void *test_charset_bits__alloc(HAllocator *allocator, size_t size)
{
    g_check_cmp_uint64(size, ==, 256/8);
    assert(size <= 256);
    return test_charset_bits__buf;
}
static void test_charset_bits(void) {
    // charset would allocate 256 bytes instead of 256 bits (= 32 bytes)

    HAllocator alloc = {
        .alloc = test_charset_bits__alloc,
        .realloc = NULL,
        .free = NULL,
    };
    test_charset_bits__buf[32] = 0xAB;
    HCharset cs = new_charset(&alloc);
    for(size_t i=0; i<32; i++)
        g_check_cmp_uint32(test_charset_bits__buf[i], ==, 0);
    g_check_cmp_uint32(test_charset_bits__buf[32], ==, 0xAB);
}

void register_regression_tests(void) {
  g_test_add_func("/core/regression/bug118", test_bug118);
  g_test_add_func("/core/regression/seq_index_path", test_seq_index_path);
  g_test_add_func("/core/regression/read_bits_48", test_read_bits_48);
  g_test_add_func("/core/regression/llk_zero_end", test_llk_zero_end);
  g_test_add_func("/core/regression/lalr_charset_lhs", test_lalr_charset_lhs);
  g_test_add_func("/core/regression/cfg_many_seq", test_cfg_many_seq);
  g_test_add_func("/core/regression/charset_bits", test_charset_bits);
}
