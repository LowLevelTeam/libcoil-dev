/**
* @file test_stream.cpp
* @brief Tests for the COIL stream classes
*/

#include <catch2/catch_test_macros.hpp>
#include "coil/stream.hpp"
#include <cstring>
#include <string>
#include <vector>

// Test data to use in streams
const char TEST_DATA[] = "COIL stream test data 12345";
const size_t TEST_DATA_SIZE = sizeof(TEST_DATA) - 1; // Exclude null terminator

TEST_CASE("Memory Stream Basic Operations", "[stream]") {
  SECTION("Creating with allocated buffer") {
      coil::MemoryStream stream(nullptr, 1024, coil::StreamMode::ReadWrite);
      
      // Check initial state
      CHECK(stream.tell() == 0);
      CHECK(stream.eof() == true); // No data written yet
      CHECK(stream.getSize() == 0);
      CHECK(stream.getBuffer() != nullptr);
  }
  
  SECTION("Writing and reading") {
      coil::MemoryStream stream(nullptr, 1024, coil::StreamMode::ReadWrite);
      
      // Write test data
      size_t written = stream.write(TEST_DATA, TEST_DATA_SIZE);
      CHECK(written == TEST_DATA_SIZE);
      CHECK(stream.getSize() == TEST_DATA_SIZE);
      
      // Read it back
      char buffer[100] = {0};
      
      // First seek back to beginning
      CHECK(stream.seek(0) == coil::Result::Success);
      CHECK(stream.tell() == 0);
      
      // Now read
      size_t read = stream.read(buffer, sizeof(buffer));
      CHECK(read == TEST_DATA_SIZE);
      CHECK(std::string(buffer) == TEST_DATA);
      
      // Should be at EOF now
      CHECK(stream.eof() == true);
  }
  
  SECTION("Using provided buffer") {
      // Create a buffer with some data
      char buffer[100];
      std::memcpy(buffer, TEST_DATA, TEST_DATA_SIZE);
      
      // Create a read-only stream with the buffer
      coil::MemoryStream stream(buffer, TEST_DATA_SIZE, coil::StreamMode::Read);
      
      // Check initial state
      CHECK(stream.tell() == 0);
      CHECK(stream.eof() == false);
      CHECK(stream.getSize() == TEST_DATA_SIZE);
      
      // Read the data
      char read_buffer[100] = {0};
      size_t read = stream.read(read_buffer, sizeof(read_buffer));
      
      CHECK(read == TEST_DATA_SIZE);
      CHECK(std::string(read_buffer) == TEST_DATA);
      CHECK(stream.eof() == true);
      
      // Seeking past end should fail
      CHECK(stream.seek(TEST_DATA_SIZE + 10) == coil::Result::InvalidArg);
  }
  
  SECTION("Write-only mode") {
      coil::MemoryStream stream(nullptr, 1024, coil::StreamMode::Write);
      
      // Write should work
      size_t written = stream.write(TEST_DATA, TEST_DATA_SIZE);
      CHECK(written == TEST_DATA_SIZE);
      
      // Read should fail
      char buffer[100] = {0};
      CHECK(stream.seek(0) == coil::Result::Success);
      size_t read = stream.read(buffer, sizeof(buffer));
      CHECK(read == 0);
  }
  
  SECTION("Read-only mode") {
      // Create a buffer with some data
      char buffer[100];
      std::memcpy(buffer, TEST_DATA, TEST_DATA_SIZE);
      
      // Create a read-only stream with the buffer
      coil::MemoryStream stream(buffer, TEST_DATA_SIZE, coil::StreamMode::Read);
      
      // Write should fail
      size_t written = stream.write("new data", 8);
      CHECK(written == 0);
  }
  
  SECTION("Typed I/O") {
      coil::MemoryStream stream(nullptr, 1024, coil::StreamMode::ReadWrite);
      
      // Write some typed values
      int ints[] = {1, 2, 3, 4, 5};
      for (int i : ints) {
          CHECK(stream.writeValue(i) == coil::Result::Success);
      }
      
      // Seek back to start
      CHECK(stream.seek(0) == coil::Result::Success);
      
      // Read the values back
      for (int expected : ints) {
          int value;
          CHECK(stream.readValue(value) == coil::Result::Success);
          CHECK(value == expected);
      }
  }
}

TEST_CASE("File Stream Operations", "[stream]") {
  const char* test_filename = "test_stream.tmp";
  
  SECTION("Creating file for writing") {
      // Create a file stream for writing
      {
          coil::FileStream stream(test_filename, coil::StreamMode::Write);
          
          // Write test data
          size_t written = stream.write(TEST_DATA, TEST_DATA_SIZE);
          CHECK(written == TEST_DATA_SIZE);
          
          // Write typed data
          int ints[] = {1, 2, 3, 4, 5};
          for (int i : ints) {
              CHECK(stream.writeValue(i) == coil::Result::Success);
          }
          
          // Stream closes automatically at end of scope
      }
      
      // Now read it back
      {
          coil::FileStream stream(test_filename, coil::StreamMode::Read);
          
          // Read the string data
          char buffer[100] = {0};
          size_t read = stream.read(buffer, TEST_DATA_SIZE);
          CHECK(read == TEST_DATA_SIZE);
          CHECK(std::string(buffer, TEST_DATA_SIZE) == TEST_DATA);
          
          // Read the typed data
          int ints[5];
          for (int& i : ints) {
              CHECK(stream.readValue(i) == coil::Result::Success);
          }
          int sixth;
          CHECK(stream.readValue(sixth) == coil::Result::IoError); // reading beyond causes EOF (Maybe change this)
          
          CHECK(ints[0] == 1);
          CHECK(ints[1] == 2);
          CHECK(ints[2] == 3);
          CHECK(ints[3] == 4);
          CHECK(ints[4] == 5);
          
          // After reading all data, should be at EOF
          CHECK(stream.eof() == true);
      }
      
      // Clean up
      std::remove(test_filename);
  }
  
  SECTION("Reading non-existent file") {
      coil::FileStream stream("non_existent_file.tmp", coil::StreamMode::Read);
      
      // The handle should be null
      char buffer[10];
      size_t read = stream.read(buffer, sizeof(buffer));
      CHECK(read == 0);
      CHECK(stream.eof() == true);
  }
}