/**
* @file test_file.c
* @brief Test suite for file I/O functionality
*
* @author Low Level Team
*/

#include <coil/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

#define TEST_FILE_PATH "test_file.dat"

/**
* @brief Test file open and close
*/
static int test_file_open_close() {
  printf("  Testing file open/close...\n");
  
  // Open a file for writing
  coil_descriptor_t fd;
  fd = open(TEST_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Close the file
  TEST_ASSERT(coil_close(fd) == COIL_ERR_GOOD, "File close should succeed");
  
  // Test handling invalid file descriptor
  TEST_ASSERT(coil_close(-1) == COIL_ERR_INVAL, "Close should fail with invalid descriptor");
  
  return 0;
}

/**
* @brief Test file read and write
*/
static int test_file_read_write() {
  printf("  Testing file read/write...\n");
  
  const char *test_data = "Hello, COIL file system!";
  coil_size_t test_len = strlen(test_data);
  
  // Open a file for writing
  coil_descriptor_t fd = open(TEST_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Write to the file
  coil_size_t bytes_written;
  coil_err_t err = coil_write(fd, (const coil_byte_t *)test_data, test_len, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Write should succeed");
  TEST_ASSERT(bytes_written == test_len, "Should write all bytes");
  
  // Seek to the beginning
  err = coil_seek(fd, 0, SEEK_SET);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Seek should succeed");
  
  // Read the data back
  char read_buffer[100] = {0};
  coil_size_t bytes_read;
  err = coil_read(fd, (coil_byte_t *)read_buffer, sizeof(read_buffer), &bytes_read);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Read should succeed");
  TEST_ASSERT(bytes_read == test_len, "Should read same number of bytes");
  TEST_ASSERT(strcmp(read_buffer, test_data) == 0, "Read data should match written data");
  
  // Close the file
  TEST_ASSERT(coil_close(fd) == COIL_ERR_GOOD, "File close should succeed");
  
  return 0;
}

/**
* @brief Test file seek
*/
static int test_file_seek() {
  printf("  Testing file seek...\n");
  
  // Open the test file
  coil_descriptor_t fd = open(TEST_FILE_PATH, O_RDWR, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Seek to various positions
  TEST_ASSERT(coil_seek(fd, 5, SEEK_SET) == COIL_ERR_GOOD, "Seek from start should succeed");
  TEST_ASSERT(coil_seek(fd, 2, SEEK_CUR) == COIL_ERR_GOOD, "Seek from current should succeed");
  TEST_ASSERT(coil_seek(fd, -1, SEEK_END) == COIL_ERR_GOOD, "Seek from end should succeed");
  
  // Read a byte to verify position
  char buf[2] = {0};
  coil_size_t bytes_read;
  coil_err_t err = coil_read(fd, (coil_byte_t *)buf, 1, &bytes_read);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Read should succeed");
  TEST_ASSERT(bytes_read == 1, "Should read one byte");
  
  // Close the file
  TEST_ASSERT(coil_close(fd) == COIL_ERR_GOOD, "File close should succeed");
  
  return 0;
}

/**
* @brief Run all file I/O tests
*/
int test_file() {
  printf("\nRunning file I/O tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_file_open_close();
  result |= test_file_read_write();
  result |= test_file_seek();
  
  // Clean up test file
  unlink(TEST_FILE_PATH);
  
  if (result == 0) {
    printf("All file I/O tests passed!\n");
  }
  
  return result;
}