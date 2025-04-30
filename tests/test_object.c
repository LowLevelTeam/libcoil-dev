/**
* @file test_object.c
* @brief Test suite for object functionality
*
* @author Low Level Team
*/

#include <coil/obj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

#define TEST_OBJECT_FILE "test_object.coil"

/**
* @brief Test object initialization and cleanup
*/
static int test_object_init_cleanup() {
  printf("  Testing object init/cleanup...\n");
  
  // Initialize an object
  coil_object_t obj;
  coil_err_t err = coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization should succeed");
  TEST_ASSERT(obj.header.magic[0] == 'C' && obj.header.magic[1] == 'O' &&
              obj.header.magic[2] == 'I' && obj.header.magic[3] == 'L', 
              "Object magic should be set");
  TEST_ASSERT(obj.header.section_count == 0, "Section count should be zero");
  TEST_ASSERT(obj.fd == -1, "File descriptor should be invalid");
  
  // Clean up the object
  coil_obj_cleanup(&obj);
  
  // Initialize with native flag
  err = coil_obj_init(&obj, COIL_OBJ_INIT_NATIVE);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization with native flag should succeed");
  TEST_ASSERT(obj.header.has_native == 1, "has_native flag should be set");
  
  // Set native defaults
  err = coil_obj_set_native_defaults(&obj, COIL_PU_CPU, COIL_CPU_x86_64, COIL_CPU_X86_AVX2);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Setting native defaults should succeed");
  TEST_ASSERT(obj.default_pu == COIL_PU_CPU, "Default PU should be set");
  TEST_ASSERT(obj.default_arch == COIL_CPU_x86_64, "Default architecture should be set");
  TEST_ASSERT(obj.default_features == COIL_CPU_X86_AVX2, "Default features should be set");
  
  // Clean up
  coil_obj_cleanup(&obj);
  
  return 0;
}

