/**
* @file test_log.c
* @brief Test suite for logging functionality
*
* @author Low Level Team
*/

#include <coil/log.h>
#include <stdio.h>
#include <stdlib.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

/**
* @brief Test basic logging functionality
*/
static int test_basic_logging() {
  printf("  Testing basic logging functionality...\n");
  
  // We can't easily test log output, so just ensure these don't crash
  
  // Test setting log level
  coil_log_set_level(COIL_LEVEL_INFO);
  
  // Test all log levels
  coil_log(COIL_LEVEL_INFO, "This is an INFO test message");
  coil_log(COIL_LEVEL_WARNING, "This is a WARNING test message");
  coil_log(COIL_LEVEL_ERROR, "This is an ERROR test message");
  
  // Test with formatting
  coil_log(COIL_LEVEL_INFO, "This is a formatted message: %d, %s", 42, "test");
  
  // Set higher log level and verify lower messages don't display
  // We can't verify output, but at least ensure it doesn't crash
  coil_log_set_level(COIL_LEVEL_ERROR);
  coil_log(COIL_LEVEL_INFO, "This should not be displayed");
  coil_log(COIL_LEVEL_WARNING, "This should not be displayed");
  coil_log(COIL_LEVEL_ERROR, "This should be displayed");
  
  return 0;
}

/**
* @brief Run all logging tests
*
* Note: Logging tests are not exhaustive since we can't easily
* capture and validate the output.
*/
int test_log() {
  printf("\nRunning logging tests...\n");
  
  int result = 0;
  
  // Reset log level to default for other tests
  coil_log_set_level(COIL_LEVEL_INFO);
  
  // Run individual test functions
  result |= test_basic_logging();
  
  if (result == 0) {
    printf("All logging tests passed!\n");
  }
  
  return result;
}