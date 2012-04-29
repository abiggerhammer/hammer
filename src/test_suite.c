#include "hammer.h"
#include "test_suite.h"

extern void register_bitreader_tests();

int main(int argc, char** argv) {
  g_test_init(&argc, &argv, NULL);

  // register various test suites...
  register_bitreader_tests();

  g_test_run();
}

