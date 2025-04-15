#pragma once

#include <string>
#include <cstdio>

namespace coil {

/**
* @brief Log levels
*/
enum class LogLevel {
  Trace = 0,  ///< Trace level (most verbose)
  Debug,      ///< Debug level
  Info,       ///< Info level
  Warning,    ///< Warning level
  Error,      ///< Error level
  Fatal,      ///< Fatal level
  None        ///< No logging
};

// ANSI color codes - placed at namespace level to avoid recreation
namespace detail {
  static const char* const LOG_LEVEL_COLORS[] = {
      "\x1b[90m",  // TRACE: Bright Black
      "\x1b[36m",  // DEBUG: Cyan
      "\x1b[32m",  // INFO: Green
      "\x1b[33m",  // WARNING: Yellow
      "\x1b[31m",  // ERROR: Red
      "\x1b[35m",  // FATAL: Magenta
      ""           // NONE: No color
  };

  static const char* const LOG_LEVEL_NAMES[] = {
      "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"
  };

  static const char* const RESET_COLOR = "\x1b[0m";
}

/**
* @brief Lightweight logger struct without heap allocations
*/
struct Logger {
  FILE* stream;            // Owned externally
  LogLevel level;
  bool coloredOutput;
  char prefix[32];         // Fixed-size buffer to avoid allocations

  /**
    * @brief Construct a logger with a specific stream
    * 
    * @param prefix Logger name/prefix
    * @param stream Output stream (stdout, file, etc.)
    * @param level Initial log level
    * @param useColors Whether to use ANSI colors for output
    */
  Logger(const char* prefix, FILE* stream, LogLevel level, bool useColors = true);
  
  /**
    * @brief Set the log level
    * 
    * @param newLevel New log level
    */
  void setLevel(LogLevel newLevel);
  
  /**
    * @brief Enable or disable colored output
    * 
    * @param enabled Whether to use ANSI colors
    */
  void setColoredOutput(bool enabled);
  
  /**
    * @brief Log a message
    * 
    * @param level Log level
    * @param file Source file
    * @param line Line number
    * @param func Function name
    * @param fmt Format string
    * @param ... Format arguments
    */
  void log(LogLevel level, const char* file, int line, const char* func, const char* fmt, ...);
  
  /**
    * @brief Get the current log level
    * 
    * @return LogLevel 
    */
  LogLevel getLevel() const;
  
  /**
    * @brief Check if a level is enabled
    * 
    * @param checkLevel Level to check
    * @return true if enabled
    */
  bool isLevelEnabled(LogLevel checkLevel) const;
};

// Convenience macros for logging - compile out trace and debug in release mode
#ifdef NDEBUG
  #define COIL_TRACE(logger, ...) ((void)0)
  #define COIL_DEBUG(logger, ...) ((void)0)
#else
  #define COIL_TRACE(logger, ...) \
      if ((logger)->isLevelEnabled(coil::LogLevel::Trace)) \
          (logger)->log(coil::LogLevel::Trace, __FILE__, __LINE__, __func__, __VA_ARGS__)

  #define COIL_DEBUG(logger, ...) \
      if ((logger)->isLevelEnabled(coil::LogLevel::Debug)) \
          (logger)->log(coil::LogLevel::Debug, __FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

#define COIL_INFO(logger, ...) \
  if ((logger)->isLevelEnabled(coil::LogLevel::Info)) \
      (logger)->log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_WARNING(logger, ...) \
  if ((logger)->isLevelEnabled(coil::LogLevel::Warning)) \
      (logger)->log(coil::LogLevel::Warning, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_ERROR(logger, ...) \
  if ((logger)->isLevelEnabled(coil::LogLevel::Error)) \
      (logger)->log(coil::LogLevel::Error, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_FATAL(logger, ...) \
  if ((logger)->isLevelEnabled(coil::LogLevel::Fatal)) \
      (logger)->log(coil::LogLevel::Fatal, __FILE__, __LINE__, __func__, __VA_ARGS__)

} // namespace coil