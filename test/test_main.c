/**
 * @file test_main.c
 * @brief Main entry point for COIL library tests
 */

#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Declare test runner functions
void run_arena_tests(void);
void run_error_tests(void);
void run_instr_tests(void);
void run_obj_tests(void);
void run_coil_tests(void);

// Main entry point
int main(int argc, char* argv[]) {
  printf("COIL Library Test Suite\n");
  printf("======================\n\n");
  
  // Check for specific test filter and verbose flag
  const char* filter = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      g_test_verbose = 1;
      printf("Verbose mode enabled\n");
    } else if (filter == NULL) {
      filter = argv[i];
      printf("Running tests matching: %s\n\n", filter);
    }
  }
  
  // Run tests
  if (!filter || strstr("arena", filter)) {
    printf("\n=== Arena Tests ===\n");
    run_arena_tests();
  }
  
  if (!filter || strstr("error", filter)) {
    printf("\n=== Error Handling Tests ===\n");
    run_error_tests();
  }
  
  if (!filter || strstr("instr", filter)) {
    printf("\n=== Instruction Tests ===\n");
    run_instr_tests();
  }
  
  if (!filter || strstr("obj", filter)) {
    printf("\n=== Object Format Tests ===\n");
    run_obj_tests();
  }
  
  if (!filter || strstr("coil", filter)) {
    printf("\n=== COIL Library Tests ===\n");
    run_coil_tests();
  }
  
  // Print overall results
  print_test_results();
  
  // Return success if all tests passed
  return (g_test_result.failed == 0) ? 0 : 1;
}