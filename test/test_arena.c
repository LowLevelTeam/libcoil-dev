/**
 * @file test_arena.c
 * @brief Tests for the arena allocator
 */

#include "test_framework.h"
#include <coil/arena.h>
#include <stdint.h>
#include <stdio.h>

// Test arena creation with valid parameters
void test_arena_init_valid(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created with valid parameters");
  
  size_t capacity = arena_capacity(arena);
  TEST_ASSERT(capacity >= 4096, "Arena capacity should be at least 4096 bytes");
  
  arena_destroy(arena);
}

// Test arena creation with invalid parameters
void test_arena_init_invalid(void) {
  // Test with too small initial size (should adjust to minimum)
  coil_arena_t* arena = arena_init(10, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should adjust small initial size");
  
  size_t capacity = arena_capacity(arena);
  TEST_ASSERT(capacity >= 4096, "Arena should adjust to minimum size");
  
  arena_destroy(arena);
  
  // Test with invalid max size
  arena = arena_init(8192, 4096);
  TEST_ASSERT_NULL(arena, "Arena should not be created with max_size < initial_size");
}

// Test arena allocation
void test_arena_alloc(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Allocate memory
  void* ptr1 = arena_alloc(arena, 128, 8);
  TEST_ASSERT_NOT_NULL(ptr1, "Allocation should succeed");
  
  // Check alignment
  TEST_ASSERT(((uintptr_t)ptr1 % 8) == 0, "Allocation should be aligned to 8 bytes");
  
  // Allocate more memory
  void* ptr2 = arena_alloc(arena, 256, 16);
  TEST_ASSERT_NOT_NULL(ptr2, "Second allocation should succeed");
  TEST_ASSERT(ptr2 != ptr1, "Allocations should be different");
  TEST_ASSERT(((uintptr_t)ptr2 % 16) == 0, "Allocation should be aligned to 16 bytes");
  
  // Check used memory
  size_t used = arena_used(arena);
  TEST_ASSERT(used >= 128 + 256, "Used memory should account for allocations");
  
  arena_destroy(arena);
}

// Test arena with default alignment
void test_arena_alloc_default(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Allocate memory with default alignment
  void* ptr = arena_alloc_default(arena, 100);
  TEST_ASSERT_NOT_NULL(ptr, "Allocation with default alignment should succeed");
  
  arena_destroy(arena);
}

// Test arena reset
void test_arena_reset(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Allocate memory
  void* ptr1 = arena_alloc_default(arena, 1000);
  void* ptr2 = arena_alloc_default(arena, 1000);
  TEST_ASSERT_NOT_NULL(ptr1, "First allocation should succeed");
  TEST_ASSERT_NOT_NULL(ptr2, "Second allocation should succeed");
  
  size_t used_before = arena_used(arena);
  TEST_ASSERT(used_before >= 2000, "Used memory should account for allocations");
  
  // Reset arena
  arena_reset(arena);
  
  size_t used_after = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(0, used_after, "Used memory should be zero after reset");
  
  // Allocate again
  void* ptr3 = arena_alloc_default(arena, 500);
  TEST_ASSERT_NOT_NULL(ptr3, "Allocation after reset should succeed");
  
  // Check if we reuse memory from the beginning
  // Note: This is implementation-dependent, but likely true
  TEST_ASSERT(ptr3 == ptr1, "Allocation after reset should reuse memory");
  
  arena_destroy(arena);
}

// Test arena maximum size enforcement
void test_arena_max_size(void) {
  const size_t initial_size = 1024;
  const size_t max_size = 4096;
  
  coil_arena_t* arena = arena_init(initial_size, max_size);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created with max size");
  
  // Allocate up to max size
  void* ptr = NULL;
  size_t allocated = 0;
  
  while (allocated < max_size) {
    size_t block_size = 512;
    ptr = arena_alloc_default(arena, block_size);
    
    if (ptr == NULL) {
      // Allocation failed, which is expected when we reach max size
      break;
    }
    
    allocated += block_size;
  }
  
  // We should have allocated close to max_size
  size_t used = arena_used(arena);
  TEST_ASSERT(used <= max_size, "Arena should not exceed max size");
  
  // This allocation should fail
  ptr = arena_alloc_default(arena, max_size);
  TEST_ASSERT_NULL(ptr, "Allocation exceeding max size should fail");
  
  arena_destroy(arena);
}

// Test arena push functions
void test_arena_push(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Push data to arena
  int data1 = 42;
  int* ptr1 = arena_push(arena, &data1, sizeof(data1), sizeof(int));
  TEST_ASSERT_NOT_NULL(ptr1, "Push should succeed");
  TEST_ASSERT_EQUAL_INT(data1, *ptr1, "Pushed data should match");
  
  // Modify original data - shouldn't affect pushed copy
  data1 = 100;
  TEST_ASSERT_EQUAL_INT(42, *ptr1, "Pushed data should be independent");
  
  // Test push_default
  char data2[] = "hello";
  char* ptr2 = arena_push_default(arena, data2, sizeof(data2));
  TEST_ASSERT_NOT_NULL(ptr2, "Push default should succeed");
  TEST_ASSERT_EQUAL_STR(data2, ptr2, "Pushed string should match");
  
  arena_destroy(arena);
}

// Test extreme conditions
void test_arena_extreme(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Test allocation of zero bytes
  void* ptr = arena_alloc(arena, 0, 8);
  TEST_ASSERT_NULL(ptr, "Allocation of zero bytes should fail");
  
  // Test allocation with 1-byte alignment
  ptr = arena_alloc(arena, 100, 1);
  TEST_ASSERT_NOT_NULL(ptr, "Allocation with 1-byte alignment should succeed");
  
  // Test large allocation forcing new block
  ptr = arena_alloc(arena, 8192, 8);
  TEST_ASSERT_NOT_NULL(ptr, "Large allocation should succeed");
  
  arena_destroy(arena);
}

// Test multiple blocks and larger-than-block allocations
void test_arena_multiple_blocks(void) {
  // Create arena with small initial size to force multiple blocks
  coil_arena_t* arena = arena_init(1024, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Allocate slightly less than initial size
  void* ptr1 = arena_alloc_default(arena, 900);
  TEST_ASSERT_NOT_NULL(ptr1, "First allocation should succeed");
  
  // Allocate again, forcing new block
  void* ptr2 = arena_alloc_default(arena, 900);
  TEST_ASSERT_NOT_NULL(ptr2, "Second allocation should succeed");
  
  // Allocate larger than initial block size
  void* ptr3 = arena_alloc_default(arena, 2000);
  TEST_ASSERT_NOT_NULL(ptr3, "Large allocation should succeed");
  
  arena_destroy(arena);
}

// Test for buffer overrun detection
void test_arena_buffer_safety(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Allocate two adjacent memory blocks
  char* ptr1 = arena_alloc(arena, 128, 8);
  char* ptr2 = arena_alloc(arena, 128, 8);
  
  TEST_ASSERT_NOT_NULL(ptr1, "First allocation should succeed");
  TEST_ASSERT_NOT_NULL(ptr2, "Second allocation should succeed");
  
  // Fill first buffer completely (but don't overflow)
  for (int i = 0; i < 128; i++) {
    ptr1[i] = (char)i;
  }
  
  // Fill second buffer with different pattern
  for (int i = 0; i < 128; i++) {
    ptr2[i] = (char)(255 - i);
  }
  
  // Verify first buffer wasn't corrupted by second
  for (int i = 0; i < 128; i++) {
    TEST_ASSERT_EQUAL_INT((char)i, ptr1[i], "First buffer data should be intact");
  }
  
  // Verify second buffer has correct pattern
  for (int i = 0; i < 128; i++) {
    TEST_ASSERT_EQUAL_INT((char)(255 - i), ptr2[i], "Second buffer data should be intact");
  }
  
  arena_destroy(arena);
}

// Define test array
test_t arena_tests[] = {
  {"Arena Init Valid", test_arena_init_valid},
  {"Arena Init Invalid", test_arena_init_invalid},
  {"Arena Allocation", test_arena_alloc},
  {"Arena Default Allocation", test_arena_alloc_default},
  {"Arena Reset", test_arena_reset},
  {"Arena Max Size", test_arena_max_size},
  {"Arena Push", test_arena_push},
  {"Arena Extreme Conditions", test_arena_extreme},
  {"Arena Multiple Blocks", test_arena_multiple_blocks},
  {"Arena Buffer Safety", test_arena_buffer_safety}
};

// Run arena tests
void run_arena_tests(void) {
  run_tests(arena_tests, sizeof(arena_tests) / sizeof(arena_tests[0]));
}