/**
* @brief Test section creation and manipulation
*/
static int test_object_sections() {
  printf("  Testing object section operations...\n");
  
  // Initialize an object
  coil_object_t obj;
  coil_err_t err = coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization should succeed");
  
  // Create a section
  coil_section_t sect;
  err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write some data to the section
  const char *test_data = "Test section data";
  coil_size_t test_len = strlen(test_data);
  coil_size_t bytes_written;
  
  err = coil_section_write(&sect, (coil_byte_t *)test_data, test_len, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
  
  // Add the section to the object
  coil_u16_t sect_index;
  err = coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".text", 
                              COIL_SECTION_FLAG_CODE, &sect, &sect_index);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Creating section should succeed");
  TEST_ASSERT(sect_index == 0, "First section index should be 0");
  TEST_ASSERT(obj.header.section_count == 1, "Section count should be 1");
  
  // Find the section by name
  coil_u16_t found_index;
  err = coil_obj_find_section(&obj, ".text", &found_index);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Finding section should succeed");
  TEST_ASSERT(found_index == sect_index, "Found index should match");
  
  // Create another section
  coil_section_t sect2;
  err = coil_section_init(&sect2, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write some data to the section
  const char *test_data2 = "Second section data";
  coil_size_t test_len2 = strlen(test_data2);
  
  err = coil_section_write(&sect2, (coil_byte_t *)test_data2, test_len2, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
  
  // Add the second section to the object
  coil_u16_t sect_index2;
  err = coil_obj_create_section(&obj, COIL_SECTION_STRTAB, ".strtab", 
                              COIL_SECTION_FLAG_NONE, &sect2, &sect_index2);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Creating section should succeed");
  TEST_ASSERT(sect_index2 == 1, "Second section index should be 1");
  TEST_ASSERT(obj.header.section_count == 2, "Section count should be 2");
  
  // Try to load the sections
  coil_section_t loaded_sect;
  err = coil_obj_load_section(&obj, sect_index, &loaded_sect, COIL_SECT_MODE_MODIFY);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Loading section should succeed");
  TEST_ASSERT(loaded_sect.size == test_len, "Loaded section size should match");
  TEST_ASSERT(memcmp(loaded_sect.data, test_data, test_len) == 0, "Loaded data should match");
  
  // Clean up sections
  coil_section_cleanup(&sect);
  coil_section_cleanup(&sect2);
  coil_section_cleanup(&loaded_sect);
  
  // Delete a section
  err = coil_obj_delete_section(&obj, sect_index);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Deleting section should succeed");
  TEST_ASSERT(obj.header.section_count == 1, "Section count should be 1");
  
  // Clean up the object
  coil_obj_cleanup(&obj);
  
  return 0;
}

/**
* @brief Test native code section creation
*/
static int test_native_section() {
  printf("  Testing native code section...\n");
  
  // Initialize an object with native flag
  coil_object_t obj;
  coil_err_t err = coil_obj_init(&obj, COIL_OBJ_INIT_NATIVE);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization should succeed");
  
  // Create some fake native code
  const char *native_code = "This is fake x86-64 native code";
  coil_size_t code_len = strlen(native_code);
  
  // TODO...

  // Clean up the object
  coil_obj_cleanup(&obj);
  
  return 0;
}

/**
* @brief Test object file I/O
*/
static int test_object_file_io() {
  printf("  Testing object file I/O...\n");
  
  // Initialize an object
  coil_object_t obj;
  coil_err_t err = coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Object initialization should succeed");
  
  // Create a section
  coil_section_t sect;
  err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Write some data to the section
  const char *test_data = "Test object file I/O data";
  coil_size_t test_len = strlen(test_data);
  coil_size_t bytes_written;
  
  err = coil_section_write(&sect, (coil_byte_t *)test_data, test_len, &bytes_written);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section write should succeed");
  
  // Add the section to the object
  coil_u16_t sect_index;
  err = coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".data", 
                              COIL_SECTION_FLAG_WRITE, &sect, &sect_index);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Creating section should succeed");
  
  // Open a file for writing
  int fd = open(TEST_OBJECT_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
  TEST_ASSERT(fd >= 0, "File open should succeed");
  
  // Save the object to the file
  err = coil_obj_save_file(&obj, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Saving object should succeed");
  
  // Close the file
  close(fd);
  
  // Clean up the original object and section
  coil_section_cleanup(&sect);
  coil_obj_cleanup(&obj);
  
  // Open the file again for reading
  fd = open(TEST_OBJECT_FILE, O_RDONLY);
  TEST_ASSERT(fd >= 0, "File open for reading should succeed");
  
  // Load a new object from the file
  coil_object_t loaded_obj;
  err = coil_obj_load_file(&loaded_obj, fd);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Loading object should succeed");
  TEST_ASSERT(loaded_obj.header.section_count == 1, "Loaded section count should be 1");
  
  // Find the section by name
  coil_u16_t found_index;
  err = coil_obj_find_section(&loaded_obj, ".data", &found_index);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Finding section should succeed");
  
  // Load the section
  coil_section_t loaded_sect;
  err = coil_obj_load_section(&loaded_obj, found_index, &loaded_sect, COIL_SECT_MODE_MODIFY);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Loading section should succeed");
  TEST_ASSERT(loaded_sect.size == test_len, "Loaded section size should match");
  TEST_ASSERT(memcmp(loaded_sect.data, test_data, test_len) == 0, "Loaded data should match");
  
  // Close the file
  close(fd);
  
  // Clean up
  coil_section_cleanup(&loaded_sect);
  coil_obj_cleanup(&loaded_obj);
  
  return 0;
}

/**
* @brief Run all object tests
*/
int test_object() {
  printf("\nRunning object tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_object_init_cleanup();
  result |= test_object_sections();
  result |= test_native_section();
  result |= test_object_file_io();
  
  // Clean up test file
  unlink(TEST_OBJECT_FILE);
  
  if (result == 0) {
    printf("All object tests passed!\n");
  }
  
  return result;
}