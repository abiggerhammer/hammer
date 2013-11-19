/* Bit-parsing operations for Hammer.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include "internal.h"
#include "hammer.h"
#include "test_suite.h"

#define LSB(range) (0:range)
#define MSB(range) (1:range)
#define LDB(range,i) (((i)>>LSB(range))&((1<<(MSB(range)-LSB(range)+1))-1))


int64_t h_read_bits(HInputStream* state, int count, char signed_p) {
  // BUG: Does not 
  int64_t out = 0;
  int offset = 0;
  int final_shift = 0;
  int64_t msb = ((signed_p ? 1LL:0) << (count - 1)); // 0 if unsigned, else 1 << (nbits - 1)
  
  
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
