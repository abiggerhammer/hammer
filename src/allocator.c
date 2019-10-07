/* Arena allocator for Hammer.
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

#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include "hammer.h"
#include "internal.h"


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

struct HArena_ {
  struct arena_link *head;
  struct HAllocator_ *mm__;
  size_t block_size;
  size_t used;
  size_t wasted;
};

HArena *h_new_arena(HAllocator* mm__, size_t block_size) {
  if (block_size == 0)
    block_size = 4096;
  struct HArena_ *ret = h_new(struct HArena_, 1);
  struct arena_link *link = (struct arena_link*)mm__->alloc(mm__, sizeof(struct arena_link) + block_size);
  if (!link) {
    // TODO: error-reporting -- let user know that arena link couldn't be allocated
    return NULL;
  }
  memset(link, 0, sizeof(struct arena_link) + block_size);
  link->free = block_size;
  link->used = 0;
  link->next = NULL;
  ret->head = link;
  ret->block_size = block_size;
  ret->used = 0;
  ret->mm__ = mm__;
  ret->wasted = sizeof(struct arena_link) + sizeof(struct HArena_) + block_size;
  return ret;
}

void* h_arena_malloc(HArena *arena, size_t size) {
  if (size <= arena->head->free) {
    // fast path..
    void* ret = arena->head->rest + arena->head->used;
    arena->used += size;
    arena->wasted -= size;
    arena->head->used += size;
    arena->head->free -= size;
    return ret;
  } else if (size > arena->block_size) {
    // We need a new, dedicated block for it, because it won't fit in a standard sized one.
    // This involves some annoying casting...
    arena->used += size;
    arena->wasted += sizeof(struct arena_link*);
    void* link = arena->mm__->alloc(arena->mm__, size + sizeof(struct arena_link*));
    if (!link) {
      // TODO: error-reporting -- let user know that arena link couldn't be allocated
      return NULL;
    }
    memset(link, 0, size + sizeof(struct arena_link*));
    *(struct arena_link**)link = arena->head->next;
    arena->head->next = (struct arena_link*)link;
    return (void*)(((uint8_t*)link) + sizeof(struct arena_link*));
  } else {
    // we just need to allocate an ordinary new block.
    struct arena_link *link = (struct arena_link*)arena->mm__->alloc(arena->mm__, sizeof(struct arena_link) + arena->block_size);
    if (!link) {
      // TODO: error-reporting -- let user know that arena link couldn't be allocated
      return NULL;
    }
    memset(link, 0, sizeof(struct arena_link) + arena->block_size);
    link->free = arena->block_size - size;
    link->used = size;
    link->next = arena->head;
    arena->head = link;
    arena->used += size;
    arena->wasted += sizeof(struct arena_link) + arena->block_size - size;
    return link->rest;
  }
}

void h_arena_free(HArena *arena, void* ptr) {
  // To be used later...
}

void h_delete_arena(HArena *arena) {
  HAllocator *mm__ = arena->mm__;
  struct arena_link *link = arena->head;
  while (link) {
    struct arena_link *next = link->next; 
    // Even in the case of a special block, without the full arena
    // header, this is correct, because the next pointer is the first
    // in the structure.
    h_free(link);
    link = next;
  }
  h_free(arena);
}

void h_allocator_stats(HArena *arena, HArenaStats *stats) {
  stats->used = arena->used;
  stats->wasted = arena->wasted;
}
