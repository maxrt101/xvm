#ifndef _XVM_LOG_H_
#define _XVM_LOG_H_ 1

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <string>

namespace xvm {

namespace colors {
constexpr char RESET[]   = "\u001b[0m";
constexpr char BLACK[]   = "\u001b[30m";
constexpr char RED[]     = "\u001b[31m";
constexpr char RED_RED[] = "\u001b[41m";
constexpr char GREEN[]   = "\u001b[32m";
constexpr char YELLOW[]  = "\u001b[33m";
constexpr char BLUE[]    = "\u001b[34m";
constexpr char MAGENTA[] = "\u001b[35m";
constexpr char CYAN[]    = "\u001b[36m";
constexpr char WHITE[]   = "\u001b[37m";
} /* namespace colors */

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

LogLevel stringToLogLevel(const std::string& s);

void vflogf(FILE* dest, LogLevel level, const std::string& format, va_list args);
void vlogf(LogLevel level, const std::string& format, va_list args);
void logf(LogLevel level, const std::string format, ...);

void debug(int debugLevel, const std::string format, ...);
void debug(const std::string format, ...);
void info(const std::string format, ...);
void warning(const std::string format, ...);
void error(const std::string format, ...);
void fatal(const std::string format, ...);

#ifdef XVM_FEATURE_VIDEO
void sdlError(std::string format, ...);
void sdlFatal(std::string format, ...);
#endif /* XVM_FEATURE_VIDEO */

} /* namespace xvm */

#endif