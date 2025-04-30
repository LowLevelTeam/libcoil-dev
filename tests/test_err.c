/**
* @file test_err.c
* @brief Test suite for error handling functionality
*
* @author Low Level Team
*/

#include <coil/err.h>
#include <stdio.h>
#include <string.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

#define TEST_ASSERT_STR_EQ(s1, s2, msg) do { \
  if (strcmp(s1, s2) != 0) { \
    printf("ASSERT FAILED: %s (line %d): \"%s\" != \"%s\"\n", msg, __LINE__, s1, s2); \
    return 1; \
  } \
} while (0)

/**
* @brief Test error code to string conversion
*/
static int test_error_strings() {
  printf("  Testing error strings...\n");
  
  // Test valid error codes
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_GOOD), "No error", "COIL_ERR_GOOD should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_NOMEM), "Memory allocation failure", "COIL_ERR_NOMEM should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_INVAL), "Invalid argument", "COIL_ERR_INVAL should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_IO), "I/O error", "COIL_ERR_IO should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_FORMAT), "Format error", "COIL_ERR_FORMAT should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_NOTFOUND), "Not found", "COIL_ERR_NOTFOUND should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_NOTSUP), "Not supported", "COIL_ERR_NOTSUP should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_BADSTATE), "Bad state", "COIL_ERR_BADSTATE should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_EXISTS), "Already exists", "COIL_ERR_EXISTS should match");
  TEST_ASSERT_STR_EQ(coil_strerr(COIL_ERR_UNKNOWN), "Unknown error", "COIL_ERR_UNKNOWN should match");
  
  // Test invalid error code
  TEST_ASSERT_STR_EQ(coil_strerr(100), "Invalid error code", "Invalid error code should return message");
  
  return 0;
}

/**
* @brief Test error setting and retrieving
*/
static int test_error_set_get() {
  printf("  Testing error set/get...\n");
  
  // Test setting and retrieving each error code
  for (coil_err_t i = COIL_ERR_GOOD; i <= COIL_ERR_UNKNOWN; i++) {
    coil_error_set(i);
    TEST_ASSERT(coil_error_get_last() == i, "Error code should match what was set");
  }
  
  return 0;
}

/**
* @brief Run all error handling tests
*/
int test_err() {
  printf("\nRunning error handling tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_error_strings();
  result |= test_error_set_get();
  
  if (result == 0) {
    printf("All error handling tests passed!\n");
  }
  
  return result;
}