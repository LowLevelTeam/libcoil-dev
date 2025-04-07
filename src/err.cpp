#include "coil/err.hpp"
#include <cstring>
#include <stdexcept>

namespace coil {

// Global default error manager
std::shared_ptr<ErrorManager> defaultErrorManager;

// Error code messages
static const std::string errorMessages[] = {
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

// ErrorEntry implementation
ErrorEntry::ErrorEntry(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message)
    : code_(code)
    , severity_(severity)
    , position_(position)
    , message_(message) {
}

// ErrorManager implementation
ErrorManager::ErrorManager(std::shared_ptr<Logger> logger)
    : logger_(logger ? logger : defaultLogger)
    , errorHandler_(nullptr)
    , userData_(nullptr) {
}

std::shared_ptr<ErrorManager> ErrorManager::create(std::shared_ptr<Logger> logger) {
    return std::shared_ptr<ErrorManager>(new ErrorManager(logger));
}

void ErrorManager::addError(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message) {
    
    // Create a new error entry
    ErrorEntry entry(code, severity, position, message);
    
    // Add to list
    {
        std::lock_guard<std::mutex> lock(mutex_);
        errors_.push_back(entry);
    }
    
    // Log the error immediately based on severity
    if (logger_) {
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
        
        if (!position.fileName.empty()) {
            COIL_INFO(logger_, "%s: %s (at %s line %zu, column %zu, offset %zu)",
                    getErrorMessage(code).c_str(),
                    message.c_str(),
                    position.fileName.c_str(),
                    position.line,
                    position.column,
                    position.offset);
        } else {
            COIL_INFO(logger_, "%s: %s",
                    getErrorMessage(code).c_str(),
                    message.c_str());
        }
    }
    
    // Call the error handler if set
    if (errorHandler_) {
        errorHandler_(code, severity, position, message, userData_);
    }
}

void ErrorManager::addInfo(
    ErrorCode code,
    const StreamPosition& position,
    const std::string& message) {
    
    addError(code, ErrorSeverity::Info, position, message);
}

void ErrorManager::addWarning(
    ErrorCode code,
    const StreamPosition& position,
    const std::string& message) {
    
    addError(code, ErrorSeverity::Warning, position, message);
}

void ErrorManager::addError(
    ErrorCode code,
    const StreamPosition& position,
    const std::string& message) {
    
    addError(code, ErrorSeverity::Error, position, message);
}

void ErrorManager::addFatal(
    ErrorCode code,
    const StreamPosition& position,
    const std::string& message) {
    
    addError(code, ErrorSeverity::Fatal, position, message);
}

bool ErrorManager::hasErrors(ErrorSeverity minSeverity) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& error : errors_) {
        if (error.getSeverity() >= minSeverity) {
            return true;
        }
    }
    
    return false;
}

void ErrorManager::dumpErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!logger_) return;
    
    if (errors_.empty()) {
        COIL_INFO(logger_, "No errors reported");
    } else {
        COIL_INFO(logger_, "Error summary (%zu errors):", errors_.size());
        
        for (size_t i = 0; i < errors_.size(); ++i) {
            const auto& error = errors_[i];
            LogLevel logLevel;
            
            switch (error.getSeverity()) {
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
            
            const auto& pos = error.getPosition();
            
            if (!pos.fileName.empty()) {
                COIL_INFO(logger_, "[%zu] %s: %s (at %s line %zu, column %zu, offset %zu)",
                        i,
                        getErrorMessage(error.getCode()).c_str(),
                        error.getMessage().c_str(),
                        pos.fileName.c_str(),
                        pos.line,
                        pos.column,
                        pos.offset);
            } else {
                COIL_INFO(logger_, "[%zu] %s: %s",
                        i,
                        getErrorMessage(error.getCode()).c_str(),
                        error.getMessage().c_str());
            }
        }
    }
}

void ErrorManager::clearErrors() {
    std::lock_guard<std::mutex> lock(mutex_);
    errors_.clear();
}

std::optional<ErrorEntry> ErrorManager::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (errors_.empty()) {
        return std::nullopt;
    }
    
    return errors_.back();
}

std::vector<ErrorEntry> ErrorManager::getAllErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return errors_;
}

void ErrorManager::setErrorHandler(ErrorHandlerFunction handler, void* userData) {
    std::lock_guard<std::mutex> lock(mutex_);
    errorHandler_ = handler;
    userData_ = userData;
}

ErrorManager::~ErrorManager() {
    // Nothing to clean up
}

StreamPosition createStreamPosition(
    const std::string& fileName,
    size_t line,
    size_t column,
    size_t offset) {
    
    return {fileName, line, column, offset};
}

void initializeErrorHandling() {
    if (!defaultLogger) {
        initializeLogging();
    }
    
    if (!defaultErrorManager) {
        defaultErrorManager = ErrorManager::create(defaultLogger);
    }
}

void cleanupErrorHandling() {
    defaultErrorManager.reset();
}

std::string getErrorMessage(ErrorCode code) {
    auto index = static_cast<size_t>(code);
    if (index < sizeof(errorMessages) / sizeof(errorMessages[0])) {
        return errorMessages[index];
    }
    
    return "Unknown error";
}

std::runtime_error createException(ErrorCode code, const std::string& message) {
    return std::runtime_error(getErrorMessage(code) + ": " + message);
}

} // namespace coil