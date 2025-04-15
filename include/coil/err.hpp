#pragma once

#include "coil/log.hpp"
#include <string>
#include <vector>
#include <functional>

namespace coil {

/**
* @brief Error codes
*/
enum ErrorCode {
  None = 0,
  Memory,         // Memory allocation failure
  IO,             // I/O error
  Format,         // Invalid format
  Syntax,         // Syntax error
  Semantic,       // Semantic error
  Reference,      // Invalid reference
  Overflow,       // Overflow
  Underflow,      // Underflow
  Bounds,         // Out of bounds
  State,          // Invalid state
  Argument,       // Invalid argument
  Internal,       // Internal error
  Unsupported,    // Unsupported operation
  Custom          // Custom error
};

/**
* @brief Error severity
*/
enum class ErrorSeverity {
  None,
  Info,           // Informational
  Warning,        // Warning
  Error,          // Error
  Fatal           // Fatal error
};

/**
* @brief Stream position
*/
struct StreamPosition {
  char fileName[256] = {0};  // Source file name (fixed size to avoid allocations)
  size_t line = 0;           // Line number
  size_t column = 0;         // Column number
  size_t offset = 0;         // Byte offset from start
  
  // Default constructor produces a zeroed position
  StreamPosition() = default;
  
  // Constructor with explicit values
  StreamPosition(const char* file, size_t ln, size_t col, size_t off);
};

/**
* @brief Error entry
*/
struct ErrorEntry {
  ErrorCode code;
  ErrorSeverity severity;
  StreamPosition position;
  char message[512];  // Fixed size to avoid allocations
  
  ErrorEntry(ErrorCode code = ErrorCode::None, ErrorSeverity severity = ErrorSeverity::None, 
            const StreamPosition& position = StreamPosition{}, const char* message = nullptr);
  
  ErrorCode getCode() const { return code; }
  ErrorSeverity getSeverity() const { return severity; }
  const StreamPosition& getPosition() const { return position; }
  const char* getMessage() const { return message; }
};

/**
* @brief Error handler function type
*/
using ErrorHandlerFunction = void (*)(
  ErrorCode code,
  ErrorSeverity severity,
  const StreamPosition& position,
  const char* message,
  void* userData
);

/**
* @brief Optimized error manager
*/
struct ErrorManager {
  // Fixed-size array of errors for performance (no dynamic allocations in typical use)
  static constexpr size_t MAX_ERRORS = 64;
  ErrorEntry errors[MAX_ERRORS];
  size_t errorCount = 0;
  
  Logger* logger;
  ErrorHandlerFunction errorHandler = nullptr;
  void* userData = nullptr;
  
  /**
    * @brief Construct an error manager
    * 
    * @param loggerPtr Logger for errors (must not be null)
    */
  explicit ErrorManager(Logger* loggerPtr);
  
  /**
    * @brief Add an error
    * 
    * If the error buffer is full, the oldest error is replaced with this new one
    * using a circular buffer approach. This prevents silently dropping errors.
    * 
    * @param code Error code
    * @param severity Error severity
    * @param position Stream position
    * @param message Error message (must not be null)
    */
  void addError(ErrorCode code, 
              ErrorSeverity severity, 
              const StreamPosition& position,
              const char* message);
  
  /**
    * @brief Add an informational message
    * 
    * @param code Error code
    * @param position Stream position
    * @param message Message (must not be null)
    */
  void addInfo(ErrorCode code, const StreamPosition& position, const char* message);
  
  /**
    * @brief Add a warning
    * 
    * @param code Error code
    * @param position Stream position
    * @param message Warning message (must not be null)
    */
  void addWarning(ErrorCode code, const StreamPosition& position, const char* message);
  
  /**
    * @brief Add an error
    * 
    * @param code Error code
    * @param position Stream position
    * @param message Error message (must not be null)
    */
  void addError(ErrorCode code, const StreamPosition& position, const char* message);
  
  /**
    * @brief Add a fatal error
    * 
    * @param code Error code
    * @param position Stream position
    * @param message Fatal error message (must not be null)
    */
  void addFatal(ErrorCode code, const StreamPosition& position, const char* message);
  
  /**
    * @brief Check if there are any errors of given severity or higher
    * 
    * @param minSeverity Minimum severity
    * @return true if there are errors
    */
  bool hasErrors(ErrorSeverity minSeverity = ErrorSeverity::Error) const;
  
  /**
    * @brief Dump all errors to the logger
    */
  void dumpErrors() const;
  
  /**
    * @brief Clear all errors
    */
  void clearErrors();
  
  /**
    * @brief Get the last error
    * 
    * @return const ErrorEntry* Pointer to the last error or nullptr if none
    */
  const ErrorEntry* getLastError() const;
  
  /**
    * @brief Get all errors
    * 
    * @param count Output parameter to receive error count (can be null)
    * @return const ErrorEntry* Pointer to the first error or nullptr if none
    */
  const ErrorEntry* getAllErrors(size_t* count = nullptr) const;
  
  /**
    * @brief Set an error handler
    * 
    * @param handler Handler function (can be null to remove handler)
    * @param data User data for the handler
    */
  void setErrorHandler(ErrorHandlerFunction handler, void* data);
};

/**
* @brief Context structure for the COIL library
* 
* This holds pointers to the logger and error manager
* to avoid global state and allow for thread safety by using
* separate contexts in different threads.
*/
struct Context {
  Logger* logger;           // Pointer to logger (not owned)
  ErrorManager* errorManager; // Pointer to error manager (not owned)
};

// Helper functions for error management
const char* getErrorMessage(ErrorCode code);
StreamPosition createStreamPosition(const char* fileName, size_t line, size_t column, size_t offset);

// Convenience macros for error handling
#define COIL_ERROR_INFO(errorMgr, code, position, message) \
  (errorMgr)->addInfo(code, position, message)

#define COIL_ERROR_WARNING(errorMgr, code, position, message) \
  (errorMgr)->addWarning(code, position, message)

#define COIL_ERROR_ERROR(errorMgr, code, position, message) \
  (errorMgr)->addError(code, position, message)

#define COIL_ERROR_FATAL(errorMgr, code, position, message) \
  (errorMgr)->addFatal(code, position, message)

} // namespace coil