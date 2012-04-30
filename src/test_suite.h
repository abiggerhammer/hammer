#ifndef HAMMER_TEST_SUITE__H
#define HAMMER_TEST_SUITE__H

// Equivalent to g_assert_*, but not using g_assert...
#define g_check_cmpint(n1, op, n2) {			\
  typeof (n1) _n1 = (n1);				\
  typeof (n2) _n2 = (n2);				\
  if (!(_n1 op _n2)) {					\
    g_test_message("Check failed: (%s): (%lld %s %d)",	\
		   #n1 " " #op " " #n2,			\
		   _n1, #op, _n2);			\
    g_test_fail();					\
  }							\
  }


#endif // #ifndef HAMMER_TEST_SUITE__H
