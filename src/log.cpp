#include "coil/log.hpp"
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <unistd.h>

namespace coil {

Logger::Logger(const char* prefix, FILE* stream, LogLevel level, bool useColors)
  : stream(stream)
  , level(level)
  , coloredOutput(useColors && (stream ? isatty(fileno(stream)) : false)) {
  // Copy the prefix to the fixed size buffer
  if (prefix) {
      strncpy(this->prefix, prefix, sizeof(this->prefix) - 1);
      this->prefix[sizeof(this->prefix) - 1] = '\0'; // Ensure null termination
  } else {
      strcpy(this->prefix, "COIL");
  }
}

void Logger::setLevel(LogLevel newLevel) {
  level = newLevel;
}

void Logger::setColoredOutput(bool enabled) {
  coloredOutput = enabled;
}

LogLevel Logger::getLevel() const {
  return level;
}

bool Logger::isLevelEnabled(LogLevel checkLevel) const {
  return checkLevel >= level;
}

void Logger::log(LogLevel logLevel, const char* file, int line, const char* func, const char* fmt, ...) {
  if (logLevel < level || !stream) return;
  
  char timestamp[32];
  std::time_t now = std::time(nullptr);
  std::tm* tm_info = std::localtime(&now);
  std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
  
  // Print timestamp and level
  if (coloredOutput) {
      fprintf(stream, "%s [%s%s%s] [%s] ", 
              timestamp, 
              detail::LOG_LEVEL_COLORS[static_cast<int>(logLevel)], 
              detail::LOG_LEVEL_NAMES[static_cast<int>(logLevel)], 
              detail::RESET_COLOR,
              prefix);
  } else {
      fprintf(stream, "%s [%s] [%s] ", 
              timestamp, 
              detail::LOG_LEVEL_NAMES[static_cast<int>(logLevel)],
              prefix);
  }
  
  // Print source location for debug and higher
  if (logLevel >= LogLevel::Debug) {
      fprintf(stream, "(%s:%d:%s) ", file, line, func);
  }
  
  // Print the actual message using va_args
  va_list args;
  va_start(args, fmt);
  vfprintf(stream, fmt, args);
  va_end(args);
  
  fprintf(stream, "\n");
  fflush(stream);
}

} // namespace coil