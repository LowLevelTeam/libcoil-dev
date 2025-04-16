/**
* @file err.cpp
* @brief Implementation of COIL error handling
*/

#include "coil/err.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace coil {

// Default error callback implementation
namespace {
  void defaultErrorCallback(ErrorLevel level, const char* message,
                            const ErrorPosition* position, void* user_data) {

      (void)user_data;
      const char* level_str = "Unknown";
      switch (level) {
          case ErrorLevel::Info:
              level_str = "Info";
              break;
          case ErrorLevel::Warning:
              level_str = "Warning";
              break;
          case ErrorLevel::Error:
              level_str = "Error";
              break;
          case ErrorLevel::Fatal:
              level_str = "Fatal";
              break;
      }
      
      if (position) {
          fprintf(stderr, "COIL %s: %s:%zu: %s\n",
                  level_str, position->file, position->index, message);
      } else {
          fprintf(stderr, "COIL %s: %s\n", level_str, message);
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

void reportErrorV(ErrorLevel level, const char* format, va_list args) {
  // Format the error message
  vsnprintf(g_error_buffer, ERROR_BUFFER_SIZE, format, args);
  g_error_buffer[ERROR_BUFFER_SIZE - 1] = '\0';
  
  // Call the error callback with no position information
  g_error_callback(level, g_error_buffer, nullptr, g_user_data);
}

void reportError(ErrorLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  reportErrorV(level, format, args);
  va_end(args);
}

void reportErrorWithPos(ErrorLevel level, const ErrorPosition* position,
                      const char* format, ...) {
  // Format the error message
  va_list args;
  va_start(args, format);
  vsnprintf(g_error_buffer, ERROR_BUFFER_SIZE, format, args);
  va_end(args);
  
  g_error_buffer[ERROR_BUFFER_SIZE - 1] = '\0';
  
  // Call the error callback with position information
  g_error_callback(level, g_error_buffer, position, g_user_data);
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

} // namespace coil