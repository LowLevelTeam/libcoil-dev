/**
 * @file test_arena.c
 * @brief Tests for the arena allocator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>
#include <stdio.h>

#include <coil/arena.h>

// For combined test mode
#ifndef RUN_INDIVIDUAL
extern int test_verbosity;
#endif

/**
 * @brief Print arena information for debugging
 */
static void debug_print_arena_info(const coil_arena_t *arena) {
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (!verbosity) return;
  
  printf("Arena state:\n");
  printf("  ├─ Capacity: %zu bytes\n", arena_capacity(arena));
  printf("  ├─ Used:     %zu bytes\n", arena_used(arena));
  printf("  └─ Max size: %zu bytes\n", arena_max_size(arena));
}

/* Test arena creation and destruction */
static void test_arena_init_destroy(void **state) {
  (void)state; /* unused */
  
  /* Test with valid parameters */
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  assert_int_equal(arena_capacity(arena), 4096);
  assert_int_equal(arena_used(arena), 0);
  assert_int_equal(arena_max_size(arena), 0); /* 0 means unlimited */
  
  debug_print_arena_info(arena);
  arena_destroy(arena);
  
  /* Test with minimum size */
  arena = arena_init(1, 0); /* Should be adjusted to minimum block size */
  assert_non_null(arena);
  assert_true(arena_capacity(arena) >= 4096); /* Should be at least minimum size */
  
  debug_print_arena_info(arena);
  arena_destroy(arena);
  
  /* Test with max size */
  arena = arena_init(4096, 8192);
  assert_non_null(arena);
  assert_int_equal(arena_max_size(arena), 8192);
  
  debug_print_arena_info(arena);
  arena_destroy(arena);
}

/* Test basic allocation */
static void test_arena_basic_alloc(void **state) {
  (void)state; /* unused */
  
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  
  /* Allocate memory and check tracking */
  void *ptr1 = arena_alloc_default(arena, 100);
  assert_non_null(ptr1);
  assert_int_equal(arena_used(arena), 100);
  
  void *ptr2 = arena_alloc_default(arena, 200);
  assert_non_null(ptr2);
  assert_int_equal(arena_used(arena), 300);
  
  /* Check that pointers are different */
  assert_ptr_not_equal(ptr1, ptr2);
  
  debug_print_arena_info(arena);
  
  /* Reset arena and check usage */
  arena_reset(arena);
  assert_int_equal(arena_used(arena), 0);
  
  debug_print_arena_info(arena);
  
  arena_destroy(arena);
}

/* Test allocations with different alignments */
static void test_arena_alignment(void **state) {
  (void)state; /* unused */
  
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  
  /* Test 1-byte alignment */
  void *ptr1 = arena_alloc(arena, 10, 1);
  assert_non_null(ptr1);
  
  /* Test 4-byte alignment */
  void *ptr2 = arena_alloc(arena, 10, 4);
  assert_non_null(ptr2);
  assert_true(((uintptr_t)ptr2 % 4) == 0);
  
  /* Test 8-byte alignment */
  void *ptr3 = arena_alloc(arena, 10, 8);
  assert_non_null(ptr3);
  assert_true(((uintptr_t)ptr3 % 8) == 0);
  
  /* Test 16-byte alignment */
  void *ptr4 = arena_alloc(arena, 10, 16);
  assert_non_null(ptr4);
  assert_true(((uintptr_t)ptr4 % 16) == 0);
  
  debug_print_arena_info(arena);
  arena_destroy(arena);
}

/* Test pushing data into the arena */
static void test_arena_push(void **state) {
  (void)state; /* unused */
  
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  
  /* Push data and verify it was copied */
  const char *test_str = "Testing arena push";
  char *str_copy = arena_push_default(arena, test_str, strlen(test_str) + 1);
  
  assert_non_null(str_copy);
  assert_string_equal(str_copy, test_str);
  
  /* Push with explicit alignment */
  int numbers[] = {1, 2, 3, 4, 5};
  int *num_copy = arena_push(arena, numbers, sizeof(numbers), 8);
  
  assert_non_null(num_copy);
  assert_true(((uintptr_t)num_copy % 8) == 0);
  assert_memory_equal(num_copy, numbers, sizeof(numbers));
  
  debug_print_arena_info(arena);
  arena_destroy(arena);
}

