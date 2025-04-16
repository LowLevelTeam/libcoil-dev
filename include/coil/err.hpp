/**
 * @file err.hpp
 * @brief Simple error handling for COIL
 */

#pragma once
#include "coil/types.hpp"
#include <cstdarg>
#include <cstddef>

namespace coil {

/**
 * @brief Error severity levels
 */
enum class ErrorLevel {
  Info,       ///< Informational message
  Warning,    ///< Warning message
  Error,      ///< Error message
  Fatal       ///< Fatal error message
};

/**
 * @brief Error position information
 */
struct ErrorPosition {
  const char* file; ///< Source file
  size_t line;      ///< Line number
  size_t index;     ///< Byte Position in File
};

/**
 * @brief Callback for error handling
 */
using ErrorCallback = void (*)(ErrorLevel level, const char* message, 
                               const ErrorPosition* position, void* user_data);

/**
 * @brief Set the global error callback
 */
void setErrorCallback(ErrorCallback callback, void* user_data);

/**
 * @brief Get the current error callback and user data
 */
ErrorCallback getErrorCallback(void** user_data = nullptr);

/**
 * @brief Report an error
 */
void reportError(ErrorLevel level, const char* format, ...);

/**
 * @brief Report an error with position information
 */
void reportErrorWithPos(ErrorLevel level, const ErrorPosition* position, 
                       const char* format, ...);

/**
 * @brief Helper for reporting errors with Result codes
 */
Result makeError(Result code, ErrorLevel level, const char* format, ...);

/**
 * @brief Helper for reporting errors with va_list
 */
void reportErrorV(ErrorLevel level, const char* format, va_list args);

/**
 * @brief Helper for reporting errors with position and va_list
 */
void reportErrorWithPosV(ErrorLevel level, const ErrorPosition* position, 
                       const char* format, va_list args);

/**
 * @brief Convert Result code to string
 */
const char* resultToString(Result result);

/**
 * @brief Convert ErrorLevel to string
 */
const char* errorLevelToString(ErrorLevel level);

/**
 * @brief Create an error position from current location
 */
#define COIL_CURRENT_POS coil::ErrorPosition{__FILE__, __LINE__, 0}

/**
 * @brief Convenience macro for reporting errors with source location
 */
#define COIL_REPORT_ERROR(level, format, ...) \
  do { \
    coil::ErrorPosition pos = {__FILE__, __LINE__, 0}; \
    coil::reportErrorWithPos(level, &pos, format, ##__VA_ARGS__); \
  } while(0)

/**
 * @brief Convenience macro for reporting errors and returning result code
 */
#define COIL_RETURN_ERROR(result, level, format, ...) \
  do { \
    coil::ErrorPosition pos = {__FILE__, __LINE__, 0}; \
    coil::reportErrorWithPos(level, &pos, format, ##__VA_ARGS__); \
    return result; \
  } while(0)

} // namespace coil