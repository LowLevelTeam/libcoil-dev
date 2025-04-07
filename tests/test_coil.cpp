#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/coil.hpp"
#include <string>
#include <memory>

using namespace coil;
using namespace testing;

class CoilLibraryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // We don't initialize COIL here because we want to test initialization
    }
    
    void TearDown() override {
        // Clean up if initialized
        cleanup();
    }
};

TEST_F(CoilLibraryTest, VersionInformation) {
    // Get library version
    Version version = getVersion();
    
    // Verify version info
    EXPECT_GE(version.major, 0);
    EXPECT_GE(version.minor, 0);
    EXPECT_GE(version.patch, 0);
    EXPECT_FALSE(version.string.empty());
    
    // Verify version string format
    std::string expected = std::to_string(version.major) + "." +
                          std::to_string(version.minor) + "." +
                          std::to_string(version.patch);
    EXPECT_EQ(version.string, expected);
}

TEST_F(CoilLibraryTest, InitializationCleanup) {
    // Initialize COIL
    EXPECT_TRUE(initialize());
    
    // Verify global components
    EXPECT_NE(defaultLogger, nullptr);
    EXPECT_NE(defaultErrorManager, nullptr);
    EXPECT_NE(globalArena, nullptr);
    
    // Clean up
    cleanup();
    
    // Re-initialize to ensure we can do it multiple times
    EXPECT_TRUE(initialize());
}

// Custom error handler for testing
static std::string lastErrorMessage;
static void testErrorHandler(
    ErrorCode code,
    ErrorSeverity severity,
    const StreamPosition& position,
    const std::string& message,
    void* userData) {
    (void)code;
    (void)severity;
    (void)position;
    (void)userData;
    
    lastErrorMessage = "Handler: " + message;
}

TEST_F(CoilLibraryTest, ErrorHandling) {
    // Initialize COIL
    EXPECT_TRUE(initialize());
    
    // Set a custom error handler
    setErrorHandler(testErrorHandler, nullptr);
    
    // Clear last error message
    lastErrorMessage.clear();
    
    // Add an error
    StreamPosition pos;
    pos.fileName = "test.cpp";
    pos.line = 42;
    pos.column = 10;
    
    defaultErrorManager->addError(ErrorCode::Memory, pos, "Test error message");
    
    // Verify error handler was called
    EXPECT_EQ(lastErrorMessage, "Handler: Test error message");
    
    // Verify last error
    std::string lastError = getLastError();
    EXPECT_FALSE(lastError.empty());
    EXPECT_THAT(lastError, HasSubstr("Memory allocation failure"));
    EXPECT_THAT(lastError, HasSubstr("Test error message"));
}

TEST_F(CoilLibraryTest, LogLevelSetting) {
    // Initialize COIL
    EXPECT_TRUE(initialize());
    
    // Get initial log level
    LogLevel initialLevel = defaultLogger->getLevel();
    
    // Set a new log level
    setLogLevel(LogLevel::Debug);
    
    // Verify log level was changed
    EXPECT_EQ(defaultLogger->getLevel(), LogLevel::Debug);
    
    // Restore log level
    setLogLevel(initialLevel);
}

TEST_F(CoilLibraryTest, SubsystemIntegration) {
    // Initialize COIL
    EXPECT_TRUE(initialize());
    
    // Test logger
    COIL_DEFAULT_INFO("Test log message");
    
    // Test error manager
    StreamPosition pos;
    pos.fileName = "test.cpp";
    defaultErrorManager->addInfo(ErrorCode::None, pos, "Test info message");
    
    // Test memory arena
    void* memory = globalArena->allocate(1024);
    EXPECT_NE(memory, nullptr);
    
    // Test thread data
    ThreadData* data = getThreadData();
    EXPECT_NE(data, nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}