#include "platform.h"

#include <err.h>
#include <stdarg.h>

void h_platform_errx(int err, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  verrx(err, format, ap);
}
