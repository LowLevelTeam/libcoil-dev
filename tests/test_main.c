/**
 * @file test_main.c
 * @brief Combined test runner for all COIL tests
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmocka.h>

// Forward declarations for test registration functions
extern struct CMUnitTest *get_arena_tests(int *count);
extern struct CMUnitTest *get_err_tests(int *count);  
extern struct CMUnitTest *get_instr_tests(int *count);
extern struct CMUnitTest *get_obj_tests(int *count);
extern struct CMUnitTest *get_coil_tests(int *count);
extern struct CMUnitTest *get_integration_tests(int *count);

/**
 * Structure to hold test group information
 */
typedef struct {
  const char *name;
  struct CMUnitTest *(*get_tests)(int *count);
} test_group_t;

// Test groups array
static const test_group_t test_groups[] = {
  { "Arena Allocator Tests", get_arena_tests },
  { "Error Handling Tests", get_err_tests },
  { "Instruction Serialization Tests", get_instr_tests },
  { "Object Format Tests", get_obj_tests },
  { "COIL Library Tests", get_coil_tests },
  { "Integration Tests", get_integration_tests }
};

static const int num_groups = sizeof(test_groups) / sizeof(test_groups[0]);

// Global test verbosity flag (can be set with environment variable)
int test_verbosity = 0;

int main(int argc, char *argv[]) {
  int run_subset = 0;
  char *subset_name = NULL;
  
  // Check for verbosity environment variable
  const char *verbosity_env = getenv("COIL_TEST_VERBOSE");
  if (verbosity_env && (strcmp(verbosity_env, "1") == 0 || 
                       strcmp(verbosity_env, "true") == 0 ||
                       strcmp(verbosity_env, "yes") == 0)) {
    test_verbosity = 1;
  }
  
  // Check command line arguments for specific test groups
  if (argc > 1) {
    run_subset = 1;
    subset_name = argv[1];
  }
  
  clock_t start_time, end_time;
  start_time = clock();
  
  printf("=== COIL Test Suite ===\n");
  printf("Verbosity: %s\n\n", test_verbosity ? "enabled" : "disabled");
  
  int total_tests = 0;
  int total_failed = 0;
  
  // Run all test groups or just the specified one
  for (int i = 0; i < num_groups; i++) {
    // If we're running a subset, check if this is the right group
    if (run_subset && strcmp(subset_name, test_groups[i].name) != 0) {
      continue;
    }
    
    printf("Running %s:\n", test_groups[i].name);
    
    // Get tests for this group
    int count = 0;
    struct CMUnitTest *tests = test_groups[i].get_tests(&count);
    printf("  Found %d tests in group\n", count);
    if (!tests || count == 0) {
      printf("  No tests found in group\n");
      continue;
    }
    
    // Run the tests in this group
    int result = _cmocka_run_group_tests(test_groups[i].name, tests, count, NULL, NULL);
    if (result != 0) {
      printf("  %d tests FAILED in %s\n", result, test_groups[i].name);
    } else {
      printf("  All %d tests PASSED in %s\n", count, test_groups[i].name);
    }
    
    // Do not free the static array - this was causing the segmentation fault
    // free(tests);
    
    total_tests += count;
    total_failed += (result < 0 ? 0 : result);
  }
  
  end_time = clock();
  double duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
  printf("\n=== Test Summary ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Tests passed: %d\n", total_tests - total_failed);
  printf("Tests failed: %d\n", total_failed);
  printf("Time elapsed: %.2f seconds\n", duration);
  
  return total_failed;
}