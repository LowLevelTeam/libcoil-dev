#include <catch2/catch_all.hpp>
#include <cstdio>
#include <string>
#include "coil/err.hpp"
#include "coil/log.hpp"

// Reuse the CaptureBuffer from test_log.cpp
struct CaptureBuffer {
    static constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    FILE* fp;
    
    CaptureBuffer() {
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    ~CaptureBuffer() {
        if (fp) fclose(fp);
    }
    
    FILE* getFile() const {
        return fp;
    }
    
    const char* getBuffer() const {
        return buffer;
    }
    
    void clear() {
        if (fp) fclose(fp);
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    bool contains(const char* str) const {
        return strstr(buffer, str) != nullptr;
    }
};

// Custom error handler for testing
static coil::ErrorCode lastErrorCode = coil::ErrorCode::None;
static coil::ErrorSeverity lastErrorSeverity = coil::ErrorSeverity::Info;
static char lastErrorMessage[512];
static coil::StreamPosition lastErrorPosition;

void testErrorHandler(
    coil::ErrorCode code,
    coil::ErrorSeverity severity,
    const coil::StreamPosition& position,
    const char* message,
    void* userData
) {
    (void)userData;
    lastErrorCode = code;
    lastErrorSeverity = severity;
    strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
    lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
    lastErrorPosition = position;
}

TEST_CASE("ErrorEntry construction and accessors", "[error]") {
    coil::StreamPosition pos = coil::createStreamPosition("test.txt", 10, 20, 300);
    coil::ErrorEntry entry(
        coil::ErrorCode::Format, 
        coil::ErrorSeverity::Warning, 
        pos, 
        "Test error message"
    );
    
    REQUIRE(entry.getCode() == coil::ErrorCode::Format);
    REQUIRE(entry.getSeverity() == coil::ErrorSeverity::Warning);
    REQUIRE(strcmp(entry.getMessage(), "Test error message") == 0);
    
    const auto& entryPos = entry.getPosition();
    REQUIRE(strcmp(entryPos.fileName, "test.txt") == 0);
    REQUIRE(entryPos.line == 10);
    REQUIRE(entryPos.column == 20);
    REQUIRE(entryPos.offset == 300);
}

TEST_CASE("ErrorManager basic functionality", "[error]") {
    CaptureBuffer capture;
    coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
    coil::ErrorManager errorMgr(&logger);
    
    SECTION("Initial state") {
        REQUIRE_FALSE(errorMgr.hasErrors());
        REQUIRE(errorMgr.getLastError() == nullptr);
        
        size_t count = 0;
        REQUIRE(errorMgr.getAllErrors(&count) == nullptr);
        REQUIRE(count == 0);
    }
    
    SECTION("Adding errors") {
        coil::StreamPosition pos = coil::createStreamPosition("test.txt", 1, 1, 0);
        
        // Add an info message
        errorMgr.addInfo(coil::ErrorCode::None, pos, "Info message");
        REQUIRE_FALSE(errorMgr.hasErrors());  // Info is not an error
        REQUIRE(errorMgr.hasErrors(coil::ErrorSeverity::Info));
        
        // Add a warning
        errorMgr.addWarning(coil::ErrorCode::Format, pos, "Warning message");
        REQUIRE_FALSE(errorMgr.hasErrors());  // Warning is not an error
        REQUIRE(errorMgr.hasErrors(coil::ErrorSeverity::Warning));
        
        // Add an error
        errorMgr.addError(coil::ErrorCode::Syntax, pos, "Error message");
        REQUIRE(errorMgr.hasErrors());  // Now we have an error
        
        // Check the last error
        const coil::ErrorEntry* lastError = errorMgr.getLastError();
        REQUIRE(lastError != nullptr);
        REQUIRE(lastError->getCode() == coil::ErrorCode::Syntax);
        REQUIRE(lastError->getSeverity() == coil::ErrorSeverity::Error);
        REQUIRE(strcmp(lastError->getMessage(), "Error message") == 0);
        
        // Get all errors
        size_t errorCount = 0;
        const coil::ErrorEntry* allErrors = errorMgr.getAllErrors(&errorCount);
        REQUIRE(allErrors != nullptr);
        REQUIRE(errorCount == 3);  // Info, Warning, Error
        
        // First error should be the info message
        REQUIRE(allErrors[0].getCode() == coil::ErrorCode::None);
        REQUIRE(allErrors[0].getSeverity() == coil::ErrorSeverity::Info);
        REQUIRE(strcmp(allErrors[0].getMessage(), "Info message") == 0);
        
        // Clear errors
        errorMgr.clearErrors();
        REQUIRE_FALSE(errorMgr.hasErrors());
        REQUIRE(errorMgr.getLastError() == nullptr);
        
        size_t newCount = 0;
        REQUIRE(errorMgr.getAllErrors(&newCount) == nullptr);
        REQUIRE(newCount == 0);
    }
    
    SECTION("Error handler") {
        // Reset global variables
        lastErrorCode = coil::ErrorCode::None;
        lastErrorSeverity = coil::ErrorSeverity::Info;
        lastErrorMessage[0] = '\0';
        memset(&lastErrorPosition, 0, sizeof(lastErrorPosition));
        
        // Set the error handler
        errorMgr.setErrorHandler(testErrorHandler, nullptr);
        
        // Add an error
        coil::StreamPosition pos = coil::createStreamPosition("handler.txt", 42, 13, 555);
        errorMgr.addError(coil::ErrorCode::IO, coil::ErrorSeverity::Fatal, pos, "Fatal IO error");
        
        // Check that the handler was called with the right parameters
        REQUIRE(lastErrorCode == coil::ErrorCode::IO);
        REQUIRE(lastErrorSeverity == coil::ErrorSeverity::Fatal);
        REQUIRE(strcmp(lastErrorMessage, "Fatal IO error") == 0);
        REQUIRE(strcmp(lastErrorPosition.fileName, "handler.txt") == 0);
        REQUIRE(lastErrorPosition.line == 42);
        REQUIRE(lastErrorPosition.column == 13);
        REQUIRE(lastErrorPosition.offset == 555);
    }
}

TEST_CASE("Error utility functions", "[error]") {
    SECTION("getErrorMessage") {
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::None), "No error") == 0);
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::Memory), "Memory allocation failure") == 0);
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::IO), "I/O error") == 0);
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::Format), "Invalid format") == 0);
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::Syntax), "Syntax error") == 0);
        REQUIRE(strcmp(coil::getErrorMessage(coil::ErrorCode::Custom), "Custom error") == 0);
    }
    
    SECTION("createStreamPosition") {
        coil::StreamPosition pos = coil::createStreamPosition("file.txt", 100, 50, 1500);
        REQUIRE(strcmp(pos.fileName, "file.txt") == 0);
        REQUIRE(pos.line == 100);
        REQUIRE(pos.column == 50);
        REQUIRE(pos.offset == 1500);
    }
}

