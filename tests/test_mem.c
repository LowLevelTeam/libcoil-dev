/**
* @file test_mem.c
* @brief Test suite for memory management functionality
*
* @author Low Level Team
*/

#include <coil/mem.h>
#include <stdio.h>
#include <string.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

/**
* @brief Test basic memory operations
*/
static int test_basic_memory_ops() {
  printf("  Testing basic memory operations...\n");
  
  // Test malloc and free
  void *mem = coil_malloc(128);
  TEST_ASSERT(mem != NULL, "Memory allocation should succeed");
  coil_free(mem);
  
  // Test calloc
  void *cmem = coil_calloc(10, 10);
  TEST_ASSERT(cmem != NULL, "Calloc should succeed");
  
  // Check zero initialization
  unsigned char *bytes = (unsigned char *)cmem;
  for (int i = 0; i < 100; i++) {
    TEST_ASSERT(bytes[i] == 0, "Calloc should zero-initialize memory");
  }
  
  // Test realloc
  void *rmem = coil_realloc(cmem, 200);
  TEST_ASSERT(rmem != NULL, "Realloc should succeed");
  
  // Clean up
  coil_free(rmem);
  
  return 0;
}

/**
* @brief Test memory utilities
*/
static int test_memory_utils() {
  printf("  Testing memory utilities...\n");
  
  // Create test data
  char src[128] = "Hello, COIL!";
  char dest[128] = {0};
  
  // Test memcpy
  void *result = coil_memcpy(dest, src, strlen(src) + 1);
  TEST_ASSERT(result == dest, "memcpy should return destination pointer");
  TEST_ASSERT(strcmp(dest, src) == 0, "memcpy should copy string correctly");
  
  // Test memset
  coil_memset(dest, 'A', 5);
  TEST_ASSERT(dest[0] == 'A' && dest[4] == 'A', "memset should set bytes correctly");
  TEST_ASSERT(dest[5] == ',', "memset should not affect bytes beyond count");
  
  // Test memcmp
  TEST_ASSERT(coil_memcmp(dest, "AAAAA, COIL!", 13) == 0, "memcmp should compare bytes correctly");
  TEST_ASSERT(coil_memcmp(dest, "AAAAB, COIL!", 13) < 0, "memcmp should return negative for dest < src");
  TEST_ASSERT(coil_memcmp(dest, "AAAA, COIL!", 12) > 0, "memcmp should return positive for dest > src");
  
  // Test memmove with overlapping regions
  char overlap[20] = "abcdefghijklmnopqrs";
  coil_memmove(overlap + 5, overlap, 10);
  TEST_ASSERT(memcmp(overlap + 5, "abcdefghij", 10) == 0, "memmove should handle overlapping regions");
  
  return 0;
}

/**
* @brief Test alignment functions
*/
static int test_alignment() {
  printf("  Testing alignment functions...\n");
  
  // Test align_up
  TEST_ASSERT(coil_align_up(1, 4) == 4, "align_up(1, 4) should be 4");
  TEST_ASSERT(coil_align_up(4, 4) == 4, "align_up(4, 4) should be 4");
  TEST_ASSERT(coil_align_up(5, 4) == 8, "align_up(5, 4) should be 8");
  TEST_ASSERT(coil_align_up(16, 16) == 16, "align_up(16, 16) should be 16");
  TEST_ASSERT(coil_align_up(17, 16) == 32, "align_up(17, 16) should be 32");
  
  // Test align_down
  TEST_ASSERT(coil_align_down(1, 4) == 0, "align_down(1, 4) should be 0");
  TEST_ASSERT(coil_align_down(4, 4) == 4, "align_down(4, 4) should be 4");
  TEST_ASSERT(coil_align_down(5, 4) == 4, "align_down(5, 4) should be 4");
  TEST_ASSERT(coil_align_down(16, 16) == 16, "align_down(16, 16) should be 16");
  TEST_ASSERT(coil_align_down(17, 16) == 16, "align_down(17, 16) should be 16");
  
  // Test aligned_size
  TEST_ASSERT(coil_aligned_size(1, 4) == 4, "aligned_size(1, 4) should be 4");
  TEST_ASSERT(coil_aligned_size(4, 4) == 4, "aligned_size(4, 4) should be 4");
  TEST_ASSERT(coil_aligned_size(5, 4) == 8, "aligned_size(5, 4) should be 8");
  
  return 0;
}

/**
* @brief Test mmap functions
*/
static int test_mmap() {
  printf("  Testing mmap functions...\n");
  
  // Get page size
  coil_size_t page_size = coil_get_page_size();
  TEST_ASSERT(page_size > 0, "Page size should be positive");
  
  // Test allocation
  void *mem = coil_mmap(page_size, page_size);
  TEST_ASSERT(mem != NULL, "mmap allocation should succeed");
  
  // Test we can write to the memory
  memset(mem, 0xAA, page_size);
  
  // Test freeing
  TEST_ASSERT(coil_munmap(mem, page_size) == COIL_ERR_GOOD, "munmap should succeed");
  
  return 0;
}

/**
* @brief Run all memory management tests
*/
int test_mem() {
  printf("\nRunning memory management tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_basic_memory_ops();
  result |= test_memory_utils();
  result |= test_alignment();
  result |= test_mmap();
  
  if (result == 0) {
    printf("All memory management tests passed!\n");
  }
  
  return result;
}