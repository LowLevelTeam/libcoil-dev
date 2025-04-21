/**
 * @file test_framework.c
 * @brief Implementation of testing framework for COIL library
 */

#include "test_framework.h"

// Global test result
test_result_t g_test_result = {0, 0, 0};

// Run all tests in a test array
void run_tests(const test_t* tests, int count) {
  printf("Running %d tests...\n", count);
  
  // Reset test counters for this test group
  int group_passed = 0;
  int group_failed = 0;
  
  // Run each test
  for (int i = 0; i < count; i++) {
    printf("- Test: %s\n", tests[i].name);
    
    // Update total count
    g_test_result.total++;
    
    // Store current pass/fail counts to detect changes
    int before_passed = g_test_result.passed;
    int before_failed = g_test_result.failed;
    
    // Run the test
    tests[i].func();
    
    // Check if test passed or failed
    if (g_test_result.failed > before_failed) {
      group_failed++;
      printf("  FAILED\n");
    } else if (g_test_result.passed > before_passed) {
      group_passed++;
      printf("  PASSED\n");
    } else {
      // No assertions were made
      printf("  WARNING: No assertions made\n");
    }
  }
  
  printf("Group results: %d passed, %d failed\n", group_passed, group_failed);
}

// Print test results
void print_test_results(void) {
  printf("\n===== Test Results =====\n");
  printf("Passed: %d\n", g_test_result.passed);
  printf("Failed: %d\n", g_test_result.failed);
  printf("Total:  %d\n", g_test_result.total);
  
  if (g_test_result.failed == 0) {
    printf("\nALL TESTS PASSED!\n");
  } else {
    printf("\nSOME TESTS FAILED!\n");
  }
}