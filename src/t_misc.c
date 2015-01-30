#include <glib.h>
#include <string.h>
#include "test_suite.h"
#include "hammer.h"

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

void register_misc_tests(void) {
  g_test_add_func("/core/misc/tt_user", test_tt_user);
  g_test_add_func("/core/misc/tt_registry", test_tt_registry);
}
