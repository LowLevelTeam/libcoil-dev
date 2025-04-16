/**
 * @file error.hpp
 * @brief Simple error handling for COIL
 */

#pragma once
#include "coil/types.hpp"
#include <cstdarg>

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
 * @brief Convert Result code to string
 */
const char* resultToString(Result result);

/**
 * @brief Create an error position from current location
 */
#define COIL_CURRENT_POS coil::ErrorPosition{__FILE__, __LINE__, 0}

} // namespace coil