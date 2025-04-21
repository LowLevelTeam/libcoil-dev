/**
 * @file test_framework.h
 * @brief Simple testing framework for COIL library
 */

#ifndef __COIL_TEST_FRAMEWORK_H
#define __COIL_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global verbose flag
extern int g_test_verbose;

// Test result structure
typedef struct {
  int passed;
  int failed;
  int total;
  double total_time;    // Track total execution time
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

// Utility to dump memory in hexadecimal format
void hexdump(const void* data, size_t size, const char* label);

// Utility to compare memory with detailed diff output
int compare_memory_with_diff(const void* expected, const void* actual, size_t size, const char* label);

// Assertion macros
#define TEST_ASSERT(condition, message) do { \
  if (!(condition)) { \
    printf("FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    printf("  Expression: %s\n", #condition); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s\n", message); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: %d (0x%x)\n  Actual: %d (0x%x)\n", \
           (int)(expected), (int)(expected), (int)(actual), (int)(actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (value: %d)\n", message, (int)(actual)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_SIZE(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: %zu (0x%zx)\n  Actual: %zu (0x%zx)\n", \
           (size_t)(expected), (size_t)(expected), (size_t)(actual), (size_t)(actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (value: %zu)\n", message, (size_t)(actual)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: %p == Actual: %p\n", \
           (void*)(expected), (void*)(actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (value: %p)\n", message, (void*)(actual)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_UINT64(expected, actual, message) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: %llu (0x%llx)\n  Actual: %llu (0x%llx)\n", \
           (unsigned long long)(expected), (unsigned long long)(expected), \
           (unsigned long long)(actual), (unsigned long long)(actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (value: %llu)\n", message, (unsigned long long)(actual)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_EQUAL_STR(expected, actual, message) do { \
  if (strcmp((expected), (actual)) != 0) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: \"%s\"\n  Actual: \"%s\"\n", (expected), (actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (value: \"%s\")\n", message, (actual)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_NULL(ptr, message) do { \
  if ((ptr) != NULL) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Expected: NULL\n  Actual: %p\n", (void*)(ptr)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (NULL as expected)\n", message); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message) do { \
  if ((ptr) == NULL) { \
    printf("FAIL: %s (%s:%d)\n", \
           message, __FILE__, __LINE__); \
    printf("  Got NULL, expected non-NULL\n"); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: %s (non-NULL address: %p)\n", message, (void*)(ptr)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_MEMORY_EQUAL(expected, actual, size, message) do { \
  if (!compare_memory_with_diff(expected, actual, size, #actual)) { \
    printf("FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    if (g_test_verbose) { \
      printf("Expected:\n"); \
      hexdump(expected, size, "expected"); \
      printf("Actual:\n"); \
      hexdump(actual, size, "actual"); \
    } \
    g_test_result.failed++; \
    return; \
  } \
  g_test_result.passed++; \
} while (0)

#endif // __COIL_TEST_FRAMEWORK_H