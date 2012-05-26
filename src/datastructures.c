#include "internal.h"
#include "hammer.h"
#include "allocator.h"
#include <assert.h>
#include <malloc.h>
// {{{ counted arrays


HCountedArray *carray_new_sized(HArena * arena, size_t size) {
  HCountedArray *ret = arena_malloc(arena, sizeof(HCountedArray));
  assert(size > 0);
  ret->used = 0;
  ret->capacity = size;
  ret->arena = arena;
  ret->elements = arena_malloc(arena, sizeof(void*) * size);
  return ret;
}
HCountedArray *carray_new(HArena * arena) {
  return carray_new_sized(arena, 4);
}

void carray_append(HCountedArray *array, void* item) {
  if (array->used >= array->capacity) {
    HParsedToken **elements = arena_malloc(array->arena, (array->capacity *= 2) * sizeof(HCountedArray*));
    for (size_t i = 0; i < array->used; i++)
      elements[i] = array->elements[i];
    for (size_t i = array->used; i < array->capacity; i++)
      elements[i] = 0;
    array->elements = elements;
  }
  array->elements[array->used++] = item;
}
