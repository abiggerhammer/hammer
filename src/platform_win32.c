#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int h_platform_asprintf(char**strp, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int res = h_platform_vasprintf(strp, fmt, ap);
  va_end(ap);
  return res;
}

int h_platform_vasprintf(char**strp, const char *fmt, va_list args)
{
  va_list ap;
  va_copy(ap, args);
  int non_null_char_count = _vscprintf(fmt, ap);
  va_end(ap);

  if (non_null_char_count < 0) {
    return -1;
  }

  size_t buffer_size = 1 + non_null_char_count;
  char* buffer = malloc(buffer_size);

  va_copy(ap, args);
  int res = vsnprintf_s(buffer, buffer_size, non_null_char_count, fmt, ap);
  if (res < 0) {
    free(buffer);
  } else {
    buffer[non_null_char_count] = 0;
    *strp = buffer;
  }
  va_end(ap);

  return res;
}

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
