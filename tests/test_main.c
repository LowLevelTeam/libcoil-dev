/**
* @file test_main.c
* @brief Main test harness for libcoil-dev
*
* @author Low Level Team
*/

#include <coil/base.h>
#include <stdio.h>
#include <stdlib.h>

extern int test_err();
extern int test_mem();
extern int test_log();
extern int test_file();
extern int test_section();
extern int test_object();
extern int test_instr();
extern int test_mmap();

/**
* @brief Run all test suites and report results
*/
int main(int argc, char *argv[]) {
  int failed = 0;
  
  printf("COIL Development Library Test Suite\n");
  printf("==================================\n\n");
  
  // Run test suites
  if (test_err() != 0) {
    printf("Error module tests FAILED\n");
    failed++;
  } else {
    printf("Error module tests PASSED\n");
  }
  
  if (test_mem() != 0) {
    printf("Memory module tests FAILED\n");
    failed++;
  } else {
    printf("Memory module tests PASSED\n");
  }
  
  if (test_log() != 0) {
    printf("Logging module tests FAILED\n");
    failed++;
  } else {
    printf("Logging module tests PASSED\n");
  }
  
  if (test_file() != 0) {
    printf("File module tests FAILED\n");
    failed++;
  } else {
    printf("File module tests PASSED\n");
  }
  
  if (test_section() != 0) {
    printf("Section module tests FAILED\n");
    failed++;
  } else {
    printf("Section module tests PASSED\n");
  }
  
  if (test_object() != 0) {
    printf("Object module tests FAILED\n");
    failed++;
  } else {
    printf("Object module tests PASSED\n");
  }
  
  if (test_instr() != 0) {
    printf("Instruction module tests FAILED\n");
    failed++;
  } else {
    printf("Instruction module tests PASSED\n");
  }
  
  if (test_mmap() != 0) {
    printf("Memory mapping tests FAILED\n");
    failed++;
  } else {
    printf("Memory mapping tests PASSED\n");
  }
  
  // Print summary
  printf("\nTest Summary: ");
  if (failed == 0) {
    printf("All tests PASSED\n");
    return 0;
  } else {
    printf("%d test suite(s) FAILED\n", failed);
    return 1;
  }
}