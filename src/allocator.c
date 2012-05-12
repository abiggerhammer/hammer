#include <glib.h>
#include <stdint.h>
#include <sys/types.h>

#include "allocator.h"

struct arena_link {
  // TODO:
  // For efficiency, we should probably allocate the arena links in 
  // their own slice, and link to a block directly. That can be
  // implemented later, though, with no change in interface.
  struct arena_link *next; // It is crucial that this be the first item; so that 
                           // any arena link can be casted to struct arena_link**.

  size_t free;
  size_t used;
  uint8_t rest[];
} ;

struct arena {
  struct arena_link *head;
  size_t block_size;
};

arena_t new_arena(size_t block_size) {
  struct arena *ret = g_new(struct arena, 1);
  struct arena_link *link = (struct arena_link*)g_malloc0(sizeof(struct arena_link) + block_size);
  link->free = block_size;
  link->used = 0;
  link->next = NULL;
  ret->head = link;
  ret->block_size = 0;

  return ret;
}

void* arena_malloc(arena_t arena, size_t size) {
  if (size <= arena->head->free) {
    // fast path..
    void* ret = arena->head->rest + arena->head->used;
    arena->head->used += size;
    arena->head->free -= size;
    return ret;
  } else if (size > arena->block_size) {
    // We need a new, dedicated block for it, because it won't fit in a standard sized one.
    // This involves some annoying casting...
    void* link = g_malloc(size + sizeof(struct arena_link*));
    *(struct arena_link**)link = arena->head->next;
    arena->head->next = (struct arena_link*)link;
    return (void*)((uint8_t*)link + sizeof(struct arena_link*));
  } else {
    // we just need to allocate an ordinary new block.
    struct arena_link *link = (struct arena_link*)g_malloc0(sizeof(struct arena_link) + arena->block_size);
    link->free = arena->block_size - size;
    link->used = size;
    link->next = arena->head;
    arena->head = link;
    return link->rest;
  }
}
    
void delete_arena(arena_t arena) {
  struct arena_link *link = arena->head;
  while (link) {
    struct arena_link *next = link->next; 
    // Even in the case of a special block, without the full arena
    // header, this is correct, because the next pointer is the first
    // in the structure.
    g_free(link);
    link = next;
  }
  g_free(arena);
}
