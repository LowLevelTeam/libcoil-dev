/**
* @file err.cpp
* @brief Implementation of COIL error handling
*/

#include "coil/err.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

namespace coil {

// Default error callback implementation
namespace {
  void defaultErrorCallback(ErrorLevel level, const char* message,
                            const ErrorPosition* position, void* user_data) {

      (void)user_data;
      const char* level_str = errorLevelToString(level);
      
      if (position) {
          if (position->line > 0) {
              fprintf(stderr, "COIL %s: %s:%zu: %s\n",
                      level_str, position->file, position->line, message);
          } else {
              fprintf(stderr, "COIL %s: %s:%zu: %s\n",
                      level_str, position->file, position->index, message);
          }
      } else {
          fprintf(stderr, "COIL %s: %s\n", level_str, message);
      }
      
      // Abort on fatal errors
      if (level == ErrorLevel::Fatal) {
          fprintf(stderr, "Fatal error: aborting\n");
          abort();
      }
  }
  
  // Global error callback and user data
  ErrorCallback g_error_callback = defaultErrorCallback;
  void* g_user_data = nullptr;
  
  // Buffer for error messages
  constexpr size_t ERROR_BUFFER_SIZE = 1024;
  char g_error_buffer[ERROR_BUFFER_SIZE];
}

void setErrorCallback(ErrorCallback callback, void* user_data) {
  if (callback) {
      g_error_callback = callback;
      g_user_data = user_data;
  } else {
      g_error_callback = defaultErrorCallback;
      g_user_data = nullptr;
  }
}

ErrorCallback getErrorCallback(void** user_data) {
  if (user_data) {
      *user_data = g_user_data;
  }
  return g_error_callback;
}

void reportErrorV(ErrorLevel level, const char* format, va_list args) {
  // Format the error message
  vsnprintf(g_error_buffer, ERROR_BUFFER_SIZE, format, args);
  g_error_buffer[ERROR_BUFFER_SIZE - 1] = '\0';
  
  // Call the error callback with no position information
  g_error_callback(level, g_error_buffer, nullptr, g_user_data);
}

void reportErrorWithPosV(ErrorLevel level, const ErrorPosition* position,
                        const char* format, va_list args) {
  // Format the error message
  vsnprintf(g_error_buffer, ERROR_BUFFER_SIZE, format, args);
  g_error_buffer[ERROR_BUFFER_SIZE - 1] = '\0';
  
  // Call the error callback with position information
  g_error_callback(level, g_error_buffer, position, g_user_data);
}

void reportError(ErrorLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  reportErrorV(level, format, args);
  va_end(args);
}

void reportErrorWithPos(ErrorLevel level, const ErrorPosition* position,
                      const char* format, ...) {
  va_list args;
  va_start(args, format);
  reportErrorWithPosV(level, position, format, args);
  va_end(args);
}

Result makeError(Result code, ErrorLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  reportErrorV(level, format, args);
  va_end(args);
  
  return code;
}

const char* resultToString(Result result) {
  switch (result) {
      case Result::Success:
          return "Success";
      case Result::InvalidArg:
          return "Invalid Argument";
      case Result::OutOfMemory:
          return "Out of Memory";
      case Result::IoError:
          return "I/O Error";
      case Result::InvalidFormat:
          return "Invalid Format";
      case Result::NotFound:
          return "Not Found";
      case Result::NotSupported:
          return "Not Supported";
      default:
          return "Unknown Error";
  }
}

const char* errorLevelToString(ErrorLevel level) {
  switch (level) {
      case ErrorLevel::Info:
          return "Info";
      case ErrorLevel::Warning:
          return "Warning";
      case ErrorLevel::Error:
          return "Error";
      case ErrorLevel::Fatal:
          return "Fatal";
      default:
          return "Unknown";
  }
}

} // namespace coil