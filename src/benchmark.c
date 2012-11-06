#include <stdio.h>
#include <time.h>
#include <string.h>
#include "hammer.h"

/*
  Usage:
  Create your parser (i.e., HParser*), and then call

  HBenchmarkResults* results = h_benchmark(parser, testcases);

  Then, you can format a report with:

  h_benchmark_report(stdout, results);

  or just generate code to make the parser run as fast as possible with:

  h_benchmark_dump_optimized_code(stdout, results);

*/


HBenchmarkResults *h_benchmark(const HParser* parser, HParserTestcase* testcases) {
  // For now, just output the results to stderr
  HParserTestcase* tc = testcases;
  HParserBackend backend = PB_MIN;

  for (backend = PB_MIN; backend < PB_MAX; backend++) {
    fprintf(stderr, "Compiling for backend %d ... ", backend);
    // Step 1: Compile grammar for given parser...
    if (h_compile(parser, PB_MIN, NULL) == -1) {
      // backend inappropriate for grammar...
      fprintf(stderr, "failed\n");
      continue;
    }
    int tc_failed = 0;
    // Step 1: verify all test cases.
    for (tc = testcases; tc->input != NULL; tc++) {
      HParseResult *res = h_parse(parser, tc->input, tc->length);
      char* res_unamb;
      if (res != NULL) {
	res_unamb = h_write_result_unamb(res->ast);
      } else
	res_unamb = NULL;
      if ((res_unamb == NULL && tc->output_unambiguous == NULL)
	  || (strcmp(res_unamb, tc->output_unambiguous) != 0)) {
	// test case failed...
	fprintf(stderr, "failed\n");
	// We want to run all testcases, for purposes of generating a
	// report. (eg, if users are trying to fix a grammar for a
	// faster backend)
	tc_failed++;
      }
      h_parse_result_free(res);
    }

    if (tc_failed > 0) {
      // Can't use this parser; skip to the next
      fprintf(stderr, "Backend failed testcases; skipping benchmark\n");
      continue;
    }

    for (tc = testcases; tc->input != NULL; tc++) {
      // The goal is to run each testcase for at least 50ms each
      // TODO: replace this with a posix timer-based benchmark. (cf. timerfd_create, timer_create, setitimer)
      int count = 1, cur;
      struct timespec ts_start, ts_end;
      long long time_diff;
      do {
	count *= 2; // Yes, this means that the first run will run the function twice. This is fine, as we want multiple runs anyway.
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	for (cur = 0; cur < count; cur++) {
	  h_parse_result_free(h_parse(parser, tc->input, tc->length));
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);
	
	// time_diff is in ns
	time_diff = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000 + (ts_end.tv_nsec - ts_start.tv_nsec);
      } while (time_diff < 100000000);
      fprintf(stderr, "Case %d: %lld ns/parse\n", (int)(tc - testcases),  time_diff / count);
    }
  }
  return NULL;
}

void h_benchmark_report(FILE* stream, HBenchmarkResults* result) {
  // TODO: fill in this function
}
