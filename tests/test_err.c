/**
 * @file test_err.c
 * @brief Tests for the error handling system
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include <coil/err.h>

/* Globals to track callback invocation */
static int callback_called = 0;
static coil_error_level_t last_level = 0;
static char last_message[256] = {0};
static coil_error_position_t last_position = {0};

/* Test error callback */
static void test_error_callback(
  coil_error_level_t level,
  const char* message,
  const coil_error_position_t* position,
  void* user_data
) {
  callback_called = 1;
  last_level = level;
  
  if (message) {
    strncpy(last_message, message, sizeof(last_message) - 1);
    last_message[sizeof(last_message) - 1] = '\0';
  } else {
    last_message[0] = '\0';
  }
  
  if (position) {
    memcpy(&last_position, position, sizeof(coil_error_position_t));
  } else {
    memset(&last_position, 0, sizeof(coil_error_position_t));
  }
  
  /* Check that user_data is correctly passed */
  assert_int_equal(*((int*)user_data), 42);
}

/* Setup function called before each test */
static int setup(void **state) {
  (void)state; /* unused */
  
  /* Initialize error system */
  coil_err_t err = coil_error_init();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Reset callback tracking */
  callback_called = 0;
  last_level = 0;
  memset(last_message, 0, sizeof(last_message));
  memset(&last_position, 0, sizeof(last_position));
  
  return 0;
}

/* Teardown function called after each test */
static int teardown(void **state) {
  (void)state; /* unused */
  
  /* Shutdown error system */
  coil_error_shutdown();
  
  return 0;
}

/* Test error system initialization and shutdown */
static void test_error_init_shutdown(void **state) {
  (void)state; /* unused */
  
  /* Already initialized in setup */
  
  /* Test getting the last error when there is none */
  const coil_error_context_t* ctx = coil_error_get_last();
  assert_non_null(ctx);
  assert_int_equal(ctx->code, COIL_ERR_GOOD);
  
  /* Test error string functionality */
  assert_string_equal(coil_error_string(COIL_ERR_GOOD), "No error");
  assert_string_equal(coil_error_string(COIL_ERR_NOMEM), "Memory allocation failure");
  assert_string_equal(coil_error_string(COIL_ERR_INVAL), "Invalid argument");
  assert_string_equal(coil_error_string(COIL_ERR_IO), "I/O error");
  assert_string_equal(coil_error_string(COIL_ERR_FORMAT), "Format error");
  assert_string_equal(coil_error_string(COIL_ERR_NOTFOUND), "Not found");
  assert_string_equal(coil_error_string(COIL_ERR_NOTSUP), "Not supported");
  assert_string_equal(coil_error_string(COIL_ERR_BADSTATE), "Bad state");
  assert_string_equal(coil_error_string(COIL_ERR_EXISTS), "Already exists");
  assert_string_equal(coil_error_string(COIL_ERR_UNKNOWN), "Unknown error");
  
  /* Test invalid error code */
  assert_string_equal(coil_error_string(999), "Unknown error");
  
  /* Shutdown in teardown */
}

/* Test error reporting functionality */
static void test_error_report(void **state) {
  (void)state; /* unused */
  
  /* Report an error */
  coil_err_t err = coil_error_report(
    COIL_LEVEL_ERROR,
    COIL_ERR_NOMEM,
    "Test error message",
    "test_file.c",
    123,
    456
  );
  
  /* Check return value */
  assert_int_equal(err, COIL_ERR_NOMEM);
  
  /* Check that error is stored */
  const coil_error_context_t* ctx = coil_error_get_last();
  assert_non_null(ctx);
  assert_int_equal(ctx->code, COIL_ERR_NOMEM);
  assert_int_equal(ctx->level, COIL_LEVEL_ERROR);
  assert_string_equal(ctx->message, "Test error message");
  assert_string_equal(ctx->position.file, "test_file.c");
  assert_int_equal(ctx->position.line, 123);
  assert_int_equal(ctx->position.index, 456);
  
  /* Test clearing the error */
  coil_error_clear();
  ctx = coil_error_get_last();
  assert_non_null(ctx);
  assert_int_equal(ctx->code, COIL_ERR_GOOD);
  assert_int_equal(ctx->message[0], '\0');
}

