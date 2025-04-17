/**
 * @file test_stream.cpp
 * @brief Tests for the COIL stream classes
 */

#include <catch2/catch_all.hpp>
#include "coil/stream.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>

using namespace Catch;

// Test data to use in streams
const char TEST_DATA[] = "COIL stream test data 12345";
const size_t TEST_DATA_SIZE = sizeof(TEST_DATA) - 1; // Exclude null terminator

TEST_CASE("Memory Stream Basic Operations", "[stream]") {
  SECTION("Creating with allocated buffer") {
    coil::MemoryStream stream(1024);
    
    // Check initial state
    CHECK(stream.tell() == 0);
    CHECK(stream.eof() == true); // No data written yet
    CHECK(stream.size() == 0);
    CHECK(stream.getBufferPointer() != nullptr);
  }
  
  SECTION("Writing and reading") {
    coil::MemoryStream stream(1024);
    
    // Write test data
    size_t written = stream.write(TEST_DATA, TEST_DATA_SIZE);
    CHECK(written == TEST_DATA_SIZE);
    CHECK(stream.size() == TEST_DATA_SIZE);
    
    // Read it back
    char buffer[100] = {0};
    
    // First seek back to beginning
    stream.seek(0);
    CHECK(stream.tell() == 0);
    
    // Now read
    size_t read = stream.read(buffer, sizeof(buffer));
    CHECK(read == TEST_DATA_SIZE);
    CHECK(std::string(buffer, read) == TEST_DATA);
    
    // Should be at EOF now
    CHECK(stream.eof() == true);
  }
  
  SECTION("Writing and reading typed values") {
    coil::MemoryStream stream(1024);
    
    // Write typed values
    stream.writev<int>(42);
    stream.writev<float>(3.14f);
    stream.writev<double>(2.71828);
    
    // Seek back to start
    stream.seek(0);
    
    // Read values back
    int i = stream.readv<int>();
    float f = stream.readv<float>();
    double d = stream.readv<double>();
    
    CHECK(i == 42);
    CHECK(f == Approx(3.14f));
    CHECK(d == Approx(2.71828));
  }
  
  SECTION("Vector data") {
    // Create from vector
    std::vector<coil::u8> data(TEST_DATA, TEST_DATA + TEST_DATA_SIZE);
    coil::MemoryStream stream(data);
    
    // Read data back
    std::vector<coil::u8> readData(TEST_DATA_SIZE);
    stream.read(readData.data(), readData.size());
    
    // Check data
    CHECK(readData == data);
    
    // Get data as vector
    std::vector<coil::u8> retrievedData = stream.getData();
    CHECK(retrievedData == data);
  }
  
  SECTION("Resizing") {
    coil::MemoryStream stream(10);
    
    // Check initial capacity
    CHECK(stream.getCapacity() == 10);
    
    // Write more than capacity
    std::string longData(20, 'X');
    stream.write(longData.c_str(), longData.size());
    
    // Capacity should have increased
    CHECK(stream.getCapacity() >= 20);
    
    // Data should be fully written
    CHECK(stream.size() == 20);
  }
  
  SECTION("Invalid operations") {
    coil::MemoryStream stream(1024);
    
    // Seeking beyond capacity should throw
    CHECK_THROWS_AS(stream.seek(2000), coil::InvalidArgException);
    
    // Seeking to negative position should throw
    CHECK_THROWS_AS(stream.seekRelative(coil::SeekOrigin::Current, -100), coil::InvalidArgException);
  }
}

TEST_CASE("File Stream Operations", "[stream]") {
  const std::string test_filename = "test_stream.tmp";
  
  // Clean up any existing file
  std::filesystem::remove(test_filename);
  
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
        stream.writev(i);
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
      for (int i = 0; i < 5; i++) {
        int value = stream.readv<int>();
        CHECK(value == i + 1);
      }
      
      // Check EOF
      CHECK_THROWS_AS(stream.readv<int>(), coil::IOException);
      CHECK(stream.eof() == true);
    }
    
    // Clean up
    std::filesystem::remove(test_filename);
  }
  
  SECTION("Reading non-existent file") {
    CHECK_THROWS_AS(
      coil::FileStream("non_existent_file.tmp", coil::StreamMode::Read),
      coil::IOException
    );
  }
  
  SECTION("File size and seek") {
    // Create a file with content
    {
      coil::FileStream stream(test_filename, coil::StreamMode::Write);
      stream.write(TEST_DATA, TEST_DATA_SIZE);
    }
    
    // Check size and seek operations
    {
      coil::FileStream stream(test_filename, coil::StreamMode::Read);
      
      // Check size
      CHECK(stream.size() == TEST_DATA_SIZE);
      
      // Seek to middle
      stream.seek(TEST_DATA_SIZE / 2);
      CHECK(stream.tell() == TEST_DATA_SIZE / 2);
      
      // Seek relative
      stream.seekRelative(coil::SeekOrigin::Current, 5);
      CHECK(stream.tell() == ((TEST_DATA_SIZE / 2) + 5));
      
      // Seek from end
      stream.seekRelative(coil::SeekOrigin::End, -5);
      CHECK(stream.tell() == TEST_DATA_SIZE - 5);
    }
    
    // Clean up
    std::filesystem::remove(test_filename);
  }
}

TEST_CASE("String operations", "[stream]") {
  SECTION("Write and read strings") {
    coil::MemoryStream stream(1024);
    
    // Write null-terminated string
    const char* str1 = "Hello, world!";
    size_t written = stream.writeString(str1);
    CHECK(written == strlen(str1) + 1); // +1 for null terminator
    
    // Write another string
    const char* str2 = "Another string";
    written = stream.writeString(str2);
    CHECK(written == strlen(str2) + 1);
    
    // Seek back to beginning
    stream.seek(0);
    
    // Read first string
    char buffer[100] = {0};
    size_t read = stream.readString(buffer, sizeof(buffer));
    CHECK(read == strlen(str1) + 1);
    CHECK(std::string(buffer) == str1);
    
    // Read second string
    read = stream.readString(buffer, sizeof(buffer));
    CHECK(read == strlen(str2) + 1);
    CHECK(std::string(buffer) == str2);
  }
}