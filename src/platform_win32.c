#include "platform.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

void h_platform_errx(int err, const char* format, ...) {
  // FIXME(windows) TODO(uucidl): to be implemented
  ExitProcess(err);
}

