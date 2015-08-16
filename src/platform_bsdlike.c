#define _GNU_SOURCE // to obtain asprintf/vasprintf
#include "platform.h"

#include <stdio.h>

#include <err.h>
#include <stdarg.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifdef __NetBSD__
#include <sys/resource.h>
#endif

int h_platform_asprintf(char **strp, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int res = h_platform_vasprintf(strp, fmt, ap);
  va_end(ap);
  return res;
}

int h_platform_vasprintf(char **strp, const char *fmt, va_list arg)
{
  return vasprintf(strp, fmt, arg);
}

void h_platform_errx(int err, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  verrx(err, format, ap);
}

// TODO: replace this with a posix timer-based benchmark. (cf. timerfd_create, timer_create, setitimer)

static void gettime(struct timespec *ts) {
  if (ts == NULL)
    return;
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  /*
   * This returns real time, not CPU time. See http://stackoverflow.com/a/6725161
   * Possible solution: http://stackoverflow.com/a/11659289
   */
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#elif defined(__NetBSD__)
  // NetBSD doesn't have CLOCK_THREAD_CPUTIME_ID. We'll use getrusage instead
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
  ts->tv_nsec = (rusage.ru_utime.tv_usec + rusage.ru_stime.tv_usec) * 1000;
  // not going to overflow; can be at most 2e9-2
  ts->tv_sec = rusage.ru_utime.tv_sec + rusage.ru_utime.tv_sec;
  if (ts->tv_nsec >= 1000000000) {
    ts->tv_nsec -=   1000000000; // subtract a second
    ts->tv_sec += 1; // add it back.
  }
  assert (ts->tv_nsec <= 1000000000);
#else
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
#endif
}

void h_platform_stopwatch_reset(struct HStopWatch* stopwatch) {
  gettime(&stopwatch->start);
}

int64_t h_platform_stopwatch_ns(struct HStopWatch* stopwatch) {
  struct timespec ts_now;
  gettime(&ts_now);

  // time_diff is in ns
  return (ts_now.tv_sec - stopwatch->start.tv_sec) * 1000000000
          + (ts_now.tv_nsec - stopwatch->start.tv_nsec);
}
