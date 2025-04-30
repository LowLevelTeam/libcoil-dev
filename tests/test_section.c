/**
* @file test_section.c
* @brief Test suite for section functionality
*
* @author Low Level Team
*/

#include <coil/sect.h>
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

#define TEST_SECTION_FILE "test_section.dat"

/**
* @brief Test section initialization and cleanup
*/
static int test_section_init_cleanup() {
  printf("  Testing section init/cleanup...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  TEST_ASSERT(sect.data != NULL, "Section data should be allocated");
  TEST_ASSERT(sect.capacity == 1024, "Section capacity should match");
  TEST_ASSERT(sect.size == 0, "Section size should be zero");
  TEST_ASSERT(sect.mode == COIL_SECT_MODE_CREATE, "Section mode should be CREATE");
  
  // Clean up the section
  coil_section_cleanup(&sect);
  TEST_ASSERT(sect.data == NULL, "Section data should be NULL after cleanup");
  
  // Test with zero capacity (should use default)
  err = coil_section_init(&sect, 0);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization with default capacity should succeed");
  TEST_ASSERT(sect.data != NULL, "Section data should be allocated");
  TEST_ASSERT(sect.capacity > 0, "Section capacity should be positive");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test section read/write operations
*/
static int test_section_read_write() {
  printf("  Testing section read/write...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write data to the section
  const char *test_data = "Hello, COIL section!";
  coil_size_t test_len = strlen(test_data);
  coil_size_t bytes_written;
  
  err = coil_section_write(&sect, (coil_byte_t *)test_data, test_len, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
  TEST_ASSERT(bytes_written == test_len, "Should write all bytes");
  TEST_ASSERT(sect.size == test_len, "Section size should match written bytes");
  TEST_ASSERT(sect.windex == test_len, "Write index should be updated");
  
  // Reset read index
  err = coil_section_seek_read(&sect, 0);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Seek read should succeed");
  
  // Read data back
  char read_buffer[100] = {0};
  coil_size_t bytes_read;
  
  err = coil_section_read(&sect, (coil_byte_t *)read_buffer, sizeof(read_buffer), &bytes_read);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section read should succeed");
  TEST_ASSERT(bytes_read == test_len, "Should read same number of bytes");
  TEST_ASSERT(strcmp(read_buffer, test_data) == 0, "Read data should match written data");
  TEST_ASSERT(sect.rindex == test_len, "Read index should be updated");
  
  // Test resizing (write more data than capacity)
  char *large_data = (char *)malloc(2048);
  if (large_data) {
    memset(large_data, 'X', 2048);
    
    // Reset write index
    err = coil_section_seek_write(&sect, 0);
    TEST_ASSERT(err == COIL_ERR_GOOD, "Seek write should succeed");
    
    // Write large data
    err = coil_section_write(&sect, (coil_byte_t *)large_data, 2048, &bytes_written);
    TEST_ASSERT(err == COIL_ERR_GOOD, "Section write (resize) should succeed");
    TEST_ASSERT(bytes_written == 2048, "Should write all bytes");
    TEST_ASSERT(sect.capacity >= 2048, "Section capacity should be increased");
    
    free(large_data);
  }
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test section string operations
*/
static int test_section_string_ops() {
  printf("  Testing section string operations...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Test putstr
  err = coil_section_putstr(&sect, "First string");
  TEST_ASSERT(err == COIL_ERR_GOOD, "putstr should succeed");
  
  // Add another string
  err = coil_section_putstr(&sect, "Second string");
  TEST_ASSERT(err == COIL_ERR_GOOD, "putstr (second) should succeed");
  
  // Get strings by offset
  const char *str1;
  err = coil_section_getstr(&sect, 0, &str1);
  TEST_ASSERT(err == COIL_ERR_GOOD, "getstr (first) should succeed");
  TEST_ASSERT(strcmp(str1, "First string") == 0, "First string should match");
  
  const char *str2;
  err = coil_section_getstr(&sect, strlen("First string") + 1, &str2);
  TEST_ASSERT(err == COIL_ERR_GOOD, "getstr (second) should succeed");
  TEST_ASSERT(strcmp(str2, "Second string") == 0, "Second string should match");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test section native code metadata
*/
static int test_section_native() {
  printf("  Testing section native code metadata...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write some "native code" to the section
  const char *code = "This is fake native code";
  coil_size_t code_len = strlen(code);
  coil_size_t bytes_written;
  
  // TODO...
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test section file I/O
*/
static int test_section_file_io() {
  printf("  Testing section file I/O...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write data to the section
  const char *test_data = "Hello, COIL section file I/O!";
  coil_size_t test_len = strlen(test_data);
  coil_size_t bytes_written;
  
  err = coil_section_write(&sect, (coil_byte_t *)test_data, test_len, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
  
  // Open a file
  int fd = open(TEST_SECTION_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Serialize the section to the file
  err = coil_section_serialize(&sect, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section serialization should succeed");
  
  // Close the file
  close(fd);
  
  // Open the file again for reading
  fd = open(TEST_SECTION_FILE, O_RDONLY);
  TEST_ASSERT(fd >= 0, "File open for reading should succeed");
  
  // Load a new section from the file
  coil_section_t loaded_sect;
  err = coil_section_load(&loaded_sect, 1024, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section loading should succeed");
  TEST_ASSERT(loaded_sect.size == test_len, "Loaded section size should match");
  TEST_ASSERT(memcmp(loaded_sect.data, test_data, test_len) == 0, "Loaded data should match");
  
  // Close the file
  close(fd);
  
  // Clean up
  coil_section_cleanup(&sect);
  coil_section_cleanup(&loaded_sect);
  
  return 0;
}

/**
* @brief Run all section tests
*/
int test_section() {
  printf("\nRunning section tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_section_init_cleanup();
  result |= test_section_read_write();
  result |= test_section_string_ops();
  result |= test_section_native();
  result |= test_section_file_io();
  
  // Clean up test file
  unlink(TEST_SECTION_FILE);
  
  if (result == 0) {
    printf("All section tests passed!\n");
  }
  
  return result;
}