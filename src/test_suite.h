#ifndef HAMMER_TEST_SUITE__H
#define HAMMER_TEST_SUITE__H

// Equivalent to g_assert_*, but not using g_assert...
#define g_check_inttype(fmt, typ, n1, op, n2) {				\
    typ _n1 = (n1);							\
    typ _n2 = (n2);							\
    if (!(_n1 op _n2)) {						\
      g_test_message("Check failed: (%s): (" fmt " %s " fmt ")",	\
		     #n1 " " #op " " #n2,				\
		     _n1, #op, _n2);					\
      g_test_fail();							\
    }									\
  }

#define g_check_bytes(len, n1, op, n2) {	\
    const uint8_t *_n1 = (n1);			\
    uint8_t *_n2 = (n2);			\
    if (!(memcmp(_n1, _n2, len) op 0)) {	\
      g_test_message("Check failed: (%s)",    	\
		     #n1 " " #op " " #n2);	\
      g_test_fail();				\
    }						\
  }

#define g_check_cmpint(n1, op, n2) g_check_inttype("%d", int, n1, op, n2)
#define g_check_cmplong(n1, op, n2) g_check_inttype("%ld", long, n1, op, n2)
#define g_check_cmplonglong(n1, op, n2) g_check_inttype("%lld", long long, n1, op, n2)
#define g_check_cmpuint(n1, op, n2) g_check_inttype("%u", unsigned int, n1, op, n2)
#define g_check_cmpulong(n1, op, n2) g_check_inttype("%lu", unsigned long, n1, op, n2)
#define g_check_cmpulonglong(n1, op, n2) g_check_inttype("%llu", unsigned long long, n1, op, n2)


#endif // #ifndef HAMMER_TEST_SUITE__H
