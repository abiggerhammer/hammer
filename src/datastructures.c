#include "internal.h"
#include "hammer.h"
#include "allocator.h"
#include <assert.h>
#include <malloc.h>
// {{{ counted arrays


HCountedArray *h_carray_new_sized(HArena * arena, size_t size) {
  HCountedArray *ret = h_arena_malloc(arena, sizeof(HCountedArray));
  assert(size > 0);
  ret->used = 0;
  ret->capacity = size;
  ret->arena = arena;
  ret->elements = h_arena_malloc(arena, sizeof(void*) * size);
  return ret;
}
HCountedArray *h_carray_new(HArena * arena) {
  return h_carray_new_sized(arena, 4);
}

void h_carray_append(HCountedArray *array, void* item) {
  if (array->used >= array->capacity) {
    HParsedToken **elements = h_arena_malloc(array->arena, (array->capacity *= 2) * sizeof(HCountedArray*));
    for (size_t i = 0; i < array->used; i++)
      elements[i] = array->elements[i];
    for (size_t i = array->used; i < array->capacity; i++)
      elements[i] = 0;
    array->elements = elements;
  }
  array->elements[array->used++] = item;
}
