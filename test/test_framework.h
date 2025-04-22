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

// -------------------------------- Original (Backward-Compatible) Assertion Macros -------------------------------- //

// Original assertion macro
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

// Original type-specific equality assertions
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

// -------------------------------- Enhanced Assertion Macros (C99/C++) -------------------------------- //
// These macros require C99 or later due to variadic macro use

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(__cplusplus) && __cplusplus >= 201103L)

// Enhanced base assertion with format
#define TEST_ASSERT_FMT(condition, format, msg_arg) do { \
  if (!(condition)) { \
    printf("FAIL: " format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
    printf("  Expression: %s\n", #condition); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: " format "\n", msg_arg); \
  } \
  g_test_result.passed++; \
} while (0)

// Generic equality check with format
#define TEST_ASSERT_EQ_FMT(expected, actual, format, type_name, msg_format, msg_arg) do { \
  if ((expected) != (actual)) { \
    printf("FAIL: " msg_format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
    printf("  Expected " type_name ": " format "\n  Actual " type_name ": " format "\n", \
           (expected), (actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: " msg_format " (value: " format ")\n", msg_arg, (actual)); \
  } \
  g_test_result.passed++; \
} while (0)

// Enhanced type-specific assertions with format
#define TEST_ASSERT_INT_FMT(expected, actual, format, msg_arg) \
  TEST_ASSERT_EQ_FMT(expected, actual, "%d", "int", format, msg_arg)

#define TEST_ASSERT_SIZE_FMT(expected, actual, format, msg_arg) \
  TEST_ASSERT_EQ_FMT(expected, actual, "%zu", "size_t", format, msg_arg)

#define TEST_ASSERT_PTR_FMT(expected, actual, format, msg_arg) \
  TEST_ASSERT_EQ_FMT(expected, actual, "%p", "pointer", format, msg_arg)

#define TEST_ASSERT_UINT64_FMT(expected, actual, format, msg_arg) \
  TEST_ASSERT_EQ_FMT(expected, actual, "%llu", "uint64", format, msg_arg)

// String comparison with format
#define TEST_ASSERT_STR_FMT(expected, actual, format, msg_arg) do { \
  if (strcmp((expected), (actual)) != 0) { \
    printf("FAIL: " format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
    printf("  Expected string: \"%s\"\n  Actual string: \"%s\"\n", (expected), (actual)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: " format " (value: \"%s\")\n", msg_arg, (actual)); \
  } \
  g_test_result.passed++; \
} while (0)

// NULL checks with format
#define TEST_ASSERT_NULL_FMT(ptr, format, msg_arg) do { \
  if ((ptr) != NULL) { \
    printf("FAIL: " format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
    printf("  Expected: NULL\n  Actual: %p\n", (void*)(ptr)); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: " format " (NULL as expected)\n", msg_arg); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_NOT_NULL_FMT(ptr, format, msg_arg) do { \
  if ((ptr) == NULL) { \
    printf("FAIL: " format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
    printf("  Got NULL, expected non-NULL\n"); \
    g_test_result.failed++; \
    return; \
  } \
  if (g_test_verbose) { \
    printf("PASS: " format " (non-NULL address: %p)\n", msg_arg, (void*)(ptr)); \
  } \
  g_test_result.passed++; \
} while (0)

#define TEST_ASSERT_MEMORY_EQUAL_FMT(expected, actual, size, format, msg_arg) do { \
  if (!compare_memory_with_diff(expected, actual, size, #actual)) { \
    printf("FAIL: " format " (%s:%d)\n", msg_arg, __FILE__, __LINE__); \
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

#endif // C99 or C++11 check

#endif // __COIL_TEST_FRAMEWORK_H