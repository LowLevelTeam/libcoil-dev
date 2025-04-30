/**
* @file test_mmap.c
* @brief Test suite for memory mapping functionality
*
* @author Low Level Team
*/

#include <coil/obj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

#define TEST_MMAP_OBJECT_FILE "test_mmap.coil"
#define TEST_MMAP_SECTION_FILE "test_mmap_section.dat"

/**
* @brief Create a test object file
*/
static int create_test_object_file() {
  printf("  Creating test object file...\n");
  
  // Initialize an object
  coil_object_t obj;
  coil_err_t err = coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization should succeed");
  
  // Create multiple sections with different content
  const char *section_data[] = {
    "This is section 1 content - Testing memory mapping functionality",
    "Section 2 has different content - COIL library rocks",
    "The third section contains some technical data: [0x1234, 0x5678, 0xABCD]",
    "This section will contain some native code (though it's just text for testing)"
  };
  
  const char *section_names[] = {
    ".text",
    ".data",
    ".debug",
    ".native"
  };
  
  // Create each section
  for (int i = 0; i < 4; i++) {
    coil_section_t sect;
    err = coil_section_init(&sect, 1024);
    TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
    
    // Write data to the section
    size_t section_len = strlen(section_data[i]);
    coil_size_t bytes_written;
    
    err = coil_section_write(&sect, (coil_byte_t *)section_data[i], section_len, &bytes_written);
    TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
    
    // Add the section to the object
    coil_u16_t sect_index;
    coil_u8_t section_type = (i == 3) ? COIL_SECTION_NATIVE : COIL_SECTION_PROGBITS;
    coil_u16_t section_flags = (i == 0) ? COIL_SECTION_FLAG_CODE : 
                               (i == 3) ? COIL_SECTION_FLAG_NATIVE : COIL_SECTION_FLAG_NONE;
    
    err = coil_obj_create_section(&obj, section_type, section_names[i], 
                                 section_flags, &sect, &sect_index);
    TEST_ASSERT(err == COIL_ERR_GOOD, "Creating section should succeed");
    
    // For the native section, set native metadata
    if (i == 3) {
      // Get the section
      coil_section_t native_sect;
      err = coil_obj_load_section(&obj, sect_index, &native_sect, COIL_SECT_MODE_MODIFY);
      TEST_ASSERT(err == COIL_ERR_GOOD, "Loading section should succeed");
      
      // Update the section
      err = coil_obj_update_section(&obj, sect_index, &native_sect);
      TEST_ASSERT(err == COIL_ERR_GOOD, "Updating section should succeed");
    }
  }
  
  // Open a file for writing
  int fd = open(TEST_MMAP_OBJECT_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Save the object to the file
  err = coil_obj_save_file(&obj, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Saving object should succeed");
  
  // Close the file
  close(fd);
  
  // Clean up
  coil_obj_cleanup(&obj);
  
  return 0;
}

/**
* @brief Create a test data file for section mapping
*/
static int create_test_section_file() {
  printf("  Creating test section file...\n");
  
  // Generate test data
  char test_data[4096];
  for (int i = 0; i < 4096; i++) {
    test_data[i] = (char)('A' + (i % 26));
  }
  
  // Add some markers
  const char *marker1 = "<<SECTION_START>>";
  const char *marker2 = "<<SECTION_MIDDLE>>";
  const char *marker3 = "<<SECTION_END>>";
  
  memcpy(test_data, marker1, strlen(marker1));
  memcpy(test_data + 2048, marker2, strlen(marker2));
  memcpy(test_data + 4096 - strlen(marker3), marker3, strlen(marker3));
  
  // Write to file
  int fd = open(TEST_MMAP_SECTION_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  ssize_t written = write(fd, test_data, 4096);
  TEST_ASSERT(written == 4096, "Writing test data should succeed");
  
  close(fd);
  
  return 0;
}

/**
* @brief Test memory mapping an object file
*/
static int test_object_mmap() {
  printf("  Testing object memory mapping...\n");
  
  // Open the test file
  int fd = open(TEST_MMAP_OBJECT_FILE, O_RDWR);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Load the object using memory mapping
  coil_object_t obj;
  coil_err_t err = coil_obj_mmap(&obj, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Memory mapping object should succeed");
  TEST_ASSERT(obj.is_mapped, "is_mapped flag should be set");
  TEST_ASSERT(obj.memory != NULL, "mapped memory should not be NULL");
  
  // Verify object content
  TEST_ASSERT(obj.header.section_count == 4, "Should have 4 sections");
  TEST_ASSERT(obj.header.magic[0] == 'C' && obj.header.magic[1] == 'O' &&
              obj.header.magic[2] == 'I' && obj.header.magic[3] == 'L', 
              "Magic should be COIL");
  
  // Load a section using VIEW mode
  coil_section_t sect;
  err = coil_obj_load_section(&obj, 0, &sect, COIL_SLOAD_VIEW);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Loading section in VIEW mode should succeed");
  TEST_ASSERT(sect.mode == COIL_SECT_MODE_VIEW, "Section mode should be VIEW");
  
  // Verify section content
  const char *expected_content = "This is section 1 content - Testing memory mapping functionality";
  TEST_ASSERT(sect.size == strlen(expected_content), "Section size should match expected content length");
  TEST_ASSERT(memcmp(sect.data, expected_content, sect.size) == 0, "Section content should match");
  
  // Test converting from memory-mapped to regular object
  err = coil_obj_unmap(&obj);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Converting to regular object should succeed");
  TEST_ASSERT(!obj.is_mapped, "is_mapped flag should be cleared");
  TEST_ASSERT(obj.memory == NULL, "memory should be NULL after unmapping");
  
  // Verify we can still access sections
  coil_section_t sect2;
  err = coil_obj_load_section(&obj, 1, &sect2, COIL_SECT_MODE_MODIFY);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Loading section after unmapping should succeed");
  
  const char *expected_content2 = "Section 2 has different content - COIL library rocks";
  TEST_ASSERT(sect2.size == strlen(expected_content2), "Section size should match expected content length");
  TEST_ASSERT(memcmp(sect2.data, expected_content2, sect2.size) == 0, "Section content should match");
  
  // Clean up
  coil_section_cleanup(&sect);
  coil_section_cleanup(&sect2);
  coil_obj_cleanup(&obj);
  close(fd);
  
  return 0;
}

/**
* @brief Test memory mapping a section directly
*/
static int test_section_mmap() {
  printf("  Testing section memory mapping...\n");
  
  // Open the test file
  int fd = open(TEST_MMAP_SECTION_FILE, O_RDONLY);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Map the section
  coil_section_t sect;
  coil_err_t err = coil_section_loadv(&sect, 1024, fd);  // Map first 1KB
  TEST_ASSERT(err == COIL_ERR_GOOD, "Memory mapping section should succeed");
  TEST_ASSERT(sect.mode == COIL_SECT_MODE_VIEW, "Section mode should be VIEW");
  TEST_ASSERT(sect.size == 1024, "Section size should match requested size");
  TEST_ASSERT(sect.is_mapped, "is_mapped flag should be set");
  TEST_ASSERT(sect.map_base != NULL, "map_base should not be NULL");
  
  // Verify section content (start marker)
  const char *marker = "<<SECTION_START>>";
  TEST_ASSERT(memcmp(sect.data, marker, strlen(marker)) == 0, 
             "Section start marker should match");
  
  // Clean up first section
  coil_section_cleanup(&sect);
  
  // Map a different part of the file
  lseek(fd, 2000, SEEK_SET);  // Move to position 2000
  err = coil_section_loadv(&sect, 100, fd);  // Map 100 bytes from current position
  TEST_ASSERT(err == COIL_ERR_GOOD, "Memory mapping section at offset should succeed");
  TEST_ASSERT(sect.is_mapped, "is_mapped flag should be set for second mapping");
  
  // Verify we're near the middle marker (should be at offset 2048)
  TEST_ASSERT(sect.size == 100, "Section size should match requested size");
  
  // Print first few bytes of the mapped section
  char buffer[10];
  memcpy(buffer, sect.data, 9);
  buffer[9] = '\0';
  printf("  Mapped section content: %.9s...\n", buffer);
  
  // Try modifying the view (should fail)
  coil_size_t bytes_written;
  err = coil_section_write(&sect, (coil_byte_t *)"test", 4, &bytes_written);
  TEST_ASSERT(err != COIL_ERR_GOOD, "Writing to VIEW mode section should fail");
  
  // Clean up
  coil_section_cleanup(&sect);
  close(fd);
  
  return 0;
}

/**
* @brief Run all memory mapping tests
*/
int test_mmap() {
  printf("\nRunning memory mapping tests...\n");
  
  int result = 0;
  
  // Create test files first
  result |= create_test_object_file();
  result |= create_test_section_file();
  
  // Run individual test functions
  result |= test_object_mmap();
  result |= test_section_mmap();
  
  // Clean up test files
  unlink(TEST_MMAP_OBJECT_FILE);
  unlink(TEST_MMAP_SECTION_FILE);
  
  if (result == 0) {
    printf("All memory mapping tests passed!\n");
  }
  
  return result;
}