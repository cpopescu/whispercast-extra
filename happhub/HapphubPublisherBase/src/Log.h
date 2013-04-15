#ifdef __ANDROID
#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "HAPPHUB/NATIVE", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "HAPPHUB/NATIVE", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "HAPPHUB/NATIVE", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "HAPPHUB/NATIVE", __VA_ARGS__)
#endif

#ifdef __WINDOWS
#pragma once

#include <stdarg.h>

namespace happhub {

enum __WINDOWS_LOG_LEVEL {
  WINDOWS_LOG_VERBOSE,
  WINDOWS_LOG_DEBUG,
  WINDOWS_LOG_WARN,
  WINDOWS_LOG_ERROR
};

void __windows_log_print(__WINDOWS_LOG_LEVEL level, const char* format, ...);
void __windows_log_printv(__WINDOWS_LOG_LEVEL level, const char* format, va_list ap);

} // namespace happhub

#define LOGV(format, ...) happhub::__windows_log_print(WINDOWS_LOG_VERBOSE, format, __VA_ARGS__)
#define LOGW(format, ...) happhub::__windows_log_print(WINDOWS_LOG_WARN, format, __VA_ARGS__)
#define LOGD(format, ...) happhub::__windows_log_print(WINDOWS_LOG_DEBUG, format, __VA_ARGS__)
#define LOGE(format, ...) happhub::__windows_log_print(WINDOWS_LOG_ERROR, format, __VA_ARGS__)
#endif
