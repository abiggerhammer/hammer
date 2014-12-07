#include <string.h>
#include <stdlib.h>
#include "internal.h"

//#define DEBUG__MEMFILL 0xFF

//static
void* system_alloc(HAllocator *allocator, size_t size) {
  
  void* ptr = malloc(size + sizeof(size_t));
#ifdef DEBUG__MEMFILL
  memset(ptr, DEBUG__MEMFILL, size + sizeof(size_t));
#endif
  *(size_t*)ptr = size;
#ifndef _MSC_VER
  return ptr + sizeof(size_t);
#else
  return (uint8_t*)ptr + sizeof(size_t);
#endif
}

//static 
void* system_realloc(HAllocator *allocator, void* ptr, size_t size) {
  if (ptr == NULL)
    return system_alloc(allocator, size);
#ifndef _MSC_VER
  ptr = realloc(ptr - sizeof(size_t), size + sizeof(size_t));
#else
  ptr = realloc((uint8_t*)ptr - sizeof(size_t), size + sizeof(size_t));
#endif
  *(size_t*)ptr = size;
#ifdef DEBUG__MEMFILL
  size_t old_size = *(size_t*)ptr;
  if (size > old_size)
    memset(ptr+sizeof(size_t)+old_size, DEBUG__MEMFILL, size - old_size);
#endif
#ifndef _MSC_VER
  return ptr + sizeof(size_t);
#else
  return (uint8_t*)ptr + sizeof(size_t);
#endif
}

//static 
void system_free(HAllocator *allocator, void* ptr) {
  if (ptr != NULL)
#ifndef _MSC_VER
    free(ptr - sizeof(size_t));
#else
	free((uint8_t*)ptr - sizeof(size_t));
#endif
}

HAllocator system_allocator = {
  .alloc = &system_alloc,
  .realloc = &system_realloc,
  .free = &system_free,
};
