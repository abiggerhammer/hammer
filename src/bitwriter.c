#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// h_bit_writer_
HBitWriter *h_bit_writer_new(HAllocator* mm__) {
  HBitWriter *writer = h_new(HBitWriter, 1);
  memset(writer, 0, sizeof(*writer));
  writer->buf = mm__->alloc(mm__, writer->capacity = 8);
  if (!writer) {
    return NULL;
  }
  memset(writer->buf, 0, writer->capacity);
  writer->mm__ = mm__;
  writer->flags = BYTE_BIG_ENDIAN | BIT_BIG_ENDIAN;
  writer->error = 0;
  
  return writer;
}

/**
 * Ensure there are at least [nbits] bits available at the end of the
 * buffer. If the buffer is expanded, the added bits should be zeroed.
 */
static void h_bit_writer_reserve(HBitWriter* w, size_t nbits) {
  // As is, this may overshoot by a byte, e.g., nbits=9, bit_offset=1.
  // This will assume that the last partial byte is full, and reserve
  // 2 bytes at the end, whereas only one is necessary.
  //
  // That said, this guarantees the postcondition that w->buf[w->index]
  // is valid.

  // Round up to bytes
  int nbytes = (nbits + 7) / 8 + ((w->bit_offset != 0) ? 1 : 0);
  size_t old_capacity = w->capacity;
  while (w->index + nbytes >= w->capacity) {
    w->buf = w->mm__->realloc(w->mm__, w->buf, w->capacity *= 2);
    if (!w->buf) {
      w->error = 1;
      return;
    }
  }

  if (old_capacity != w->capacity)
    memset(w->buf + old_capacity, 0, w->capacity - old_capacity);
}


void h_bit_writer_put(HBitWriter* w, uint64_t data, size_t nbits) {
  assert(nbits > 0); // Less than or equal to zero makes complete nonsense

  // expand size...
  h_bit_writer_reserve(w, nbits);

  while (nbits) {
    size_t count = MIN((size_t)(8 - w->bit_offset), nbits);

    // select the bits to be written from the source
    uint16_t bits;
    if (w->flags & BYTE_BIG_ENDIAN) {
      // read from the top few bits; masking will be done later.
      bits = data >> (nbits - count);
    } else {
      // just copy the bottom byte over.
      bits = data & 0xff;
      data >>= count; // remove the bits that have just been used.
    }
    // mask off the unnecessary bits.
    bits &= (1 << count) - 1;

    // Now, push those bits onto the current byte...
    if (w->flags & BIT_BIG_ENDIAN)
      w->buf[w->index] = (w->buf[w->index] << count) | bits;
    else
      w->buf[w->index] = (w->buf[w->index] | (bits << 8)) >> count;

    // update index and bit_offset.
    w->bit_offset += count;
    if (w->bit_offset == 8) {
      w->bit_offset = 0;
      w->index++;
    }
    nbits -= count;
  }

}


const uint8_t *h_bit_writer_get_buffer(HBitWriter* w, size_t *len) {
  assert (len != NULL);
  assert (w != NULL);
  // Not entirely sure how to handle a non-integral number of bytes... make it an error for now
  assert (w->bit_offset == 0); // BUG: change this to some sane behaviour

  *len = w->index;
  return w->buf;
}

void h_bit_writer_free(HBitWriter* w) {
  HAllocator *mm__ = w->mm__;
  h_free(w->buf);
  h_free(w);
}
