/* Parser combinators for binary formats.
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

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "allocator.h"
#include "parsers/parser_internal.h"

static HParserBackendVTable *backends[PB_MAX + 1] = {
  &h__packrat_backend_vtable,
  &h__regex_backend_vtable,
  &h__llk_backend_vtable,
  &h__lalr_backend_vtable,
  &h__glr_backend_vtable,
};


/* Helper function, since these lines appear in every parser */

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;



#define DEFAULT_ENDIANNESS (BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN)

HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length) {
  return h_parse__m(&system_allocator, parser, input, length);
}
HParseResult* h_parse__m(HAllocator* mm__, const HParser* parser, const uint8_t* input, size_t length) {
  // Set up a parse state...
  HInputStream input_stream = {
    .pos = 0,
    .index = 0,
    .bit_offset = 0,
    .overrun = 0,
    .endianness = DEFAULT_ENDIANNESS,
    .length = length,
    .input = input,
    .last_chunk = true
  };
  
  return backends[parser->backend]->parse(mm__, parser, &input_stream);
}

void h_parse_result_free__m(HAllocator *alloc, HParseResult *result) {
  h_parse_result_free(result);
}

void h_parse_result_free(HParseResult *result) {
  if(result == NULL) return;
  h_delete_arena(result->arena);
}

bool h_false(void* env) {
  (void)env;
  return false;
}

bool h_true(void* env) {
  (void)env;
  return true;
}

bool h_not_regular(HRVMProg *prog, void *env) {
  (void)env;
  return false;
}

int h_compile(HParser* parser, HParserBackend backend, const void* params) {
  return h_compile__m(&system_allocator, parser, backend, params);
}

int h_compile__m(HAllocator* mm__, HParser* parser, HParserBackend backend, const void* params) {
  backends[parser->backend]->free(parser);
  int ret = backends[backend]->compile(mm__, parser, params);
  if (!ret)
    parser->backend = backend;
  return ret;
}


HSuspendedParser* h_parse_start(const HParser* parser) {
  return h_parse_start__m(&system_allocator, parser);
}
HSuspendedParser* h_parse_start__m(HAllocator* mm__, const HParser* parser) {
  if(!backends[parser->backend]->parse_start)
    return NULL;

  // allocate and init suspended state
  HSuspendedParser *s = h_new(HSuspendedParser, 1);
  if(!s)
    return NULL;
  s->mm__ = mm__;
  s->parser = parser;
  s->backend_state = NULL;
  s->done = false;
  s->pos = 0;
  s->bit_offset = 0;
  s->endianness = DEFAULT_ENDIANNESS;

  // backend-specific initialization
  // should allocate s->backend_state
  backends[parser->backend]->parse_start(s);

  return s;
}

bool h_parse_chunk(HSuspendedParser* s, const uint8_t* input, size_t length) {
  assert(backends[s->parser->backend]->parse_chunk != NULL);

  // no-op if parser is already done
  if(s->done)
    return true;

  // input 
  HInputStream input_stream = {
    .pos = s->pos,
    .index = 0,
    .bit_offset = 0,
    .overrun = 0,
    .endianness = s->endianness,
    .length = length,
    .input = input,
    .last_chunk = false
  };

  // process chunk
  s->done = backends[s->parser->backend]->parse_chunk(s, &input_stream);
  s->endianness = input_stream.endianness;
  s->pos += input_stream.index;
  s->bit_offset = input_stream.bit_offset;

  return s->done;
}

HParseResult* h_parse_finish(HSuspendedParser* s) {
  assert(backends[s->parser->backend]->parse_chunk != NULL);
  assert(backends[s->parser->backend]->parse_finish != NULL);

  HAllocator *mm__ = s->mm__;

  // signal end of input if parser is not already done
  if(!s->done) {
    HInputStream empty = {
      .pos = s->pos,
      .index = 0,
      .bit_offset = 0,
      .overrun = 0,
      .endianness = s->endianness,
      .length = 0,
      .input = NULL,
      .last_chunk = true
    };

    s->done = backends[s->parser->backend]->parse_chunk(s, &empty);
    assert(s->done);
  }

  // extract result
  HParseResult *r = backends[s->parser->backend]->parse_finish(s);
  if(r)
    r->bit_length = s->pos * 8 + s->bit_offset;

  // NB: backend should have freed backend_state
  h_free(s);

  return r;
}
