#include "coil/err.hpp"
#include <cstring>
#include <stdexcept>

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
ErrorManager::ErrorManager(Logger& logger)
    : logger_(logger) {
}

void ErrorManager::addError(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message) {
    
    // Create a new error entry
    ErrorEntry entry(code, severity, position, message);
    
    // Add to list
    errors_.push_back(entry);
    
    // Log the error immediately based on severity
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
        logger_.log(logLevel, __FILE__, __LINE__, __func__, "%s: %s (at %s line %zu, column %zu, offset %zu)",
            getErrorMessage(code),
            message.c_str(),
            position.fileName.c_str(),
            position.line,
            position.column,
            position.offset
        );
    } else {
        logger_.log(logLevel, __FILE__, __LINE__, __func__, "%s: %s",
            getErrorMessage(code),
            message.c_str()
        );
    }
    
    // Call the error handler if set
    if (errorHandler_) {
        errorHandler_(code, severity, position, message, userData_);
    }
}

bool ErrorManager::hasErrors(ErrorSeverity minSeverity) const {
    for (const auto& error : errors_) {
        if (error.getSeverity() >= minSeverity) {
            return true;
        }
    }
    
    return false;
}

void ErrorManager::dumpErrors() const {
    if (errors_.empty()) {
        COIL_INFO(logger_, "No errors reported");
    } else {
        COIL_INFO(logger_, "Error summary (%zu errors):", errors_.size());
        
        for (size_t i = 0; i < errors_.size(); ++i) {
            const auto& error = errors_[i];
            
            const auto& pos = error.getPosition();
            
            if (!pos.fileName.empty()) {
                COIL_INFO(logger_, "[%zu] %s: %s (at %s line %zu, column %zu, offset %zu)",
                        i,
                        getErrorMessage(error.getCode()),
                        error.getMessage().c_str(),
                        pos.fileName.c_str(),
                        pos.line,
                        pos.column,
                        pos.offset);
            } else {
                COIL_INFO(logger_, "[%zu] %s: %s",
                        i,
                        getErrorMessage(error.getCode()),
                        error.getMessage().c_str());
            }
        }
    }
}

void ErrorManager::clearErrors() {
    errors_.clear();
}

const ErrorEntry* ErrorManager::getLastError() const {
    if (errors_.empty()) {
        return nullptr;
    }
    
    return &errors_.back();
}

const ErrorEntry* ErrorManager::getAllErrors(size_t* count) const {
    if (count) {
        *count = errors_.size();
    }
    
    return errors_.empty() ? nullptr : errors_.data();
}

void ErrorManager::setErrorHandler(ErrorHandlerFunction handler, void* userData) {
    errorHandler_ = handler;
    userData_ = userData;
}

const char* getErrorMessage(ErrorCode code) {
    auto index = static_cast<size_t>(code);
    if (index < sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0])) {
        return ERROR_MESSAGES[index];
    }
    
    return "Unknown error";
}

std::runtime_error createException(ErrorCode code, const std::string& message) {
    std::string fullMessage = getErrorMessage(code);
    fullMessage += ": ";
    fullMessage += message;
    return std::runtime_error(fullMessage);
}

} // namespace coil