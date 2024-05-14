#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <zero/Types.h>

namespace zero {

enum class LogLevel { Jabber, Debug, Info, Warning, Error, Count };

extern LogLevel g_LogPrintLevel;
extern const char* g_LogPath;

void LogArgs(LogLevel level, const char* fmt, va_list args);

inline void Log(LogLevel level, const char* fmt, ...) {
  va_list args;

  va_start(args, fmt);

  LogArgs(level, fmt, args);

  va_end(args);
}

}  // namespace zero
