#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/log.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using namespace coil;
using namespace testing;

// Custom FILE* wrapper for testing
class TestStream {
public:
    TestStream() {
        file_ = tmpfile();
    }
    
    ~TestStream() {
        if (file_) {
            fclose(file_);
        }
    }
    
    FILE* get() {
        return file_;
    }
    
    std::string getContents() {
        if (!file_) return "";
        
        long pos = ftell(file_);
        rewind(file_);
        
        std::string content;
        char buffer[1024];
        size_t bytesRead;
        
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file_)) > 0) {
            content.append(buffer, bytesRead);
        }
        
        fseek(file_, pos, SEEK_SET);
        return content;
    }
    
    void clear() {
        if (file_) {
            freopen(NULL, "w+", file_);
        }
    }

private:
    FILE* file_ = nullptr;
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        stream_ = std::make_unique<TestStream>();
        logger_ = Logger::create("TEST", stream_->get(), LogLevel::Trace);
        logger_->setColoredOutput(false); // Disable colors for easier testing
    }

    void TearDown() override {
        logger_.reset();
        stream_.reset();
    }

    std::unique_ptr<TestStream> stream_;
    std::shared_ptr<Logger> logger_;
};

TEST_F(LoggerTest, InitializesWithCorrectLevel) {
    EXPECT_EQ(logger_->getLevel(), LogLevel::Trace);
}

TEST_F(LoggerTest, LogLevelFiltering) {
    // Should log at Trace level
    COIL_TRACE(logger_, "Trace message");
    std::string content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("Trace message"));
    
    // Change level to Info and verify Trace/Debug are filtered
    logger_->setLevel(LogLevel::Info);
    stream_->clear();
    
    COIL_TRACE(logger_, "Filtered trace");
    COIL_DEBUG(logger_, "Filtered debug");
    COIL_INFO(logger_, "Info passes");
    
    content = stream_->getContents();
    EXPECT_THAT(content, Not(HasSubstr("Filtered trace")));
    EXPECT_THAT(content, Not(HasSubstr("Filtered debug")));
    EXPECT_THAT(content, HasSubstr("Info passes"));
}

TEST_F(LoggerTest, LogsCorrectPrefix) {
    COIL_INFO(logger_, "Test message");
    std::string content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("[TEST]"));
}

TEST_F(LoggerTest, LogsLevelName) {
    COIL_INFO(logger_, "Info message");
    std::string content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("[INFO]"));
    
    stream_->clear();
    COIL_ERROR(logger_, "Error message");
    content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("[ERROR]"));
}

TEST_F(LoggerTest, DefaultLoggerWorks) {
    // Save old default logger and restore at end
    auto oldDefaultLogger = defaultLogger;
    
    // Create a test default logger
    auto testStream = std::make_unique<TestStream>();
    defaultLogger = Logger::create("DEFAULT", testStream->get(), LogLevel::Info);
    defaultLogger->setColoredOutput(false);
    
    COIL_DEFAULT_INFO("Default logger test");
    std::string content = testStream->getContents();
    EXPECT_THAT(content, HasSubstr("Default logger test"));
    
    // Restore
    defaultLogger = oldDefaultLogger;
}

TEST_F(LoggerTest, FormatsIntegersCorrectly) {
    COIL_INFO(logger_, "Integer: %d", 42);
    std::string content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("Integer: 42"));
}

TEST_F(LoggerTest, FormatsStringsCorrectly) {
    COIL_INFO(logger_, "String: %s", "test string");
    std::string content = stream_->getContents();
    EXPECT_THAT(content, HasSubstr("String: test string"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}