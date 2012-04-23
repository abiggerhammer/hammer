#include <stdint.h>
#include "internal.h"
#include "hammer.h"

#define LSB(range) (0:range)
#define MSB(range) (1:range)
#define LDB(range,i) (((i)>>LSB(range))&((1<<(MSB(range)-LSB(range)+1))-1))

long long read_bits(parse_state_t* state, int count) {
  long long out = 0;
  int offset = 0;
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
  return out;
}
