# COIL Error Handling System

## Overview

The COIL library includes a robust error handling system that provides detailed error information with minimal overhead. This system is designed for zero-cost abstractions when no errors occur, while offering comprehensive feedback when errors are encountered.

## Design Philosophy

The error handling system follows these principles:

1. **Minimal Allocations**: Fixed-size buffers instead of dynamic allocations
2. **Clear Ownership**: No hidden RAII or smart pointers
3. **Direct Interface**: Simple C-style API with struct-based design
4. **Position Tracking**: Precise source location tracking for errors
5. **Integration with Logging**: Automatic logging on error reporting

## Error Codes

COIL defines a set of error codes to categorize different error types:

```cpp
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
```

## Error Severity

Each error is assigned a severity level:

```cpp
enum class ErrorSeverity {
    Info,           // Informational
    Warning,        // Warning
    Error,          // Error
    Fatal           // Fatal error
};
```

## Position Tracking

COIL provides source location tracking for comprehensive error reporting:

```cpp
struct StreamPosition {
    char fileName[256] = {0};  // Source file name 
    size_t line = 0;           // Line number
    size_t column = 0;         // Column number
    size_t offset = 0;         // Byte offset from start
};
```

## Error Entry

Each error is represented by an `ErrorEntry` struct:

```cpp
struct ErrorEntry {
    ErrorCode code;
    ErrorSeverity severity;
    StreamPosition position;
    char message[512];  // Fixed size to avoid allocations
    
    // Methods to access the error information
    ErrorCode getCode() const;
    ErrorSeverity getSeverity() const;
    const StreamPosition& getPosition() const;
    const char* getMessage() const;
};
```

## Error Manager

The `ErrorManager` struct centralizes error handling:

```cpp
struct ErrorManager {
    // Fixed-size array of errors (no dynamic allocations)
    static constexpr size_t MAX_ERRORS = 64;
    ErrorEntry errors[MAX_ERRORS];
    size_t errorCount;
    
    Logger* logger;
    ErrorHandlerFunction errorHandler;
    void* userData;
    
    // Error handling methods
    void addError(ErrorCode code, ErrorSeverity severity, 
                 const StreamPosition& position, const char* message);
    void addInfo(ErrorCode code, const StreamPosition& position, const char* message);
    void addWarning(ErrorCode code, const StreamPosition& position, const char* message);
    void addError(ErrorCode code, const StreamPosition& position, const char* message);
    void addFatal(ErrorCode code, const StreamPosition& position, const char* message);
    
    // Error information methods
    bool hasErrors(ErrorSeverity minSeverity = ErrorSeverity::Error) const;
    void dumpErrors() const;
    void clearErrors();
    const ErrorEntry* getLastError() const;
    const ErrorEntry* getAllErrors(size_t* count = nullptr) const;
    
    // Custom error handling
    void setErrorHandler(ErrorHandlerFunction handler, void* userData);
};
```

## Custom Error Handlers

COIL supports custom error handlers for application-specific error processing:

```cpp
using ErrorHandlerFunction = void (*)(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const char* message,
    void* userData
);
```

Example of setting a custom error handler:

```cpp
void myErrorHandler(
    coil::ErrorCode code,
    coil::ErrorSeverity severity,
    const coil::StreamPosition& position,
    const char* message,
    void* userData
) {
    fprintf(stderr, "Error %d: %s at %s:%zu:%zu\n",
        static_cast<int>(code),
        message,
        position.fileName,
        position.line,
        position.column
    );
    
    // Additional application-specific handling
    if (severity == coil::ErrorSeverity::Fatal) {
        exit(1);
    }
}

// Later in the code:
errorManager.setErrorHandler(myErrorHandler, nullptr);
```

## Convenience Macros

COIL provides macros for easier error reporting:

```cpp
// Report an info message
COIL_ERROR_INFO(errorMgr, ErrorCode::None, position, "Information message");

// Report a warning
COIL_ERROR_WARNING(errorMgr, ErrorCode::Format, position, "Warning message");

// Report an error
COIL_ERROR_ERROR(errorMgr, ErrorCode::Syntax, position, "Error message");

// Report a fatal error
COIL_ERROR_FATAL(errorMgr, ErrorCode::Memory, position, "Fatal error message");
```

## Integration with Logging

The error manager is integrated with COIL's logging system:

```cpp
coil::Logger logger("COIL", stdout, coil::LogLevel::Info);
coil::ErrorManager errorMgr(&logger);

// When reporting an error
errorMgr.addError(coil::ErrorCode::Syntax, position, "Invalid syntax");

// This automatically logs the error with the appropriate level
```

## Error Context

COIL uses a Context structure to pass around the error manager and logger:

```cpp
struct Context {
    Logger* logger;
    ErrorManager* errorManager;
};

// Usage example
void processFile(const char* filename, const Context* ctx) {
    if (!filename) {
        ctx->errorManager->addError(ErrorCode::Argument, StreamPosition(), 
                               "Invalid filename");
        return;
    }
    
    // Process the file...
}
```

## Best Practices

1. **Always check for errors**: Check `hasErrors()` after operations that might fail
2. **Provide detailed error messages**: Include specific information about what went wrong
3. **Include position information**: Whenever possible, include source location details
4. **Use appropriate severity levels**: Don't mark warnings as errors or vice versa
5. **Log errors immediately**: The error manager automatically logs errors when they're added

## Thread Safety

The current implementation of `ErrorManager` is not thread-safe. For multi-threaded applications:

1. Use separate `Context` objects for different threads
2. Or implement synchronization when accessing a shared error manager