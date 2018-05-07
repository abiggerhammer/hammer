#ifndef HAMMER_PLATFORM__H
#define HAMMER_PLATFORM__H

/**
 * @file interface between hammer and the operating system /
 * underlying platform.
 */

#include "compiler_specifics.h"

#include <stdarg.h>
#include <stdint.h>

/* String Formatting */

/** see GNU C asprintf */
int h_platform_asprintf(char **strp, const char *fmt, ...);

/** see GNU C vasprintf */
int h_platform_vasprintf(char **strp, const char *fmt, va_list arg);

/* Error Reporting */

/* BSD errx function, seen in err.h */
H_MSVC_DECLSPEC(noreturn) \
void h_platform_errx(int err, const char* format, ...)	\
  H_GCC_ATTRIBUTE((noreturn, format (printf,2,3)));

/* Time Measurement */

struct HStopWatch; /* forward definition */

/* initialize a stopwatch */
void h_platform_stopwatch_reset(struct HStopWatch* stopwatch);

/* return difference between last reset point and now */
int64_t h_platform_stopwatch_ns(struct HStopWatch* stopwatch);

/* Platform dependent definitions for HStopWatch */
#if defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

struct HStopWatch {
  LARGE_INTEGER qpf;
  LARGE_INTEGER start;
};

#else
/* Unix like platforms */

#include <time.h>

struct HStopWatch {
  struct timespec start;
};

#endif

#endif
