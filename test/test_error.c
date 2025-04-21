/**
 * @file test_error.c
 * @brief Tests for the error handling system
 */

#include "test_framework.h"
#include <coil/err.h>
#include <string.h>

// Global variable to track callback calls
static int g_callback_called = 0;
static coil_error_level_t g_last_error_level = 0;
static char g_last_error_message[256] = {0};
static coil_error_position_t g_last_error_position = {0};

// Custom error callback function for testing
static void test_error_handler(
  coil_error_level_t level,
  const char* message,
  const coil_error_position_t* position,
  void* user_data
) {
  int* callback_counter = (int*)user_data;
  if (callback_counter) {
    (*callback_counter)++;
  }
  
  g_callback_called++;
  g_last_error_level = level;
  
  if (message) {
    strncpy(g_last_error_message, message, sizeof(g_last_error_message) - 1);
    g_last_error_message[sizeof(g_last_error_message) - 1] = '\0';
  } else {
    g_last_error_message[0] = '\0';
  }
  
  if (position) {
    g_last_error_position = *position;
  } else {
    memset(&g_last_error_position, 0, sizeof(g_last_error_position));
  }
}

// Test error initialization and shutdown
void test_error_init_shutdown(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // Verify last error is cleared
  const coil_error_context_t* ctx = coil_error_get_last();
  TEST_ASSERT_NOT_NULL(ctx, "Error context should be available");
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, ctx->code, "Initial error code should be GOOD");
  
  // Shutdown and reinitialize
  coil_error_shutdown();
  err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system re-initialization should succeed");
  
  coil_error_shutdown();
}

// Test error callback functionality
void test_error_callback_func(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // Reset global tracking variables
  g_callback_called = 0;
  g_last_error_message[0] = '\0';
  
  // Set custom callback
  int callback_counter = 0;
  coil_error_set_callback(test_error_handler, &callback_counter);
  
  // Report an error
  const char* test_message = "Test error message";
  err = coil_error_report(COIL_LEVEL_ERROR, COIL_ERR_INVAL, test_message, "test.c", 42, 0);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "Error report should return the error code");
  
  // Check callback was called
  TEST_ASSERT_EQUAL_INT(1, g_callback_called, "Error callback should be called");
  TEST_ASSERT_EQUAL_INT(1, callback_counter, "User data counter should be incremented");
  TEST_ASSERT_EQUAL_INT(COIL_LEVEL_ERROR, g_last_error_level, "Error level should match");
  TEST_ASSERT_EQUAL_STR(test_message, g_last_error_message, "Error message should match");
  TEST_ASSERT_EQUAL_STR("test.c", g_last_error_position.file, "Error file should match");
  TEST_ASSERT_EQUAL_INT(42, g_last_error_position.line, "Error line should match");
  
  // Reset callback
  coil_error_set_callback(NULL, NULL);
  
  // Report another error - our callback shouldn't be called
  g_callback_called = 0;
  callback_counter = 0;
  coil_error_report(COIL_LEVEL_WARNING, COIL_ERR_NOMEM, "Another message", "file.c", 100, 0);
  TEST_ASSERT_EQUAL_INT(0, g_callback_called, "Our callback should not be called after reset");
  TEST_ASSERT_EQUAL_INT(0, callback_counter, "User data counter should not be incremented");
  
  coil_error_shutdown();
}

// Test get_last and clear functions
void test_error_get_clear(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // Report an error
  const char* test_message = "Test error for get_last";
  coil_error_report(COIL_LEVEL_ERROR, COIL_ERR_FORMAT, test_message, "file.c", 123, 456);
  
  // Get the last error
  const coil_error_context_t* ctx = coil_error_get_last();
  TEST_ASSERT_NOT_NULL(ctx, "Error context should be available");
  TEST_ASSERT_EQUAL_INT(COIL_ERR_FORMAT, ctx->code, "Error code should match");
  TEST_ASSERT_EQUAL_INT(COIL_LEVEL_ERROR, ctx->level, "Error level should match");
  TEST_ASSERT(strncmp(test_message, ctx->message, strlen(test_message)) == 0, 
              "Error message should match");
  TEST_ASSERT_EQUAL_STR("file.c", ctx->position.file, "Error file should match");
  TEST_ASSERT_EQUAL_INT(123, ctx->position.line, "Error line should match");
  TEST_ASSERT_EQUAL_SIZE(456, ctx->position.index, "Error index should match");
  
  // Clear the error
  coil_error_clear();
  
  // Get the error again
  ctx = coil_error_get_last();
  TEST_ASSERT_NOT_NULL(ctx, "Error context should still be available");
  TEST_ASSERT_EQUAL_INT(0, ctx->code, "Error code should be reset");
  TEST_ASSERT_EQUAL_INT(0, ctx->level, "Error level should be reset");
  TEST_ASSERT_EQUAL_INT(0, strlen(ctx->message), "Error message should be empty");
  
  coil_error_shutdown();
}

