#include "Log.h"

#ifdef __WINDOWS
#include <Windows.h>
#include <strsafe.h>

namespace happhub {

static
const char* __windows_log_levels[] = {
    "VERBOSE: ",
    "DEBUG:   ",
    "WARNING: ",
    "ERROR:   "
};
void __windows_log_print(__WINDOWS_LOG_LEVEL level, const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  __windows_log_printv(level, format, ap);
}
void __windows_log_printv(__WINDOWS_LOG_LEVEL level, const char* format, va_list ap) {
  char buffer[4096];
  ::StringCchCopyA(buffer, sizeof(buffer), __windows_log_levels[level]);

  HRESULT hr = ::StringCbVPrintfA(buffer+9, sizeof(buffer)-12, format, ap);
  if (STRSAFE_E_INSUFFICIENT_BUFFER == hr || S_OK == hr) {
    ::StringCchCatA(buffer, sizeof(buffer), "\r\n");
    ::OutputDebugStringA(buffer);
  }
}

} //namespace happhub
#endif