/* Test custom error callback */
static void test_error_callback_func(void **state) {
  (void)state; /* unused */
  
  /* Set up user data for callback */
  int user_value = 42;
  
  /* Set custom callback */
  coil_error_set_callback(test_error_callback, &user_value);
  
  /* Report an error to trigger callback */
  coil_error_report(
    COIL_LEVEL_WARNING,
    COIL_ERR_IO,
    "Callback test message",
    "callback_test.c",
    789,
    101112
  );
  
  /* Verify callback was called with correct parameters */
  assert_true(callback_called);
  assert_int_equal(last_level, COIL_LEVEL_WARNING);
  assert_string_equal(last_message, "Callback test message");
  assert_string_equal(last_position.file, "callback_test.c");
  assert_int_equal(last_position.line, 789);
  assert_int_equal(last_position.index, 101112);
  
  /* Test setting NULL callback (should revert to default) */
  coil_error_set_callback(NULL, NULL);
  
  /* Report again (shouldn't update our tracking variables) */
  callback_called = 0;
  coil_error_report(
    COIL_LEVEL_INFO,
    COIL_ERR_GOOD,
    "This shouldn't trigger our callback",
    NULL,
    0,
    0
  );
  
  /* Verify callback was not called */
  assert_false(callback_called);
}

/* Test error macros */
static void test_error_macros(void **state) {
  (void)state; /* unused */
  
  /* Set custom callback to track calls */
  int user_value = 42;
  coil_error_set_callback(test_error_callback, &user_value);
  
  /* Test info macro */
  callback_called = 0;
  COIL_INFO(COIL_ERR_GOOD, "Info message");
  assert_true(callback_called);
  assert_int_equal(last_level, COIL_LEVEL_INFO);
  
  /* Test warning macro */
  callback_called = 0;
  COIL_WARNING(COIL_ERR_IO, "Warning message");
  assert_true(callback_called);
  assert_int_equal(last_level, COIL_LEVEL_WARNING);
  
  /* Test error macro */
  callback_called = 0;
  COIL_ERROR(COIL_ERR_INVAL, "Error message");
  assert_true(callback_called);
  assert_int_equal(last_level, COIL_LEVEL_ERROR);
  
  /* Test fatal macro */
  /* Note: In a real scenario, this would abort the program */
  /* For testing, we're relying on the fact that our test callback doesn't abort */
  callback_called = 0;
  COIL_FATAL(COIL_ERR_NOMEM, "Fatal message");
  assert_true(callback_called);
  assert_int_equal(last_level, COIL_LEVEL_FATAL);
}

/* Test error message truncation */
static void test_error_message_truncation(void **state) {
  (void)state; /* unused */
  
  /* Create a very long message */
  char long_message[1024];
  memset(long_message, 'A', sizeof(long_message) - 1);
  long_message[sizeof(long_message) - 1] = '\0';
  
  /* Report with long message */
  coil_error_report(
    COIL_LEVEL_INFO,
    COIL_ERR_GOOD,
    long_message,
    NULL,
    0,
    0
  );
  
  /* Check that message was truncated */
  const coil_error_context_t* ctx = coil_error_get_last();
  assert_non_null(ctx);
  assert_true(strlen(ctx->message) < sizeof(long_message) - 1);
  assert_int_equal(strlen(ctx->message), 255); /* 256 - 1 for null terminator */
}

/* Main function running all tests */
int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_error_init_shutdown),
    cmocka_unit_test(test_error_report),
    cmocka_unit_test(test_error_callback_func),
    cmocka_unit_test(test_error_macros),
    cmocka_unit_test(test_error_message_truncation),
  };
  
  return cmocka_run_group_tests(tests, setup, teardown);
}