// Test error string function
void test_error_string(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // Test all error codes
  const char* str = coil_error_string(COIL_ERR_GOOD);
  TEST_ASSERT(strcmp("No error", str) == 0, "GOOD error string should match");
  
  str = coil_error_string(COIL_ERR_NOMEM);
  TEST_ASSERT(strcmp("Memory allocation failure", str) == 0, "NOMEM error string should match");
  
  str = coil_error_string(COIL_ERR_INVAL);
  TEST_ASSERT(strcmp("Invalid argument", str) == 0, "INVAL error string should match");
  
  str = coil_error_string(COIL_ERR_IO);
  TEST_ASSERT(strcmp("I/O error", str) == 0, "IO error string should match");
  
  str = coil_error_string(COIL_ERR_FORMAT);
  TEST_ASSERT(strcmp("Format error", str) == 0, "FORMAT error string should match");
  
  str = coil_error_string(COIL_ERR_NOTFOUND);
  TEST_ASSERT(strcmp("Not found", str) == 0, "NOTFOUND error string should match");
  
  str = coil_error_string(COIL_ERR_NOTSUP);
  TEST_ASSERT(strcmp("Not supported", str) == 0, "NOTSUP error string should match");
  
  str = coil_error_string(COIL_ERR_BADSTATE);
  TEST_ASSERT(strcmp("Bad state", str) == 0, "BADSTATE error string should match");
  
  str = coil_error_string(COIL_ERR_EXISTS);
  TEST_ASSERT(strcmp("Already exists", str) == 0, "EXISTS error string should match");
  
  str = coil_error_string(COIL_ERR_UNKNOWN);
  TEST_ASSERT(strcmp("Unknown error", str) == 0, "UNKNOWN error string should match");
  
  // Test invalid error code
  str = coil_error_string(999);
  TEST_ASSERT(strcmp("Unknown error", str) == 0, "Invalid error code should return unknown");
  
  coil_error_shutdown();
}

// Test error macros
void test_error_macros(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // Set custom callback
  g_callback_called = 0;
  coil_error_set_callback(test_error_handler, NULL);
  
  // Test INFO macro
  COIL_INFO(COIL_ERR_GOOD, "Info message");
  TEST_ASSERT_EQUAL_INT(1, g_callback_called, "INFO macro should call callback");
  TEST_ASSERT_EQUAL_INT(COIL_LEVEL_INFO, g_last_error_level, "Level should be INFO");
  
  // Test WARNING macro
  g_callback_called = 0;
  COIL_WARNING(COIL_ERR_IO, "Warning message");
  TEST_ASSERT_EQUAL_INT(1, g_callback_called, "WARNING macro should call callback");
  TEST_ASSERT_EQUAL_INT(COIL_LEVEL_WARNING, g_last_error_level, "Level should be WARNING");
  
  // Test ERROR macro
  g_callback_called = 0;
  COIL_ERROR(COIL_ERR_NOMEM, "Error message");
  TEST_ASSERT_EQUAL_INT(1, g_callback_called, "ERROR macro should call callback");
  TEST_ASSERT_EQUAL_INT(COIL_LEVEL_ERROR, g_last_error_level, "Level should be ERROR");
  
  // Testing FATAL would cause an abort, so we'll skip it
  
  // Reset callback
  coil_error_set_callback(NULL, NULL);
  
  coil_error_shutdown();
}

// Test error system with extreme input
void test_error_extreme(void) {
  coil_err_t err = coil_error_init();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Error system initialization should succeed");
  
  // NULL message
  err = coil_error_report(COIL_LEVEL_INFO, COIL_ERR_GOOD, NULL, "file.c", 1, 0);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "NULL message should be handled");
  
  // NULL file
  err = coil_error_report(COIL_LEVEL_INFO, COIL_ERR_GOOD, "Message", NULL, 1, 0);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "NULL file should be handled");
  
  // Very long message
  char long_message[1024];
  memset(long_message, 'A', sizeof(long_message) - 1);
  long_message[sizeof(long_message) - 1] = '\0';
  
  err = coil_error_report(COIL_LEVEL_INFO, COIL_ERR_GOOD, long_message, "file.c", 1, 0);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Long message should be handled (truncated)");
  
  // Get the last error to check truncation
  const coil_error_context_t* ctx = coil_error_get_last();
  TEST_ASSERT_NOT_NULL(ctx, "Error context should be available");
  TEST_ASSERT(strlen(ctx->message) < sizeof(long_message), "Message should be truncated");
  
  coil_error_shutdown();
}

// Define test array
test_t error_tests[] = {
  {"Error Init/Shutdown", test_error_init_shutdown},
  {"Error Callback", test_error_callback_func},
  {"Error Get/Clear", test_error_get_clear},
  {"Error String", test_error_string},
  {"Error Macros", test_error_macros},
  {"Error Extreme", test_error_extreme}
};

// Run error tests
void run_error_tests(void) {
  run_tests(error_tests, sizeof(error_tests) / sizeof(error_tests[0]));
}