/* Test allocation beyond initial capacity */
static void test_arena_grow(void **state) {
  (void)state; /* unused */
  
  /* Create a small arena */
  coil_arena_t *arena = arena_init(128, 0);
  assert_non_null(arena);
  assert_int_equal(arena_capacity(arena), 128);
  
  /* Fill the arena */
  void *ptr1 = arena_alloc_default(arena, 64);
  assert_non_null(ptr1);
  void *ptr2 = arena_alloc_default(arena, 64);
  assert_non_null(ptr2);
  
  /* Print state before growth */
  printf("\n--- Pre-growth arena state ---\n");
  debug_print_arena_info(arena);
  
  /* This allocation should cause the arena to grow */
  void *ptr3 = arena_alloc_default(arena, 64);
  assert_non_null(ptr3);
  assert_true(arena_capacity(arena) > 128);
  
  /* Print state after growth */
  printf("--- Post-growth arena state ---\n");
  debug_print_arena_info(arena);
  
  arena_destroy(arena);
}

/* Test allocation with max size limit */
static void test_arena_max_size(void **state) {
  (void)state; /* unused */
  
  /* Create an arena with a max size */
  coil_arena_t *arena = arena_init(128, 256);
  assert_non_null(arena);
  
  /* Allocate up to the max size */
  void *ptr1 = arena_alloc_default(arena, 128);
  assert_non_null(ptr1);
  
  printf("\n--- Arena before reaching max size ---\n");
  debug_print_arena_info(arena);
  
  void *ptr2 = arena_alloc_default(arena, 100);
  assert_non_null(ptr2);
  
  printf("--- Arena at max size ---\n");
  debug_print_arena_info(arena);
  
  /* This allocation should fail because it would exceed max size */
  void *ptr3 = arena_alloc_default(arena, 100);
  assert_null(ptr3);
  
  arena_destroy(arena);
}

/* Test edge cases */
static void test_arena_edge_cases(void **state) {
  (void)state; /* unused */
  
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  
  /* Test zero-size allocation */
  void *ptr1 = arena_alloc_default(arena, 0);
  assert_null(ptr1);
  
  /* Test NULL arena */
  void *ptr2 = arena_alloc_default(NULL, 100);
  assert_null(ptr2);
  
  /* Test invalid arena operations */
  assert_int_equal(arena_capacity(NULL), 0);
  assert_int_equal(arena_used(NULL), 0);
  assert_int_equal(arena_max_size(NULL), 0);
  
  /* Valid arena */
  arena_destroy(arena);
  
  /* Test destroying NULL arena (should not crash) */
  arena_destroy(NULL);
}

/* Get arena tests for combined testing */
struct CMUnitTest *get_arena_tests(int *count) {
  static struct CMUnitTest arena_tests[] = {
    cmocka_unit_test(test_arena_init_destroy),
    cmocka_unit_test(test_arena_basic_alloc),
    cmocka_unit_test(test_arena_alignment),
    cmocka_unit_test(test_arena_push),
    cmocka_unit_test(test_arena_grow),
    cmocka_unit_test(test_arena_max_size),
    cmocka_unit_test(test_arena_edge_cases),
  };
  
  *count = sizeof(arena_tests) / sizeof(arena_tests[0]);
  printf("[get_arena_tests] Returning %d tests\n", *count);
  return arena_tests;
}

/* Individual test main function */
#ifdef RUN_INDIVIDUAL
int main(void) {
  printf("Running arena tests individually\n");
  
  int count;
  struct CMUnitTest *tests = get_arena_tests(&count);
  printf("Running %d tests\n", count);
  
  return _cmocka_run_group_tests("Arena Tests", tests, count, NULL, NULL);
}
#endif