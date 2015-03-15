#include <stdint.h>
#include <glib.h>
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#define MK_INPUT_STREAM(buf,len,endianness_)  \
  {					      \
      .input = (uint8_t*)buf,		      \
      .length = len,			      \
      .index = 0,			      \
      .bit_offset = 0,			      \
      .endianness = endianness_		      \
  }


static void test_bitreader_ints(void) {
  HInputStream is = MK_INPUT_STREAM("\xFF\xFF\xFF\xFE\x00\x00\x00\x00", 8, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmp_int64(h_read_bits(&is, 64, true), ==, -0x200000000);
}

static void test_bitreader_be(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 3, false), ==, 0x03);
  g_check_cmp_int32(h_read_bits(&is, 8, false), ==, 0x52);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0x1A);
}
static void test_bitreader_le(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 3, false), ==, 0x02);
  g_check_cmp_int32(h_read_bits(&is, 8, false), ==, 0x4D);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0x0B);
}

static void test_largebits_be(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 11, false), ==, 0x352);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0x1A);
}
  
static void test_largebits_le(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 11, false), ==, 0x26A);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0x0B);
}

static void test_offset_largebits_be(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0xD);
  g_check_cmp_int32(h_read_bits(&is, 11, false), ==, 0x25A);
}
  
static void test_offset_largebits_le(void) {
  HInputStream is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmp_int32(h_read_bits(&is, 5, false), ==, 0xA);
  g_check_cmp_int32(h_read_bits(&is, 11, false), ==, 0x2D3);
}

void register_bitreader_tests(void)  {
  g_test_add_func("/core/bitreader/be", test_bitreader_be);
  g_test_add_func("/core/bitreader/le", test_bitreader_le);
  g_test_add_func("/core/bitreader/largebits-be", test_largebits_be);
  g_test_add_func("/core/bitreader/largebits-le", test_largebits_le);
  g_test_add_func("/core/bitreader/offset-largebits-be", test_offset_largebits_be);
  g_test_add_func("/core/bitreader/offset-largebits-le", test_offset_largebits_le);
  g_test_add_func("/core/bitreader/ints", test_bitreader_ints);
}
