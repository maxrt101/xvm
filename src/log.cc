#include "log.h"

#define _VLOGF_INTERNAL(level) \
  va_list args; \
  va_start(args, format); \
  vlogf(level, format, args); \
  va_end(args);

xvm::LogLevel xvm::stringToLogLevel(const std::string& s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  if (str == "debug") {
    return LogLevel::DEBUG;
  } else if (str == "info") {
    return LogLevel::INFO;
  } else if (str == "warning") {
    return LogLevel::WARNING;
  } else if (str == "error") {
    return LogLevel::ERROR;
  } else if (str == "fatal") {
    return LogLevel::FATAL;
  }
  return LogLevel::DEBUG;
}

void xvm::vflogf(FILE* dest, LogLevel level, const std::string& format, va_list args) {
  switch (level) {
    case LogLevel::FATAL:   fprintf(dest, "%sFATAL",   colors::RED_RED); break;
    case LogLevel::ERROR:   fprintf(dest, "%sERROR",   colors::RED);     break;
    case LogLevel::WARNING: fprintf(dest, "%sWARNING", colors::YELLOW);  break;
    case LogLevel::INFO:    fprintf(dest, "%sINFO",    colors::CYAN);    break;
    case LogLevel::DEBUG:   fprintf(dest, "%sDEBUG",   colors::BLUE);    break;
    default:                fprintf(dest, "INVALID_LOG_LEVEL");          break;
  }

  fprintf(dest, "\033[0m: ");
  vfprintf(dest, format.c_str(), args);
  fprintf(dest, "\n");
}

void xvm::vlogf(LogLevel level, const std::string& format, va_list args) {
  FILE* dest = stdout;
  if (level > LogLevel::INFO) {
    dest = stderr;
  }

  vflogf(dest, level, format, args);
}

void xvm::logf(LogLevel level, const std::string format, ...) {
  va_list args;
  va_start(args, format);
  vlogf(level, format, args);
  va_end(args);
}

void xvm::debug(const std::string format, ...) {
  _VLOGF_INTERNAL(LogLevel::DEBUG);
}

void xvm::info(const std::string format, ...) {
  _VLOGF_INTERNAL(LogLevel::INFO);
}

void xvm::warning(const std::string format, ...) {
  _VLOGF_INTERNAL(LogLevel::WARNING);
}

void xvm::error(const std::string format, ...) {
  _VLOGF_INTERNAL(LogLevel::ERROR);
}

void xvm::fatal(const std::string format, ...) {
  _VLOGF_INTERNAL(LogLevel::FATAL);
}
