#include <glib.h>
#include <stdint.h>
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

typedef struct {
  uint64_t data;
  size_t nbits;
} bitwriter_test_elem; // should end with {0,0}

void run_bitwriter_test(bitwriter_test_elem data[], char flags) {
  size_t len;
  const uint8_t *buf;
  HBitWriter *w = h_bit_writer_new(&system_allocator);
  int i;
  w->flags = flags;
  for (i = 0; data[i].nbits; i++) {
    h_bit_writer_put(w, data[i].data, data[i].nbits);
  }

  buf = h_bit_writer_get_buffer(w, &len);
  HInputStream input = {
    .input = buf,
    .index = 0,
    .length = len,
    .bit_offset = 0,
    .endianness = flags,
    .overrun = 0
  };

  for (i = 0; data[i].nbits; i++) {
    g_check_cmp_uint64((uint64_t)h_read_bits(&input, data[i].nbits, FALSE), ==,  data[i].data);
  }
}

static void test_bitwriter_ints(void) {
  bitwriter_test_elem data[] = {
    { -0x200000000, 64 },
    { 0,0 }
  };
  run_bitwriter_test(data, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
}

static void test_bitwriter_be(void) {
  bitwriter_test_elem data[] = {
    { 0x03, 3 },
    { 0x52, 8 },
    { 0x1A, 5 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
}

static void test_bitwriter_le(void) {
  bitwriter_test_elem data[] = {
    { 0x02, 3 },
    { 0x4D, 8 },
    { 0x0B, 5 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
}

static void test_largebits_be(void) {
  bitwriter_test_elem data[] = {
    { 0x352, 11 },
    { 0x1A, 5 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
}

static void test_largebits_le(void) {
  bitwriter_test_elem data[] = {
    { 0x26A, 11 },
    { 0x0B, 5 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
}

static void test_offset_largebits_be(void) {
  bitwriter_test_elem data[] = {
    { 0xD, 5 },
    { 0x25A, 11 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
}

static void test_offset_largebits_le(void) {
  bitwriter_test_elem data[] = {
    { 0xA, 5 },
    { 0x2D3, 11 },
    { 0, 0 }
  };
  run_bitwriter_test(data, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
}

void register_bitwriter_tests(void) {
  g_test_add_func("/core/bitwriter/be", test_bitwriter_be);
  g_test_add_func("/core/bitwriter/le", test_bitwriter_le);
  g_test_add_func("/core/bitwriter/largebits-be", test_largebits_be);
  g_test_add_func("/core/bitwriter/largebits-le", test_largebits_le);
  g_test_add_func("/core/bitwriter/offset-largebits-be", test_offset_largebits_be);
  g_test_add_func("/core/bitwriter/offset-largebits-le", test_offset_largebits_le);
  g_test_add_func("/core/bitwriter/ints", test_bitwriter_ints);
}
