#include <string.h>
#include <stdlib.h>
#include "internal.h"

// NOTE(uucidl): undefine to automatically fill up newly allocated block
// with this byte:
// #define DEBUG__MEMFILL 0xFF

#if defined(DEBUG__MEMFILL)
/**
 * Blocks allocated by the system_allocator start with this header.
 * I.e. the user part of the allocation directly follows.
 */
typedef struct HDebugBlockHeader_
{
  size_t size; /** size of the user allocation */
} HDebugBlockHeader;

#define BLOCK_HEADER_SIZE (sizeof(HDebugBlockHeader))
#else
#define BLOCK_HEADER_SIZE (0)
#endif

/**
 * Compute the total size needed for a given allocation size.
 */
static inline size_t block_size(size_t alloc_size) {
  return BLOCK_HEADER_SIZE + alloc_size;
}

/**
 * Obtain the block containing the user pointer `uptr`
 */
static inline void* block_for_user_ptr(void *uptr) {
  return ((char*)uptr) - BLOCK_HEADER_SIZE;
}

/**
 * Obtain the user area of the allocation from a given block
 */
static inline void* user_ptr(void *block) {
  return ((char*)block) + BLOCK_HEADER_SIZE;
}

static void* system_alloc(HAllocator *allocator, size_t size) {
  void *block = malloc(block_size(size));
  if (!block) {
    return NULL;
  }
  void *uptr = user_ptr(block);
#ifdef DEBUG__MEMFILL
  memset(uptr, DEBUG__MEMFILL, size);
  ((HDebugBlockHeader*)block)->size = size;
#endif
  return uptr;
}

static void* system_realloc(HAllocator *allocator, void* uptr, size_t size) {
  if (!uptr) {
    return system_alloc(allocator, size);
  }
  void* block = realloc(block_for_user_ptr(uptr), block_size(size));
  if (!block) {
    return NULL;
  }
  uptr = user_ptr(block);

#ifdef DEBUG__MEMFILL
  size_t old_size = ((HDebugBlockHeader*)block)->size;
  if (size > old_size)
    memset((char*)uptr+old_size, DEBUG__MEMFILL, size - old_size);
  ((HDebugBlockHeader*)block)->size = size;
#endif
  return uptr;
}

static void system_free(HAllocator *allocator, void* uptr) {
  if (uptr) {
    free(block_for_user_ptr(uptr));
  }
}

HAllocator system_allocator = {
  .alloc = system_alloc,
  .realloc = system_realloc,
  .free = system_free,
};
