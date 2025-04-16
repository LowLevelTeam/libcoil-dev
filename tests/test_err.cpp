/**
* @file test_err.cpp
* @brief Tests for the COIL error handling
*/

#include <catch2/catch_test_macros.hpp>
#include "coil/err.hpp"
#include <string>
#include <vector>

// Helper struct to track error messages
struct ErrorRecord {
  coil::ErrorLevel level;
  std::string message;
  bool has_position;
  std::string file;
  size_t index;
};

// Custom error callback for testing
std::vector<ErrorRecord> g_errors;

void testErrorCallback(coil::ErrorLevel level, const char* message, 
                    const coil::ErrorPosition* position, void* user_data) {
  ErrorRecord record;
  record.level = level;
  record.message = message;
  record.has_position = (position != nullptr);
  
  if (position) {
      record.file = position->file;
      record.index = position->index;
  }
  
  g_errors.push_back(record);
}

TEST_CASE("Error callback registration", "[error]") {
  // Clear previous errors
  g_errors.clear();
  
  // Set our custom callback
  coil::setErrorCallback(testErrorCallback, nullptr);
  
  SECTION("Basic error reporting") {
      coil::reportError(coil::ErrorLevel::Warning, "Test warning message");
      
      REQUIRE(g_errors.size() == 1);
      CHECK(g_errors[0].level == coil::ErrorLevel::Warning);
      CHECK(g_errors[0].message == "Test warning message");
      CHECK(g_errors[0].has_position == false);
  }
  
  SECTION("Error reporting with position") {
      coil::ErrorPosition pos = {"test_file.cpp", 42};
      coil::reportErrorWithPos(coil::ErrorLevel::Error, &pos, "Test error at position");
      
      REQUIRE(g_errors.size() == 1);
      CHECK(g_errors[0].level == coil::ErrorLevel::Error);
      CHECK(g_errors[0].message == "Test error at position");
      CHECK(g_errors[0].has_position == true);
      CHECK(g_errors[0].file == "test_file.cpp");
  }
  
  SECTION("Multiple error levels") {
      coil::reportError(coil::ErrorLevel::Info, "Info message");
      coil::reportError(coil::ErrorLevel::Warning, "Warning message");
      coil::reportError(coil::ErrorLevel::Error, "Error message");
      coil::reportError(coil::ErrorLevel::Fatal, "Fatal message");
      
      REQUIRE(g_errors.size() == 4);
      CHECK(g_errors[0].level == coil::ErrorLevel::Info);
      CHECK(g_errors[1].level == coil::ErrorLevel::Warning);
      CHECK(g_errors[2].level == coil::ErrorLevel::Error);
      CHECK(g_errors[3].level == coil::ErrorLevel::Fatal);
  }
  
  SECTION("Making errors with result codes") {
      coil::Result result = coil::makeError(coil::Result::InvalidArg, 
                                            coil::ErrorLevel::Error, 
                                            "Invalid argument: %s", "test");
      
      REQUIRE(g_errors.size() == 1);
      CHECK(result == coil::Result::InvalidArg);
      CHECK(g_errors[0].level == coil::ErrorLevel::Error);
      CHECK(g_errors[0].message == "Invalid argument: test");
  }
}

TEST_CASE("Result code string conversion", "[error]") {
  CHECK(std::string(coil::resultToString(coil::Result::Success)) == "Success");
  CHECK(std::string(coil::resultToString(coil::Result::InvalidArg)) == "Invalid Argument");
  CHECK(std::string(coil::resultToString(coil::Result::OutOfMemory)) == "Out of Memory");
  CHECK(std::string(coil::resultToString(coil::Result::IoError)) == "I/O Error");
  CHECK(std::string(coil::resultToString(coil::Result::InvalidFormat)) == "Invalid Format");
  CHECK(std::string(coil::resultToString(coil::Result::NotFound)) == "Not Found");
  CHECK(std::string(coil::resultToString(coil::Result::NotSupported)) == "Not Supported");
}