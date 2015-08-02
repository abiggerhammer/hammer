#include <string.h>
#include <stdlib.h>
#include "internal.h"

//#define DEBUG__MEMFILL 0xFF

static void* system_alloc(HAllocator *allocator, size_t size) {
  
  void* ptr = malloc(size + sizeof(size_t));
  if (!ptr) {
    return NULL;
  }
#ifdef DEBUG__MEMFILL
  memset(ptr, DEBUG__MEMFILL, size + sizeof(size_t));
#endif
  *(size_t*)ptr = size;
  return ptr + sizeof(size_t);
}

static void* system_realloc(HAllocator *allocator, void* ptr, size_t size) {
  if (!ptr) {
    return system_alloc(allocator, size);
  }
  ptr = realloc(ptr - sizeof(size_t), size + sizeof(size_t));
  if (!ptr) {
    return NULL;
  }
  *(size_t*)ptr = size;
#ifdef DEBUG__MEMFILL
  size_t old_size = *(size_t*)ptr;
  if (size > old_size)
    memset(ptr+sizeof(size_t)+old_size, DEBUG__MEMFILL, size - old_size);
#endif
  return ptr + sizeof(size_t);
}

static void system_free(HAllocator *allocator, void* ptr) {
  if (ptr) {
    free(ptr - sizeof(size_t));
  }
}

HAllocator system_allocator = {
  .alloc = system_alloc,
  .realloc = system_realloc,
  .free = system_free,
};
