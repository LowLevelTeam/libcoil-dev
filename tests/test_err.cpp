/**
 * @file test_err.cpp
 * @brief Tests for the COIL error handling
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "coil/err.hpp"
#include <string>
#include <vector>

// Helper struct to track error messages
struct ErrorRecord {
  coil::ErrorLevel level;
  std::string message;
  bool has_position;
  std::string file;
  size_t line;
  size_t index;
};

// Storage for captured errors
static std::vector<ErrorRecord> g_errors;

// Custom error callback for testing
void testErrorCallback(coil::ErrorLevel level, const std::string& message, 
                       const coil::ErrorPosition* position) {
  ErrorRecord record;
  record.level = level;
  record.message = message;
  record.has_position = (position != nullptr);
  
  if (position) {
    record.file = position->file;
    record.line = position->line;
    record.index = position->index;
  }
  
  g_errors.push_back(record);
}

TEST_CASE("Error callback registration", "[error]") {
  // Clear previous errors
  g_errors.clear();
  
  // Set our custom callback
  coil::Logger::setCallback(testErrorCallback);
  
  SECTION("Basic error logging") {
    coil::Logger::info("Test info message");
    coil::Logger::warning("Test warning message");
    
    REQUIRE(g_errors.size() == 2);
    CHECK(g_errors[0].level == coil::ErrorLevel::Info);
    CHECK(g_errors[0].message == "Test info message");
    CHECK(g_errors[0].has_position == true);
    
    CHECK(g_errors[1].level == coil::ErrorLevel::Warning);
    CHECK(g_errors[1].message == "Test warning message");
    CHECK(g_errors[1].has_position == true);
  }
  
  SECTION("Error logging with position") {
    coil::ErrorPosition pos = coil::ErrorPosition::current("test_file.cpp", 42);
    coil::Logger::warning("Test warning at position", pos);
    
    REQUIRE(g_errors.size() == 1);
    CHECK(g_errors[0].level == coil::ErrorLevel::Warning);
    CHECK(g_errors[0].message == "Test warning at position");
    CHECK(g_errors[0].has_position == true);
    CHECK(g_errors[0].file == "test_file.cpp");
    CHECK(g_errors[0].line == 42);
  }
  
  SECTION("Error exceptions") {
    // Error should throw exception
    CHECK_THROWS_AS(coil::Logger::error("Test error message"), coil::CoilException);
    
    // Fatal should throw exception
    CHECK_THROWS_AS(coil::Logger::fatal("Test fatal message"), coil::CoilException);
    
    // Check that errors were still logged
    REQUIRE(g_errors.size() == 2);
    CHECK(g_errors[0].level == coil::ErrorLevel::Error);
    CHECK(g_errors[0].message == "Test error message");
    
    CHECK(g_errors[1].level == coil::ErrorLevel::Fatal);
    CHECK(g_errors[1].message == "Test fatal message");
  }
  
  SECTION("Error macros with position") {
    // Using macros should include position information
    CHECK_THROWS_AS(COIL_ERROR("Macro error"), coil::CoilException);
    
    REQUIRE(g_errors.size() == 1);
    CHECK(g_errors[0].level == coil::ErrorLevel::Error);
    CHECK(g_errors[0].message == "Macro error");
    CHECK(g_errors[0].has_position == true);
    CHECK(g_errors[0].file == __FILE__);
  }
  
  // Reset callback
  coil::Logger::setCallback(nullptr);
}

TEST_CASE("Exception classes", "[error]") {
  // Test various exception types
  CHECK_THROWS_MATCHES(
    throw coil::InvalidArgException("test"), 
    coil::InvalidArgException, 
    Catch::Matchers::Message("Invalid argument: test")
  );
  
  CHECK_THROWS_MATCHES(
    throw coil::IOException("test"), 
    coil::IOException, 
    Catch::Matchers::Message("I/O error: test")
  );
  
  // All exceptions should derive from CoilException
  try {
    throw coil::FormatException("test");
  } catch (const coil::CoilException& e) {
    CHECK(std::string(e.what()) == "Format error: test");
  } catch (...) {
    FAIL("Exception not caught as CoilException");
  }
}