TEST_CASE("Error convenience macros", "[error]") {
    CaptureBuffer capture;
    coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
    coil::ErrorManager errorMgr(&logger);
    coil::StreamPosition pos = coil::createStreamPosition("macros.txt", 1, 1, 0);
    
    SECTION("COIL_ERROR_INFO macro") {
        COIL_ERROR_INFO(&errorMgr, coil::ErrorCode::None, pos, "Info via macro");
        REQUIRE(capture.contains("Info via macro"));
        REQUIRE(errorMgr.hasErrors(coil::ErrorSeverity::Info));
    }
    
    SECTION("COIL_ERROR_WARNING macro") {
        COIL_ERROR_WARNING(&errorMgr, coil::ErrorCode::Format, pos, "Warning via macro");
        REQUIRE(capture.contains("Warning via macro"));
        REQUIRE(errorMgr.hasErrors(coil::ErrorSeverity::Warning));
    }
    
    SECTION("COIL_ERROR_ERROR macro") {
        COIL_ERROR_ERROR(&errorMgr, coil::ErrorCode::Syntax, pos, "Error via macro");
        REQUIRE(capture.contains("Error via macro"));
        REQUIRE(errorMgr.hasErrors());
    }
    
    SECTION("COIL_ERROR_FATAL macro") {
        COIL_ERROR_FATAL(&errorMgr, coil::ErrorCode::IO, pos, "Fatal via macro");
        REQUIRE(capture.contains("Fatal via macro"));
        REQUIRE(errorMgr.hasErrors(coil::ErrorSeverity::Fatal));
    }
}