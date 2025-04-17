/**
 * @file err.hpp
 * @brief Error handling for COIL using modern C++ exceptions
 */

#pragma once
#include "coil/types.hpp"
#include <string>
#include <functional>
#include <sstream>
#include <iostream>

namespace coil {

/**
 * @brief Error severity levels
 */
enum class ErrorLevel {
  Info,       // Informational message
  Warning,    // Warning message
  Error,      // Error message
  Fatal       // Fatal error message
};

/**
 * @brief Error position information
 */
struct ErrorPosition {
  std::string file;  // Source file
  size_t line;       // Line number
  size_t index;      // Byte position in file
  
  // Create an error position
  static ErrorPosition current(const char* file, size_t line, size_t index = 0) {
    return { file, line, index };
  }
};

/**
 * @brief Convenience macro for current position
 */
#define COIL_CURRENT_POS coil::ErrorPosition::current(__FILE__, __LINE__)

/**
 * @brief Modern logger class for COIL errors and messages
 */
class Logger {
public:
  using Callback = std::function<void(ErrorLevel, const std::string&, const ErrorPosition*)>;
  
  /**
   * @brief Set callback for error handling
   */
  static void setCallback(Callback callback) {
    s_callback = std::move(callback);
  }
  
  /**
   * @brief Get current callback
   */
  static Callback getCallback() {
    return s_callback;
  }
  
  /**
   * @brief Log a message
   */
  static void log(ErrorLevel level, const std::string& message, const ErrorPosition* position = nullptr) {
    if (s_callback) {
      s_callback(level, message, position);
    } else {
      defaultLog(level, message, position);
    }
    
    // Throw exception for errors and fatal errors
    if (level == ErrorLevel::Error) {
      throw CoilException(message);
    } else if (level == ErrorLevel::Fatal) {
      throw CoilException("FATAL: " + message);
    }
  }
  
  /**
   * @brief Log with position information
   */
  static void log(ErrorLevel level, const ErrorPosition& position, const std::string& message) {
    log(level, message, &position);
  }
  
  /**
   * @brief Log an informational message
   */
  static void info(const std::string& message, const ErrorPosition& position = COIL_CURRENT_POS) {
    log(ErrorLevel::Info, message, &position);
  }
  
  /**
   * @brief Log a warning message
   */
  static void warning(const std::string& message, const ErrorPosition& position = COIL_CURRENT_POS) {
    log(ErrorLevel::Warning, message, &position);
  }
  
  /**
   * @brief Log an error message and throw exception
   */
  static void error(const std::string& message, const ErrorPosition& position = COIL_CURRENT_POS) {
    log(ErrorLevel::Error, message, &position);
  }
  
  /**
   * @brief Log a fatal error message and throw exception
   */
  static void fatal(const std::string& message, const ErrorPosition& position = COIL_CURRENT_POS) {
    log(ErrorLevel::Fatal, message, &position);
  }
  
private:
  static inline Callback s_callback;
  
  /**
   * @brief Default logging implementation
   */
  static void defaultLog(ErrorLevel level, const std::string& message, const ErrorPosition* position) {
    std::ostringstream ss;
    ss << "COIL " << toString(level) << ": ";
    
    if (position) {
      if (position->line > 0) {
        ss << position->file << ":" << position->line << ": ";
      } else {
        ss << position->file << ":" << position->index << ": ";
      }
    }
    
    ss << message;
    std::cerr << ss.str() << std::endl;
    
    // Abort on fatal errors (in addition to throwing)
    if (level == ErrorLevel::Fatal) {
      std::cerr << "Fatal error: aborting" << std::endl;
      std::abort();
    }
  }
  
  /**
   * @brief Convert ErrorLevel to string
   */
  static std::string toString(ErrorLevel level) {
    switch (level) {
      case ErrorLevel::Info:    return "Info";
      case ErrorLevel::Warning: return "Warning";
      case ErrorLevel::Error:   return "Error";
      case ErrorLevel::Fatal:   return "Fatal";
      default:                  return "Unknown";
    }
  }
};

/**
 * @brief Convenience macros for logging
 */
#define COIL_INFO(message) coil::Logger::info(message, COIL_CURRENT_POS)
#define COIL_WARNING(message) coil::Logger::warning(message, COIL_CURRENT_POS)
#define COIL_ERROR(message) coil::Logger::error(message, COIL_CURRENT_POS)
#define COIL_FATAL(message) coil::Logger::fatal(message, COIL_CURRENT_POS)

} // namespace coil