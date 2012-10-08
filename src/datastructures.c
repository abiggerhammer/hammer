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

// HSlist
HSlist* h_slist_new(HArena *arena) {
  HSlist *ret = h_arena_malloc(arena, sizeof(HSlist));
  ret->head = NULL;
  ret->arena = arena;
  return ret;
}

void* h_slist_pop(HSlist *slist) {
  HSlistNode *head = slist->head;
  if (!head)
    return NULL;
  void* ret = head->elem;
  slist->head = head->next;
  h_arena_free(slist->arena, head);
  return ret;
}

void h_slist_push(HSlist *slist, void* item) {
  HSlistNode *hnode = h_arena_malloc(slist->arena, sizeof(HSlistNode));
  hnode->elem = item;
  hnode->next = slist->head;
  // write memory barrier here.
  slist->head = hnode;
}

bool h_slist_find(HSlist *slist, const void* item) {
  assert (item != NULL);
  HSlistNode *head = slist->head;
  while (head != NULL) {
    if (head->elem == item)
      return true;
    head = head->next;
  }
  return false;
}

HSlist* h_slist_remove_all(HSlist *slist, const void* item) {
  assert (item != NULL);
  HSlistNode *node = slist->head;
  HSlistNode *prev = NULL;
  while (node != NULL) {
    if (node->elem == item) {
      HSlistNode *next = node->next;
      if (prev)
	prev->next = next;
      else
	slist->head = next;
      // FIXME free the removed node! this leaks.
      node = next;
    }
    else {
      prev = node;
      node = prev->next;
    }
  }
  return slist;
}

void h_slist_free(HSlist *slist) {
  while (slist->head != NULL)
    h_slist_pop(slist);
  h_arena_free(slist->arena, slist);
}

