#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include "hammer.h"

typedef struct pp_state {
  int delta;
  int indent_amt;
  int at_bol;
} pp_state_t;

void pprint(const parsed_token_t* tok, int indent, int delta) {
  switch (tok->token_type) {
  case TT_NONE:
    printf("%*snull\n", indent, "");
    break;
  case TT_BYTES:
    if (tok->bytes.len == 0)
      printf("%*s<>\n", indent, "");
    else {
      printf("%*s", indent, "");
      for (size_t i = 0; i < tok->bytes.len; i++) {
	printf("%c%02hhx",
	       (i == 0) ? '<' : '.',
	       tok->bytes.token[i]);
      }
      printf(">\n");
    }
    break;
  case TT_SINT:
    printf("%*su %#lx\n", indent, "", tok->sint);
    break;
  case TT_UINT:
     printf("%*ss %#lx\n", indent, "", tok->uint);
    break;
  case TT_SEQUENCE: {
    GSequenceIter *it;
    printf("%*s[\n", indent, "");
    for (it = g_sequence_get_begin_iter(tok->seq);
	 !g_sequence_iter_is_end(it);
	 it = g_sequence_iter_next(it)) {
      parsed_token_t* subtok = g_sequence_get(it);
      pprint(subtok, indent + delta, delta);
    }
    printf("%*s]\n", indent, "");
  } // TODO: implement this
  default:
    g_assert_not_reached();
  }
}


struct result_buf {
  char* output;
  size_t len;
  size_t capacity;
};

static inline void ensure_capacity(struct result_buf *buf, int amt) {
  while (buf->len + amt >= buf->capacity)
    buf->output = g_realloc(buf->output, buf->capacity *= 2);
}

static inline void append_buf(struct result_buf *buf, const char* input, int len) {
  ensure_capacity(buf, len);
  memcpy(buf->output + buf->len, input, len);
  buf->len += len;
}

static inline void append_buf_c(struct result_buf *buf, char v) {
  ensure_capacity(buf, 1);
  buf->output[buf->len++] = v;
}

static void unamb_sub(const parsed_token_t* tok, struct result_buf *buf) {
  char* tmpbuf;
  int len;
  switch (tok->token_type) {
  case TT_NONE:
    append_buf(buf, "null", 4);
    break;
  case TT_BYTES:
    if (tok->bytes.len == 0)
      append_buf(buf, "<>", 2);
    else {
      for (size_t i = 0; i < tok->bytes.len; i++) {
	const char *HEX = "0123456789ABCDEF";
	append_buf_c(buf, (i == 0) ? '<': '.');
	char c = tok->bytes.token[i];
	append_buf_c(buf, HEX[(c >> 4) & 0xf]);
	append_buf_c(buf, HEX[(c >> 0) & 0xf]);
      }
      append_buf_c(buf, '>');
    }
    break;
  case TT_SINT:
    len = asprintf(&tmpbuf, "u%#lx", tok->sint);
    append_buf(buf, tmpbuf, len);
    break;
  case TT_UINT:
    len = asprintf(&tmpbuf, "s%#lx", tok->uint);
    append_buf(buf, tmpbuf, len);
    break;
  case TT_ERR:
    append_buf(buf, "ERR", 3);
    break;
  case TT_SEQUENCE: {
    GSequenceIter *it;
    int at_begin = 1;
    append_buf_c(buf, '(');
    for (it = g_sequence_get_begin_iter(tok->seq);
	 !g_sequence_iter_is_end(it);
	 it = g_sequence_iter_next(it)) {
      parsed_token_t* subtok = g_sequence_get(it);
      if (at_begin)
	at_begin = 0;
      else
	append_buf_c(buf, ' ');
      unamb_sub(subtok, buf);
    }
    append_buf_c(buf, ')');
  }
    break;
  default:
    fprintf(stderr, "Unexpected token type %d\n", tok->token_type);
    g_assert_not_reached();
  }
}
  

char* write_result_unamb(const parsed_token_t* tok) {
  struct result_buf buf = {
    .output = g_malloc0(16),
    .len = 0,
    .capacity = 16
  };
  unamb_sub(tok, &buf);
  return buf.output;
}
  


