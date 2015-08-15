#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

void h_platform_errx(int err, const char* format, ...) {
  // FIXME(windows) TODO(uucidl): to be implemented
  ExitProcess(err);
}

void h_platform_stopwatch_reset(struct HStopWatch* stopwatch) {
  QueryPerformanceFrequency(&stopwatch->qpf);
  QueryPerformanceCounter(&stopwatch->start);
}

/* return difference between last reset point and now */
int64_t h_platform_stopwatch_ns(struct HStopWatch* stopwatch) {
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);

  return 1000000000 * (now.QuadPart - stopwatch->start.QuadPart) / stopwatch->qpf.QuadPart;
}
