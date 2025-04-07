#include "coil/coil.hpp"
#include <cstring>

namespace coil {

// Library version
constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "0.1.0";

// Error handler
static ErrorHandlerFunction errorHandler = nullptr;
static void* errorHandlerUserData = nullptr;

// Last error message
static std::string lastError;

// Default error handler function
static void defaultErrorHandler(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message,
    void* userData) {
    (void)userData;
    
    // Store the error message
    if (position.fileName.empty()) {
        lastError = getErrorMessage(code) + ": " + message;
    } else {
        lastError = getErrorMessage(code) + ": " + message + " (at " +
            position.fileName + " line " + std::to_string(position.line) +
            ", column " + std::to_string(position.column) +
            ", offset " + std::to_string(position.offset) + ")";
    }
    
    // Log the error (the ErrorManager already does this, but we include it here for completeness)
    if (defaultLogger) {
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
        
        if (position.fileName.empty()) {
            COIL_INFO(defaultLogger, "%s", lastError.c_str());
        } else {
            COIL_INFO(defaultLogger, "%s", lastError.c_str());
        }
    }
    
    // Call the custom error handler if set
    if (errorHandler) {
        errorHandler(code, severity, position, message, errorHandlerUserData);
    }
}

Version getVersion() {
    Version version;
    version.major = VERSION_MAJOR;
    version.minor = VERSION_MINOR;
    version.patch = VERSION_PATCH;
    version.string = VERSION_STRING;
    return version;
}

bool initialize() {
    // Initialize in the correct order to handle dependencies
    
    // Step 1: Initialize logging
    initializeLogging();
    if (!defaultLogger) {
        return false;
    }
    
    // Step 2: Initialize error handling
    initializeErrorHandling();
    if (!defaultErrorManager) {
        cleanupLogging();
        return false;
    }
    
    // Step 3: Initialize memory management
    initializeMemory();
    if (!globalArena) {
        cleanupErrorHandling();
        cleanupLogging();
        return false;
    }
    
    // Step 4: Initialize thread management
    if (!initializeThreading()) {
        cleanupMemory();
        cleanupErrorHandling();
        cleanupLogging();
        return false;
    }
    
    // Log initialization
    Version version = getVersion();
    COIL_DEFAULT_INFO("COIL v%s initialized", version.string.c_str());
    
    return true;
}

void cleanup() {
    // Clean up in the reverse order of initialization
    
    // Log cleanup
    COIL_DEFAULT_INFO("COIL shutting down");
    
    // Step 1: Clean up thread management
    cleanupThreading();
    
    // Step 2: Clean up memory management
    cleanupMemory();
    
    // Step 3: Clean up error handling
    cleanupErrorHandling();
    
    // Step 4: Clean up logging
    cleanupLogging();
}

std::string getLastError() {
    return lastError;
}

void setLogLevel(LogLevel level) {
    if (defaultLogger) {
        defaultLogger->setLevel(level);
    }
}

void setErrorHandler(ErrorHandlerFunction handler, void* userData) {
    errorHandler = handler;
    errorHandlerUserData = userData;
}

} // namespace coil