#include "internal.h"
#include "hammer.h"
#include "allocator.h"
#include <assert.h>
#include <malloc.h>
// {{{ counted arrays


counted_array_t *carray_new_sized(arena_t arena, size_t size) {
  counted_array_t *ret = arena_malloc(arena, sizeof(counted_array_t));
  assert(size > 0);
  ret->used = 0;
  ret->capacity = size;
  ret->arena = arena;
  ret->elements = arena_malloc(arena, sizeof(void*) * size);
  return ret;
}
counted_array_t *carray_new(arena_t arena) {
  return carray_new_sized(arena, 4);
}

void carray_append(counted_array_t *array, void* item) {
  if (array->used >= array->capacity) {
    void **elements = arena_malloc(array->arena, (array->capacity *= 2) * sizeof(counted_array_t*));
    for (size_t i = 0; i < array->used; i++)
      elements[i] = array->elements[i];
    for (size_t i = array->used; i < array->capacity; i++)
      elements[i] = 0;
    array->elements = elements;
  }
  array->elements[array->used++] = item;
}
