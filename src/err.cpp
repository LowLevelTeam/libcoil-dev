#include "coil/err.hpp"
#include <cstring>

namespace coil {

// Error code messages - static string literals to avoid runtime string construction
static const char* const ERROR_MESSAGES[] = {
  "No error",                  // None
  "Memory allocation failure", // Memory
  "I/O error",                 // IO
  "Invalid format",            // Format
  "Syntax error",              // Syntax
  "Semantic error",            // Semantic
  "Invalid reference",         // Reference
  "Overflow",                  // Overflow
  "Underflow",                 // Underflow
  "Out of bounds",             // Bounds
  "Invalid state",             // State
  "Invalid argument",          // Argument
  "Internal error",            // Internal
  "Unsupported operation",     // Unsupported
  "Custom error"               // Custom
};

// StreamPosition implementation
StreamPosition::StreamPosition(const char* file, size_t ln, size_t col, size_t off)
  : line(ln), column(col), offset(off) {
  if (file) {
      strncpy(fileName, file, sizeof(fileName) - 1);
      fileName[sizeof(fileName) - 1] = '\0'; // Ensure null termination
  }
}

// ErrorEntry implementation
ErrorEntry::ErrorEntry(
  ErrorCode code, 
  ErrorSeverity severity,
  const StreamPosition& position,
  const char* message)
  : code(code)
  , severity(severity)
  , position(position) {
  if (message) {
      strncpy(this->message, message, sizeof(this->message) - 1);
      this->message[sizeof(this->message) - 1] = '\0'; // Ensure null termination
  } else {
      this->message[0] = '\0';
  }
}

// ErrorManager implementation
ErrorManager::ErrorManager(Logger* loggerPtr)
  : logger(loggerPtr) {
}

void ErrorManager::addError(
  ErrorCode code,
  ErrorSeverity severity,
  const StreamPosition& position,
  const char* message) {
  
  if (errorCount >= MAX_ERRORS) {
      // Cannot add more errors, log this fact if possible
      if (logger) {
          logger->log(LogLevel::Error, __FILE__, __LINE__, __func__,
                    "Error buffer overflow, cannot add more errors");
      }
      return;
  }
  
  // Create a new error entry
  errors[errorCount++] = ErrorEntry(code, severity, position, message);
  
  // Log the error immediately based on severity
  if (!logger) return;
  
  LogLevel logLevel;
  
  switch (severity) {
      case ErrorSeverity::Info:
          logLevel = LogLevel::Info;
          break;
      case ErrorSeverity::Warning:
          logLevel = LogLevel::Warning;
          break;
      case ErrorSeverity::Error:
          logLevel = LogLevel::Error;
          break;
      case ErrorSeverity::Fatal:
          logLevel = LogLevel::Fatal;
          break;
      default:
          logLevel = LogLevel::Error;
          break;
  }
  
  if (position.fileName[0] != '\0') {
      logger->log(logLevel, __FILE__, __LINE__, __func__, "%s: %s (at %s line %zu, column %zu, offset %zu)",
          getErrorMessage(code),
          message,
          position.fileName,
          position.line,
          position.column,
          position.offset
      );
  } else {
      logger->log(logLevel, __FILE__, __LINE__, __func__, "%s: %s",
          getErrorMessage(code),
          message
      );
  }
  
  // Call the error handler if set
  if (errorHandler) {
      errorHandler(code, severity, position, message, userData);
  }
}

void ErrorManager::addInfo(ErrorCode code, const StreamPosition& position, const char* message) {
  addError(code, ErrorSeverity::Info, position, message);
}

void ErrorManager::addWarning(ErrorCode code, const StreamPosition& position, const char* message) {
  addError(code, ErrorSeverity::Warning, position, message);
}

void ErrorManager::addError(ErrorCode code, const StreamPosition& position, const char* message) {
  addError(code, ErrorSeverity::Error, position, message);
}

void ErrorManager::addFatal(ErrorCode code, const StreamPosition& position, const char* message) {
  addError(code, ErrorSeverity::Fatal, position, message);
}

bool ErrorManager::hasErrors(ErrorSeverity minSeverity) const {
  for (size_t i = 0; i < errorCount; i++) {
      if (errors[i].getSeverity() >= minSeverity) {
          return true;
      }
  }
  
  return false;
}

void ErrorManager::dumpErrors() const {
  if (!logger) return;
  
  if (errorCount == 0) {
      COIL_INFO(logger, "No errors reported");
  } else {
      COIL_INFO(logger, "Error summary (%zu errors):", errorCount);
      
      for (size_t i = 0; i < errorCount; ++i) {
          const auto& error = errors[i];
          
          const auto& pos = error.getPosition();
          
          if (pos.fileName[0] != '\0') {
              COIL_INFO(logger, "[%zu] %s: %s (at %s line %zu, column %zu, offset %zu)",
                      i,
                      getErrorMessage(error.getCode()),
                      error.getMessage(),
                      pos.fileName,
                      pos.line,
                      pos.column,
                      pos.offset);
          } else {
              COIL_INFO(logger, "[%zu] %s: %s",
                      i,
                      getErrorMessage(error.getCode()),
                      error.getMessage());
          }
      }
  }
}

void ErrorManager::clearErrors() {
  errorCount = 0;
}

const ErrorEntry* ErrorManager::getLastError() const {
  if (errorCount == 0) {
      return nullptr;
  }
  
  return &errors[errorCount - 1];
}

const ErrorEntry* ErrorManager::getAllErrors(size_t* count) const {
  if (count) {
      *count = errorCount;
  }
  
  return errorCount == 0 ? nullptr : errors;
}

void ErrorManager::setErrorHandler(ErrorHandlerFunction handler, void* data) {
  errorHandler = handler;
  userData = data;
}

const char* getErrorMessage(ErrorCode code) {
  auto index = static_cast<size_t>(code);
  if (index < sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0])) {
      return ERROR_MESSAGES[index];
  }
  
  return "Unknown error";
}

StreamPosition createStreamPosition(const char* fileName, size_t line, size_t column, size_t offset) {
  return StreamPosition(fileName, line, column, offset);
}

} // namespace coil