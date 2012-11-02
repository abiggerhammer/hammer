// At this point, this is just a compile/link test.
#include "hammer.h"

HParserTestcase testcases[] = {
  {NULL, 0, NULL}
};

void test_benchmark_1() {
  HParser *parser = NULL; // TODO: fill this in.

  HBenchmarkResults *res = h_benchmark(parser, testcases);
  h_benchmark_report(stderr, res);

}
