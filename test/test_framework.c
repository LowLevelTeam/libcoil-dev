/**
 * @file test_framework.c
 * @brief Implementation of testing framework for COIL library
 */

#include "test_framework.h"

// Global test result
test_result_t g_test_result = {0, 0, 0, 0.0};

// Global verbose flag
int g_test_verbose = 0;

// Run all tests in a test array
void run_tests(const test_t* tests, int count) {
  printf("Running %d tests...\n", count);
  
  // Reset test counters for this test group
  int group_passed = 0;
  int group_failed = 0;
  double group_time = 0.0;
  
  // Run each test
  for (int i = 0; i < count; i++) {
    printf("- Test: %s\n", tests[i].name);
    
    // Update total count
    g_test_result.total++;
    
    // Store current pass/fail counts to detect changes
    int before_passed = g_test_result.passed;
    int before_failed = g_test_result.failed;
    
    // Start timing
    clock_t start = clock();
    
    // Run the test
    tests[i].func();
    
    // End timing
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    g_test_result.total_time += elapsed;
    group_time += elapsed;
    
    // Check if test passed or failed
    if (g_test_result.failed > before_failed) {
      group_failed++;
      printf("  FAILED (%.3f sec)\n", elapsed);
    } else if (g_test_result.passed > before_passed) {
      group_passed++;
      printf("  PASSED (%.3f sec)\n", elapsed);
    } else {
      // No assertions were made
      printf("  WARNING: No assertions made (%.3f sec)\n", elapsed);
    }
  }
  
  printf("Group results: %d passed, %d failed (%.3f sec total)\n", 
         group_passed, group_failed, group_time);
}

// Print test results
void print_test_results(void) {
  printf("\n===== Test Results =====\n");
  printf("Passed: %d\n", g_test_result.passed);
  printf("Failed: %d\n", g_test_result.failed);
  printf("Total:  %d\n", g_test_result.total);
  printf("Time:   %.3f seconds\n", g_test_result.total_time);
  
  if (g_test_result.failed == 0) {
    printf("\nALL TESTS PASSED!\n");
  } else {
    printf("\nSOME TESTS FAILED!\n");
  }
}

// Utility to dump memory in hexadecimal format
void hexdump(const void* data, size_t size, const char* label) {
  const unsigned char* bytes = (const unsigned char*)data;
  printf("Hexdump of %s (%zu bytes):\n", label, size);
  
  for (size_t i = 0; i < size; i += 16) {
    // Print address
    printf("%04zx: ", i);
    
    // Print hex bytes
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        printf("%02x ", bytes[i + j]);
      } else {
        printf("   ");
      }
      if (j == 7) printf(" ");  // Extra space in the middle
    }
    
    // Print ASCII representation
    printf(" |");
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        unsigned char c = bytes[i + j];
        printf("%c", (c >= 32 && c <= 126) ? c : '.');
      } else {
        printf(" ");
      }
    }
    printf("|\n");
  }
  printf("\n");
}

// Utility to compare memory with detailed diff output
int compare_memory_with_diff(const void* expected, const void* actual, size_t size, const char* label) {
  const unsigned char* exp_bytes = (const unsigned char*)expected;
  const unsigned char* act_bytes = (const unsigned char*)actual;
  int differences = 0;
  
  for (size_t i = 0; i < size; i++) {
    if (exp_bytes[i] != act_bytes[i]) {
      if (differences == 0) {
        printf("Memory differences in %s:\n", label);
        printf("  OFFSET  EXPECTED  ACTUAL\n");
      }
      differences++;
      
      printf("  0x%04zx:    0x%02x    0x%02x\n", i, exp_bytes[i], act_bytes[i]);
      
      // Limit number of differences shown
      if (differences >= 10) {
        printf("  ... and more differences\n");
        break;
      }
    }
  }
  
  return (differences == 0);
}