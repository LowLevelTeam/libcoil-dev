#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/err.hpp"
#include "coil/log.hpp"
#include <memory>
#include <string>

using namespace coil;
using namespace testing;

// Mock logger for testing
class MockLogger : public Logger {
public:
    MockLogger() : Logger("MOCK", nullptr, LogLevel::Trace) {}
    
    MOCK_METHOD(void, log, (LogLevel, const char*, int, const char*, const char*), ());
    MOCK_METHOD(void, log, (LogLevel, const char*, int, const char*, const char*, const char*), ());
    MOCK_METHOD(void, log, (LogLevel, const char*, int, const char*, const char*, const char*, const char*), ());
};

// Mock error handler
class MockErrorHandler {
public:
    MOCK_METHOD(void, handleError, (ErrorCode, ErrorSeverity, const StreamPosition&, const std::string&), ());
};

class ErrorManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockLogger = std::make_shared<MockLogger>();
        errorManager = ErrorManager::create(mockLogger);
        mockHandler = std::make_unique<MockErrorHandler>();
    }
    
    void TearDown() override {
        errorManager.reset();
        mockLogger.reset();
        mockHandler.reset();
    }
    
    std::shared_ptr<MockLogger> mockLogger;
    std::shared_ptr<ErrorManager> errorManager;
    std::unique_ptr<MockErrorHandler> mockHandler;
    
    // Helper to set up the mock error handler
    void setupMockHandler() {
        errorManager->setErrorHandler(
            [this](ErrorCode code, ErrorSeverity severity, const StreamPosition& pos, const std::string& msg, void*) {
                mockHandler->handleError(code, severity, pos, msg);
            }, 
            nullptr
        );
    }
};

TEST_F(ErrorManagerTest, InitialStateHasNoErrors) {
    EXPECT_FALSE(errorManager->hasErrors());
    EXPECT_FALSE(errorManager->getLastError().has_value());
    EXPECT_TRUE(errorManager->getAllErrors().empty());
}

TEST_F(ErrorManagerTest, AddErrorStoresCorrectly) {
    StreamPosition pos = createStreamPosition("test.cpp", 42, 10, 1000);
    errorManager->addError(ErrorCode::Syntax, pos, "Test error");
    
    EXPECT_TRUE(errorManager->hasErrors());
    
    auto lastError = errorManager->getLastError();
    ASSERT_TRUE(lastError.has_value());
    EXPECT_EQ(lastError->getCode(), ErrorCode::Syntax);
    EXPECT_EQ(lastError->getSeverity(), ErrorSeverity::Error);
    EXPECT_EQ(lastError->getMessage(), "Test error");
    EXPECT_EQ(lastError->getPosition().fileName, "test.cpp");
    EXPECT_EQ(lastError->getPosition().line, 42);
}

TEST_F(ErrorManagerTest, ClearErrorsWorks) {
    errorManager->addError(ErrorCode::Syntax, StreamPosition(), "Test error");
    EXPECT_TRUE(errorManager->hasErrors());
    
    errorManager->clearErrors();
    EXPECT_FALSE(errorManager->hasErrors());
    EXPECT_FALSE(errorManager->getLastError().has_value());
    EXPECT_TRUE(errorManager->getAllErrors().empty());
}

TEST_F(ErrorManagerTest, HasErrorsLevelFiltering) {
    // Add a warning
    errorManager->addWarning(ErrorCode::Format, StreamPosition(), "Warning");
    
    // Should not have errors at Error level
    EXPECT_FALSE(errorManager->hasErrors(ErrorSeverity::Error));
    
    // Should have errors at Warning level
    EXPECT_TRUE(errorManager->hasErrors(ErrorSeverity::Warning));
    
    // Add an error
    errorManager->addError(ErrorCode::Syntax, StreamPosition(), "Error");
    
    // Now should have errors at Error level
    EXPECT_TRUE(errorManager->hasErrors(ErrorSeverity::Error));
}

TEST_F(ErrorManagerTest, ErrorHandlerGetsInvoked) {
    setupMockHandler();
    
    StreamPosition pos = createStreamPosition("test.cpp", 42, 10, 1000);
    
    // Expect the handler to be called exactly once
    EXPECT_CALL(*mockHandler, handleError(
        ErrorCode::Syntax, 
        ErrorSeverity::Error, 
        AllOf(
            Field(&StreamPosition::fileName, Eq("test.cpp")),
            Field(&StreamPosition::line, Eq(42))
        ),
        "Test error"
    )).Times(1);
    
    errorManager->addError(ErrorCode::Syntax, pos, "Test error");
}

TEST_F(ErrorManagerTest, GetAllErrorsReturnsAllErrors) {
    errorManager->addWarning(ErrorCode::Format, StreamPosition(), "Warning 1");
    errorManager->addError(ErrorCode::Syntax, StreamPosition(), "Error 1");
    errorManager->addWarning(ErrorCode::Reference, StreamPosition(), "Warning 2");
    
    auto errors = errorManager->getAllErrors();
    EXPECT_EQ(errors.size(), 3);
    
    // Check first error
    EXPECT_EQ(errors[0].getCode(), ErrorCode::Format);
    EXPECT_EQ(errors[0].getSeverity(), ErrorSeverity::Warning);
    EXPECT_EQ(errors[0].getMessage(), "Warning 1");
    
    // Check second error
    EXPECT_EQ(errors[1].getCode(), ErrorCode::Syntax);
    EXPECT_EQ(errors[1].getSeverity(), ErrorSeverity::Error);
    EXPECT_EQ(errors[1].getMessage(), "Error 1");
    
    // Check third error
    EXPECT_EQ(errors[2].getCode(), ErrorCode::Reference);
    EXPECT_EQ(errors[2].getSeverity(), ErrorSeverity::Warning);
    EXPECT_EQ(errors[2].getMessage(), "Warning 2");
}

TEST_F(ErrorManagerTest, ErrorCodeToMessageWorks) {
    EXPECT_EQ(getErrorMessage(ErrorCode::Memory), "Memory allocation failure");
    EXPECT_EQ(getErrorMessage(ErrorCode::IO), "I/O error");
    EXPECT_EQ(getErrorMessage(ErrorCode::Syntax), "Syntax error");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}