#pragma once

#include "coil/log.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <functional>
#include <optional>

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
    
    ErrorCode getCode() const { return code_; }
    ErrorSeverity getSeverity() const { return severity_; }
    const StreamPosition& getPosition() const { return position_; }
    const std::string& getMessage() const { return message_; }
    
private:
    ErrorCode code_;
    ErrorSeverity severity_;
    StreamPosition position_;
    std::string message_;
};

/**
 * @brief Error handler function type
 */
using ErrorHandlerFunction = std::function<void(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message,
    void* userData
)>;

/**
 * @brief Modern C++ Error Manager class
 */
class ErrorManager {
public:
    /**
     * @brief Construct an error manager
     * 
     * @param logger Logger for errors
     */
    explicit ErrorManager(std::shared_ptr<Logger> logger);
    
    /**
     * @brief Create an error manager
     * 
     * @param logger Logger for errors
     * @return std::shared_ptr<ErrorManager> 
     */
    static std::shared_ptr<ErrorManager> create(std::shared_ptr<Logger> logger);
    
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
    void addInfo(ErrorCode code, const StreamPosition& position, const std::string& message);
    
    /**
     * @brief Add a warning
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Warning message
     */
    void addWarning(ErrorCode code, const StreamPosition& position, const std::string& message);
    
    /**
     * @brief Add an error
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Error message
     */
    void addError(ErrorCode code, const StreamPosition& position, const std::string& message);
    
    /**
     * @brief Add a fatal error
     * 
     * @param code Error code
     * @param position Stream position
     * @param message Fatal error message
     */
    void addFatal(ErrorCode code, const StreamPosition& position, const std::string& message);
    
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
     * @return std::optional<ErrorEntry> 
     */
    std::optional<ErrorEntry> getLastError() const;
    
    /**
     * @brief Get all errors
     * 
     * @return std::vector<ErrorEntry> 
     */
    std::vector<ErrorEntry> getAllErrors() const;
    
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
    ~ErrorManager();
    
    // Delete copy and move constructors/assignments
    ErrorManager(const ErrorManager&) = delete;
    ErrorManager& operator=(const ErrorManager&) = delete;
    ErrorManager(ErrorManager&&) = delete;
    ErrorManager& operator=(ErrorManager&&) = delete;

private:
    std::vector<ErrorEntry> errors_;
    mutable std::mutex mutex_;
    std::shared_ptr<Logger> logger_;
    ErrorHandlerFunction errorHandler_;
    void* userData_;
};

// Global default error manager
extern std::shared_ptr<ErrorManager> defaultErrorManager;

// Initialize library error management
void initializeErrorHandling();

// Cleanup library error management
void cleanupErrorHandling();

// Get an error message for a given error code
std::string getErrorMessage(ErrorCode code);

// Create an exception from an error
std::runtime_error createException(ErrorCode code, const std::string& message);

// Convenience function to create a stream position
StreamPosition createStreamPosition(const std::string& fileName, 
                                  size_t line, 
                                  size_t column, 
                                  size_t offset);

// Default error manager convenience macros
#define COIL_DEFAULT_ERROR_INFO(code, position, message) \
    coil::defaultErrorManager->addInfo(code, position, message)

#define COIL_DEFAULT_ERROR_WARNING(code, position, message) \
    coil::defaultErrorManager->addWarning(code, position, message)

#define COIL_DEFAULT_ERROR_ERROR(code, position, message) \
    coil::defaultErrorManager->addError(code, position, message)

#define COIL_DEFAULT_ERROR_FATAL(code, position, message) \
    coil::defaultErrorManager->addFatal(code, position, message)

} // namespace coil