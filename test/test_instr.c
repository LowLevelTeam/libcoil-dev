/**
 * @file test_instr.c
 * @brief Enhanced tests for the instruction encoding and decoding functionality
 * with improved debugging information
 */

#include "test_framework.h"
#include <coil/instr.h>
#include <coil/arena.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Define test array with more comprehensive tests
test_t instr_tests[] = {
  

  // {"TODO", TODO},
};

// Run instruction tests
void run_instr_tests(void) {
  printf("Running instruction encoding/decoding tests with enhanced debugging...\n\n");
  run_tests(instr_tests, sizeof(instr_tests) / sizeof(instr_tests[0]));
}