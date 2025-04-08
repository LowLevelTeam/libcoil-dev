#pragma once

#include "coil/log.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>
#include <array>

namespace coil {

/**
 * @brief Error codes
 */
enum class ErrorCode {
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
    Info,           // Informational
    Warning,        // Warning
    Error,          // Error
    Fatal           // Fatal error
};

/**
 * @brief Stream position
 */
struct StreamPosition {
    std::string fileName;  // Source file name
    size_t line;           // Line number
    size_t column;         // Column number
    size_t offset;         // Byte offset from start
    
    StreamPosition() : line(0), column(0), offset(0) {}
    
    StreamPosition(const std::string& file, size_t ln, size_t col, size_t off)
        : fileName(file), line(ln), column(col), offset(off) {}
};

/**
 * @brief Error entry
 */
class ErrorEntry {
public:
    ErrorEntry(ErrorCode code, 
              ErrorSeverity severity, 
              const StreamPosition& position,
              const std::string& message);
    
    inline ErrorCode getCode() const { return code_; }
    inline ErrorSeverity getSeverity() const { return severity_; }
    inline const StreamPosition& getPosition() const { return position_; }
    inline const std::string& getMessage() const { return message_; }
    
private:
    ErrorCode code_;
    ErrorSeverity severity_;
    StreamPosition position_;
    std::string message_;
};

/**
 * @brief Error handler function type
 */
using ErrorHandlerFunction = void (*)(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message,
    void* userData
);

/**
 * @brief Optimized error manager without thread safety or dynamic allocations
 */
class ErrorManager {
public:
    /**
     * @brief Construct an error manager
     * 
     * @param logger Logger for errors
     */
    explicit ErrorManager(Logger& logger);
    
    /**
     * @brief Add an error
     * 
     * @param code Error code
     * @param severity Error severity
     * @param position Stream position
     * @param message Error message
     */
    void addError(ErrorCode code, 
                ErrorSeverity severity, 
                const StreamPosition& position,
                const std::string& message);
    
    /**
     * @brief Add an informational message
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Message
     */
    inline void addInfo(ErrorCode code, const StreamPosition& position, const std::string& message) {
        addError(code, ErrorSeverity::Info, position, message);
    }
    
    /**
     * @brief Add a warning
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Warning message
     */
    inline void addWarning(ErrorCode code, const StreamPosition& position, const std::string& message) {
        addError(code, ErrorSeverity::Warning, position, message);
    }
    
    /**
     * @brief Add an error
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Error message
     */
    inline void addError(ErrorCode code, const StreamPosition& position, const std::string& message) {
        addError(code, ErrorSeverity::Error, position, message);
    }
    
    /**
     * @brief Add a fatal error
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Fatal error message
     */
    inline void addFatal(ErrorCode code, const StreamPosition& position, const std::string& message) {
        addError(code, ErrorSeverity::Fatal, position, message);
    }
    
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
     * @param count Output parameter to receive error count
     * @return const ErrorEntry* Pointer to the first error or nullptr if none
     */
    const ErrorEntry* getAllErrors(size_t* count = nullptr) const;
    
    /**
     * @brief Set an error handler
     * 
     * @param handler Handler function
     * @param userData User data for the handler
     */
    void setErrorHandler(ErrorHandlerFunction handler, void* userData);
    
    /**
     * @brief Destructor
     */
    ~ErrorManager() = default;

private:
    std::vector<ErrorEntry> errors_;
    Logger& logger_;
    ErrorHandlerFunction errorHandler_ = nullptr;
    void* userData_ = nullptr;
};

/**
 * @brief Context structure for the COIL library
 * 
 * This holds references to the logger and error manager
 * to avoid global state and allow for thread safety by using
 * separate contexts in different threads.
 */
struct Context {
    Logger& logger;
    ErrorManager& errorManager;
};

// Helper functions for error management
const char* getErrorMessage(ErrorCode code);
std::runtime_error createException(ErrorCode code, const std::string& message);
inline StreamPosition createStreamPosition(const std::string& fileName, 
                                       size_t line, 
                                       size_t column, 
                                       size_t offset) {
    return {fileName, line, column, offset};
}

// Convenience macros for error handling
#define COIL_ERROR_INFO(errorMgr, code, position, message) \
    (errorMgr).addInfo(code, position, message)

#define COIL_ERROR_WARNING(errorMgr, code, position, message) \
    (errorMgr).addWarning(code, position, message)

#define COIL_ERROR_ERROR(errorMgr, code, position, message) \
    (errorMgr).addError(code, position, message)

#define COIL_ERROR_FATAL(errorMgr, code, position, message) \
    (errorMgr).addFatal(code, position, message)

} // namespace coil