// At this point, this is just a compile/link test.
#include "hammer.h"

HParserTestcase testcases[] = {
  {(unsigned char*)"1,2,3", 5, "(u0x31 u0x32 u0x33)"},
  {(unsigned char*)"1,3,2", 5, "(u0x31 u0x33 u0x32)"},
  {(unsigned char*)"1,3", 3, "(u0x31 u0x33)"},
  {(unsigned char*)"3", 1, "(u0x33)"},
  { NULL, 0, NULL }
};

void test_benchmark_1() {
  const HParser *parser = h_sepBy1(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(',')); 

  HBenchmarkResults *res = h_benchmark(parser, testcases);
  h_benchmark_report(stderr, res);
}

int main(int argc, char **argv) {
  test_benchmark_1();
  return 0;
}
