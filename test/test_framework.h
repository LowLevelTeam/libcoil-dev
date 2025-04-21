/**
 * @file test_framework.h
 * @brief Simple testing framework for COIL library
 */

#ifndef __COIL_TEST_FRAMEWORK_H
#define __COIL_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test result structure
typedef struct {
  int passed;
  int failed;
  int total;
} test_result_t;

// Test function type
typedef void (*test_func_t)(void);

// Test structure
typedef struct {
  const char* name;
  test_func_t func;
} test_t;

// Global test result
extern test_result_t g_test_result;

// Run all tests in a test array
void run_tests(const test_t* tests, int count);

// Print test results
void print_test_results(void);

// Assertion macros
#define TEST_ASSERT(condition, message) do { \
  if (!(condition)) { \
    printf("FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (expected %d, got %d) (%s:%d)\n", \
           message, (int)(expected), (int)(actual), __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_SIZE(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (expected %zu, got %zu) (%s:%d)\n", \
           message, (size_t)(expected), (size_t)(actual), __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_UINT64(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (expected %llu, got %llu) (%s:%d)\n", \
           message, (unsigned long long)(expected), (unsigned long long)(actual), __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_STR(expected, actual, message) do { \
  if (strcmp((expected), (actual)) != 0) { \
    printf("FAIL: %s (expected \"%s\", got \"%s\") (%s:%d)\n", \
           message, (expected), (actual), __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_NULL(ptr, message) do { \
  if ((ptr) != NULL) { \
    printf("FAIL: %s (expected NULL) (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message) do { \
  if ((ptr) == NULL) { \
    printf("FAIL: %s (got NULL) (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#endif // __COIL_TEST_FRAMEWORK_H