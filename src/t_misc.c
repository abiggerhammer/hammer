#include <glib.h>
#include <string.h>
#include "test_suite.h"
#include "hammer.h"
#include "internal.h"
#include "glue.h"

static void test_tt_user(void) {
  g_check_cmp_int32(TT_USER, >, TT_NONE);
  g_check_cmp_int32(TT_USER, >, TT_BYTES);
  g_check_cmp_int32(TT_USER, >, TT_SINT);
  g_check_cmp_int32(TT_USER, >, TT_UINT);
  g_check_cmp_int32(TT_USER, >, TT_SEQUENCE);
  g_check_cmp_int32(TT_USER, >, TT_ERR);
}

static void test_tt_registry(void) {
  int id = h_allocate_token_type("com.upstandinghackers.test.token_type");
  g_check_cmp_int32(id, >=, TT_USER);
  int id2 = h_allocate_token_type("com.upstandinghackers.test.token_type_2");
  g_check_cmp_int32(id2, !=, id);
  g_check_cmp_int32(id2, >=, TT_USER);
  g_check_cmp_int32(id, ==, h_get_token_type_number("com.upstandinghackers.test.token_type"));
  g_check_cmp_int32(id2, ==, h_get_token_type_number("com.upstandinghackers.test.token_type_2"));
  g_check_string("com.upstandinghackers.test.token_type", ==, h_get_token_type_name(id));
  g_check_string("com.upstandinghackers.test.token_type_2", ==, h_get_token_type_name(id2));
  if (h_get_token_type_name(0) != NULL) {
    g_test_message("Unknown token type should not return a name");
    g_test_fail();
  }
  g_check_cmp_int32(h_get_token_type_number("com.upstandinghackers.test.unkown_token_type"), ==, 0);
}

static void test_seq_index_path(void) {
  HArena *arena = h_new_arena(&system_allocator, 0);

  HParsedToken *seq = h_make_seqn(arena, 1);
  HParsedToken *seq2 = h_make_seqn(arena, 2);
  HParsedToken *tok1 = h_make_uint(arena, 41);
  HParsedToken *tok2 = h_make_uint(arena, 42);

  seq->seq->elements[0] = seq2;
  seq->seq->used = 1;
  seq2->seq->elements[0] = tok1;
  seq2->seq->elements[1] = tok2;
  seq2->seq->used = 2;

  g_check_cmp_int(h_seq_index_path(seq, 0, -1)->token_type, ==, TT_SEQUENCE);
  g_check_cmp_int(h_seq_index_path(seq, 0, 0, -1)->token_type, ==, TT_UINT);
  g_check_cmp_int64(h_seq_index_path(seq, 0, 0, -1)->uint, ==, 41);
  g_check_cmp_int64(h_seq_index_path(seq, 0, 1, -1)->uint, ==, 42);
}

void register_misc_tests(void) {
  g_test_add_func("/core/misc/tt_user", test_tt_user);
  g_test_add_func("/core/misc/tt_registry", test_tt_registry);
  g_test_add_func("/core/misc/seq_index_path", test_seq_index_path);
}
