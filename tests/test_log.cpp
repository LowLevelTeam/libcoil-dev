#include <catch2/catch_all.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include "coil/log.hpp"

class CaptureBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 4096;
    
    CaptureBuffer() {
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    ~CaptureBuffer() {
        if (fp) fclose(fp);
    }
    
    FILE* getFile() const { return fp; }
    
    const char* getBuffer() const { return buffer; }
    
    void clear() {
        fclose(fp);
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    bool contains(const std::string& str) const {
        return strstr(buffer, str.c_str()) != nullptr;
    }
    
private:
    char buffer[BUFFER_SIZE];
    FILE* fp;
};

TEST_CASE("Logger configuration and levels", "[log]") {
    CaptureBuffer capture;
    
    SECTION("Logger initialization") {
        coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
        
        REQUIRE(logger.getLevel() == coil::LogLevel::Info);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Info) == true);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Debug) == false);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Trace) == false);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Warning) == true);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Error) == true);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Fatal) == true);
    }
    
    SECTION("Changing log level") {
        coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
        
        logger.setLevel(coil::LogLevel::Debug);
        REQUIRE(logger.getLevel() == coil::LogLevel::Debug);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Debug) == true);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Trace) == false);
        
        logger.setLevel(coil::LogLevel::Trace);
        REQUIRE(logger.getLevel() == coil::LogLevel::Trace);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Trace) == true);
        
        logger.setLevel(coil::LogLevel::Error);
        REQUIRE(logger.getLevel() == coil::LogLevel::Error);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Info) == false);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Warning) == false);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Error) == true);
        REQUIRE(logger.isLevelEnabled(coil::LogLevel::Fatal) == true);
    }
}

TEST_CASE("Logger message formatting", "[log]") {
    CaptureBuffer capture;
    coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Trace, false);
    
    SECTION("Log messages with different levels") {
        logger.log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, "Info message");
        REQUIRE(capture.contains("[INFO]"));
        REQUIRE(capture.contains("[TEST]"));
        REQUIRE(capture.contains("Info message"));
        
        capture.clear();
        logger.log(coil::LogLevel::Warning, __FILE__, __LINE__, __func__, "Warning message");
        REQUIRE(capture.contains("[WARNING]"));
        REQUIRE(capture.contains("Warning message"));
        
        capture.clear();
        logger.log(coil::LogLevel::Error, __FILE__, __LINE__, __func__, "Error: %s", "custom error");
        REQUIRE(capture.contains("[ERROR]"));
        REQUIRE(capture.contains("Error: custom error"));
    }
    
    SECTION("Log message with arguments") {
        logger.log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, 
                  "Integer: %d, String: %s, Float: %.2f", 
                  42, "test string", 3.14159);
        
        REQUIRE(capture.contains("Integer: 42"));
        REQUIRE(capture.contains("String: test string"));
        REQUIRE(capture.contains("Float: 3.14"));
    }
    
    SECTION("Log level filtering") {
        logger.setLevel(coil::LogLevel::Warning);
        
        // This shouldn't appear in output
        logger.log(coil::LogLevel::Info, __FILE__, __LINE__, __func__, "Should be filtered");
        REQUIRE_FALSE(capture.contains("Should be filtered"));
        
        // This should appear
        logger.log(coil::LogLevel::Error, __FILE__, __LINE__, __func__, "Should be logged");
        REQUIRE(capture.contains("Should be logged"));
    }
}

TEST_CASE("Logger convenience macros", "[log]") {
    CaptureBuffer capture;
    coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Trace, false);
    
    SECTION("COIL_INFO macro") {
        COIL_INFO(logger, "Info message via macro");
        REQUIRE(capture.contains("[INFO]"));
        REQUIRE(capture.contains("Info message via macro"));
    }
    
    SECTION("COIL_WARNING macro") {
        COIL_WARNING(logger, "Warning message via macro");
        REQUIRE(capture.contains("[WARNING]"));
        REQUIRE(capture.contains("Warning message via macro"));
    }
    
    SECTION("COIL_ERROR macro") {
        COIL_ERROR(logger, "Error message via macro");
        REQUIRE(capture.contains("[ERROR]"));
        REQUIRE(capture.contains("Error message via macro"));
    }
    
    SECTION("COIL_FATAL macro") {
        COIL_FATAL(logger, "Fatal message via macro");
        REQUIRE(capture.contains("[FATAL]"));
        REQUIRE(capture.contains("Fatal message via macro"));
    }
    
#ifndef NDEBUG
    SECTION("Debug macros in debug mode") {
        COIL_DEBUG(logger, "Debug message via macro");
        REQUIRE(capture.contains("[DEBUG]"));
        REQUIRE(capture.contains("Debug message via macro"));
        
        capture.clear();
        COIL_TRACE(logger, "Trace message via macro");
        REQUIRE(capture.contains("[TRACE]"));
        REQUIRE(capture.contains("Trace message via macro"));
    }
#else
    SECTION("Debug macros in release mode") {
        COIL_DEBUG(logger, "Debug message via macro");
        REQUIRE_FALSE(capture.contains("[DEBUG]"));
        
        COIL_TRACE(logger, "Trace message via macro");
        REQUIRE_FALSE(capture.contains("[TRACE]"));
    }
#endif
}