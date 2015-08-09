#ifndef HAMMER_PLATFORM__H
#define HAMMER_PLATFORM__H

/**
 * @file interface between hammer and the operating system /
 * underlying platform.
 */

#include "compiler_specifics.h"

/* Error Reporting */

/* BSD errx function, seen in err.h */
H_MSVC_DECLSPEC(noreturn) \
void h_platform_errx(int err, const char* format, ...)	\
  H_GCC_ATTRIBUTE((noreturn, format (printf,2,3)));

#endif
