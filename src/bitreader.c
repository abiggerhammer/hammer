#include <stdint.h>
#include <stdio.h>
#include "internal.h"
#include "hammer.h"
#include "test_suite.h"

#define LSB(range) (0:range)
#define MSB(range) (1:range)
#define LDB(range,i) (((i)>>LSB(range))&((1<<(MSB(range)-LSB(range)+1))-1))


long long read_bits(input_stream_t* state, int count, char signed_p) {
  // BUG: Does not 
  long long out = 0;
  int offset = 0;
  int final_shift = 0;
  long long msb = (!!signed_p) << (count - 1); // 0 if unsigned, else 1 << (nbits - 1)
  
  
  // overflow check...
  int bits_left = (state->length - state->index); // well, bytes for now
  if (bits_left <= 64) { // Large enough to handle any valid count, but small enough that overflow isn't a problem.
    // not in danger of overflowing, so add in bits
    // add in number of bits...
    if (state->endianness & BIT_BIG_ENDIAN)
      bits_left = (bits_left << 3) - 8 + state->bit_offset;
    else
      bits_left = (bits_left << 3) - state->bit_offset;
    if (bits_left < count) {
      if (state->endianness & BYTE_BIG_ENDIAN)
	final_shift = count - bits_left;
      else
	final_shift = 0;
      count = bits_left;
      state->overrun = true;
    } else
      final_shift = 0;
  }
  
  if ((state->bit_offset & 0x7) == 0 && (count & 0x7) == 0) {
    // fast path
    if (state->endianness & BYTE_BIG_ENDIAN) {
      while (count > 0) {
	count -= 8;
	out = (out << 8) | state->input[state->index++];
      }
    } else {
      while (count > 0) {
	count -= 8;
	out |= state->input[state->index++] << count;
      }
    }
  } else {
    while (count) {
      int segment, segment_len;
      // Read a segment...
      if (state->endianness & BIT_BIG_ENDIAN) {
	if (count >= state->bit_offset) {
	  segment_len = state->bit_offset;
	  state->bit_offset = 8;
	  segment = state->input[state->index] & ((1 << segment_len) - 1);
	  state->index++;
	} else {
	  segment_len = count;
	  state->bit_offset -= count;
	  segment = (state->input[state->index] >> state->bit_offset) & ((1 << segment_len) - 1);
	}
      } else { // BIT_LITTLE_ENDIAN
	if (count + state->bit_offset >= 8) {
	  segment_len = 8 - state->bit_offset;
	  segment = (state->input[state->index] >> state->bit_offset);
	  state->index++;
	  state->bit_offset = 0;
	} else {
	  segment_len = count;
	  segment = (state->input[state->index] >> state->bit_offset) & ((1 << segment_len) - 1);
	  state->bit_offset += segment_len;
	}
      }
      
      // have a valid segment; time to assemble the byte
      if (state->endianness & BYTE_BIG_ENDIAN) {
	out = out << segment_len | segment;
      } else { // BYTE_LITTLE_ENDIAN
	out |= segment << offset;
	offset += segment_len;
      }
      count -= segment_len;
    }
  }
  out <<= final_shift;
  return (out ^ msb) - msb; // perform sign extension
}

#ifdef INCLUDE_TESTS

#define MK_INPUT_STREAM(buf,len,endianness_)   \
  {					      \
    .input = (uint8_t*)buf,					\
      .length = len,						\
      .index = 0,						\
      .bit_offset = (((endianness_) & BIT_BIG_ENDIAN) ? 8 : 0),	\
      .endianness = endianness_					\
      }

static void test_bitreader_be(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmpint(read_bits(&is, 3, false), ==, 0x03);
  g_check_cmpint(read_bits(&is, 8, false), ==, 0x52);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0x1A);
}
static void test_bitreader_le(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmpint(read_bits(&is, 3, false), ==, 0x02);
  g_check_cmpint(read_bits(&is, 8, false), ==, 0x4D);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0x0B);
}

static void test_largebits_be(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmpint(read_bits(&is, 11, false), ==, 0x352);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0x1A);
}
  
static void test_largebits_le(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmpint(read_bits(&is, 11, false), ==, 0x26A);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0x0B);
}

static void test_offset_largebits_be(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0xD);
  g_check_cmpint(read_bits(&is, 11, false), ==, 0x25A);
}
  
static void test_offset_largebits_le(void) {
  input_stream_t is = MK_INPUT_STREAM("\x6A\x5A", 2, BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN);
  g_check_cmpint(read_bits(&is, 5, false), ==, 0xA);
  g_check_cmpint(read_bits(&is, 11, false), ==, 0x2D3);
}


void register_bitreader_tests(void)  {
  g_test_add_func("/core/bitreader/be", test_bitreader_be);
  g_test_add_func("/core/bitreader/le", test_bitreader_le);
  g_test_add_func("/core/bitreader/largebits-be", test_largebits_be);
  g_test_add_func("/core/bitreader/largebits-le", test_largebits_le);
  g_test_add_func("/core/bitreader/offset-largebits-be", test_offset_largebits_be);
  g_test_add_func("/core/bitreader/offset-largebits-le", test_offset_largebits_le);
}

#endif // #ifdef INCLUDE_TESTS
