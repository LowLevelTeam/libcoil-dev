/**
 * @file test_obj.c
 * @brief Tests for the COIL object format
 */

#include "test_framework.h"
#include <coil/obj.h>
#include <coil/arena.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper function to print object details
void print_object_details(const coil_object_t* obj, const char* label) {
  if (!g_test_verbose || !obj) return;
  
  printf("Object details for %s:\n", label);
  
  const coil_object_header_t* header = coil_object_get_header(obj);
  if (!header) {
    printf("  ERROR: Could not get object header\n");
    return;
  }
  
  printf("  Magic:        '%c%c%c%c'\n", 
         header->magic[0], header->magic[1], header->magic[2], header->magic[3]);
  printf("  Version:      0x%04x\n", header->version);
  printf("  Section count: %d\n", header->section_count);
  printf("  File size:     %llu bytes\n", (unsigned long long)header->file_size);
  
  // Print section info
  for (coil_u16_t i = 1; i <= header->section_count; i++) {
    coil_section_header_t sec_header;
    const void* sec_data;
    coil_u64_t sec_size;
    
    if (coil_object_get_section(obj, i, &sec_header, &sec_data, &sec_size) == COIL_ERR_GOOD) {
      char sec_name[64] = {0};
      coil_object_get_string(obj, sec_header.name, sec_name, sizeof(sec_name));
      
      printf("  Section %d: '%s'\n", i, sec_name);
      printf("    Type:  %d\n", sec_header.type);
      printf("    Flags: 0x%04x\n", sec_header.flags);
      printf("    Size:  %llu bytes\n", (unsigned long long)sec_size);
      
      if (g_test_verbose > 1 && sec_data && sec_size > 0 && sec_size < 256) {
        // For very verbose mode, dump section content for small sections
        hexdump(sec_data, sec_size, sec_name);
      }
    } else {
      printf("  Section %d: <error retrieving data>\n", i);
    }
  }
  printf("\n");
}

// Function to create a temporary file name
static char* create_temp_filename(void) {
  static char filename[256];
  static int counter = 0;
  
#ifdef _WIN32
  sprintf(filename, "coil_test_%d.tmp", counter++);
#else
  sprintf(filename, "/tmp/coil_test_%d.tmp", counter++);
#endif
  
  return filename;
}

// Helper to create a basic object with a string table
static coil_object_t* create_basic_object(coil_arena_t* arena) {
  coil_object_t* obj = coil_object_create(arena);
  if (obj == NULL) {
    return NULL; // Return NULL if creation fails
  }
  
  // Initialize string table
  coil_err_t err = coil_object_init_string_table(obj, arena);
  if (err != COIL_ERR_GOOD) {
    return NULL; // Return NULL if string table init fails
  }
  
  return obj;
}

// Test object creation and destruction
void test_object_create_destroy(void) {
  // Test with arena
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation with arena should succeed");
  
  // Verify header initialization
  const coil_object_header_t* header = coil_object_get_header(obj);
  TEST_ASSERT_NOT_NULL(header, "Header should be available");
  
  // Check magic number
  const uint8_t expected_magic[] = COIL_MAGIC_BYTES;
  TEST_ASSERT_EQUAL_INT(expected_magic[0], header->magic[0], "Magic byte 0 should match");
  TEST_ASSERT_EQUAL_INT(expected_magic[1], header->magic[1], "Magic byte 1 should match");
  TEST_ASSERT_EQUAL_INT(expected_magic[2], header->magic[2], "Magic byte 2 should match");
  TEST_ASSERT_EQUAL_INT(expected_magic[3], header->magic[3], "Magic byte 3 should match");
  
  // Check version
  TEST_ASSERT_EQUAL_INT(COIL_VERSION, header->version, "Version should match");
  
  // Check section count
  TEST_ASSERT_EQUAL_INT(0, header->section_count, "Section count should be 0");
  
  // Destroy object (should be a no-op since arena owns memory)
  coil_object_destroy(obj, arena);
  
  // Free arena
  arena_destroy(arena);
  
  // Test with malloc
  obj = coil_object_create(NULL);
  TEST_ASSERT_NOT_NULL(obj, "Object creation with malloc should succeed");
  
  // Verify again
  header = coil_object_get_header(obj);
  TEST_ASSERT_NOT_NULL(header, "Header should be available");
  TEST_ASSERT_EQUAL_INT(COIL_VERSION, header->version, "Version should match");
  
  // Destroy object (should free memory)
  coil_object_destroy(obj, NULL);
}

// Test string table initialization and usage
void test_object_string_table(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Initialize string table
  coil_err_t err = coil_object_init_string_table(obj, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String table init should succeed");
  
  // Add some strings
  coil_u64_t offset1 = coil_object_add_string(obj, "hello", arena);
  TEST_ASSERT(offset1 > 0, "String addition should succeed");
  
  coil_u64_t offset2 = coil_object_add_string(obj, "world", arena);
  TEST_ASSERT(offset2 > 0, "String addition should succeed");
  TEST_ASSERT(offset2 > offset1, "Second string should have higher offset");
  
  // Add duplicate string - should return same offset
  coil_u64_t offset3 = coil_object_add_string(obj, "hello", arena);
  TEST_ASSERT_EQUAL_INT(offset1, offset3, "Duplicate string should return same offset");
  
  // Get strings back
  char buffer[256];
  err = coil_object_get_string(obj, offset1, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String retrieval should succeed");
  TEST_ASSERT_EQUAL_STR("hello", buffer, "Retrieved string should match");
  
  err = coil_object_get_string(obj, offset2, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String retrieval should succeed");
  TEST_ASSERT_EQUAL_STR("world", buffer, "Retrieved string should match");
  
  // Test with small buffer (truncation)
  err = coil_object_get_string(obj, offset1, buffer, 3);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String truncation should succeed");
  TEST_ASSERT_EQUAL_STR("he", buffer, "Truncated string should match");
  
  // Test invalid offset
  err = coil_object_get_string(obj, 9999, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "Invalid offset should fail");
  
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
}

// Test adding and retrieving sections
void test_object_sections(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = create_basic_object(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation with string table should succeed");
  
  // Get current section count before adding new section
  coil_u16_t initial_section_count = coil_object_get_section_count(obj);
  
  if (g_test_verbose) {
    printf("Initial section count: %d\n", initial_section_count);
    print_object_details(obj, "Object before adding sections");
  }
  
  // Add a section name
  coil_u64_t name1 = coil_object_add_string(obj, ".text", arena);
  TEST_ASSERT(name1 > 0, "Section name addition should succeed");
  
  // Create section data
  uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
  
  // Add a section
  coil_u16_t section1 = coil_object_add_section(
    obj,
    name1,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data1,
    sizeof(data1),
    arena
  );
  
  if (g_test_verbose) {
    printf("After adding .text section, index = %d\n", section1);
    print_object_details(obj, "Object after adding .text section");
  }
  
  // Check section was added properly and has an expected index
  TEST_ASSERT(section1 > 0, "Section addition should succeed");
  
  // The string table is already section 1, so our new section should be 2
  // Strtab index = 1, our new section index = 2
  TEST_ASSERT_EQUAL_INT(initial_section_count + 1, section1, "Section index should be one more than initial count");
  
  // Check section count
  coil_u16_t count = coil_object_get_section_count(obj);
  TEST_ASSERT_EQUAL_INT(initial_section_count + 1, count, "Section count should be increased by 1");
  
  // Add another section
  coil_u64_t name2 = coil_object_add_string(obj, ".data", arena);
  TEST_ASSERT(name2 > 0, "Section name addition should succeed");
  
  uint8_t data2[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  
  coil_u16_t section2 = coil_object_add_section(
    obj,
    name2,
    COIL_SECTION_FLAG_WRITE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data2,
    sizeof(data2),
    arena
  );
  
  if (g_test_verbose) {
    printf("After adding .data section, index = %d\n", section2);
    print_object_details(obj, "Object after adding .data section");
  }
  
  TEST_ASSERT(section2 > 0, "Section addition should succeed");
  TEST_ASSERT_EQUAL_INT(section1 + 1, section2, "Second section should have index one more than first section");
  
  // Get section by index
  coil_section_header_t header;
  const void* section_data;
  coil_u64_t section_size;
  
  coil_err_t err = coil_object_get_section(obj, section1, &header, &section_data, &section_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Section retrieval should succeed");
  TEST_ASSERT_EQUAL_UINT64(name1, header.name, "Section name offset should match");
  TEST_ASSERT_EQUAL_INT(COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC, header.flags, "Section flags should match");
  TEST_ASSERT_EQUAL_INT(COIL_SECTION_PROGBITS, header.type, "Section type should match");
  TEST_ASSERT_EQUAL_UINT64(sizeof(data1), section_size, "Section size should match");
  TEST_ASSERT_EQUAL_INT(0, memcmp(data1, section_data, sizeof(data1)), "Section data should match");
  
  // Get section by name
  coil_u16_t found_index = coil_object_get_section_index(obj, ".text");
  TEST_ASSERT_EQUAL_INT(section1, found_index, "Section index by name should match");
  
  // Try invalid index
  err = coil_object_get_section(obj, 99, &header, &section_data, &section_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "Invalid section index should fail");
  
  // Try invalid name
  found_index = coil_object_get_section_index(obj, "nonexistent");
  TEST_ASSERT_EQUAL_INT(0, found_index, "Nonexistent section name should return 0");
  
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
}

// Test symbol table functionality
void test_object_symbols(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = create_basic_object(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Initialize symbol table
  coil_err_t err = coil_object_init_symbol_table(obj, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Symbol table init should succeed");
  
  // Add a text section
  coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
  TEST_ASSERT(text_name > 0, "Section name addition should succeed");
  
  uint8_t code[] = {0x01, 0x02, 0x03, 0x04};
  coil_u16_t text_section = coil_object_add_section(
    obj, text_name, COIL_SECTION_FLAG_CODE, COIL_SECTION_PROGBITS, 
    code, sizeof(code), arena
  );
  TEST_ASSERT(text_section > 0, "Section addition should succeed");
  
  // Add symbols
  coil_u64_t sym1_name = coil_object_add_string(obj, "main", arena);
  TEST_ASSERT(sym1_name > 0, "Symbol name addition should succeed");
  
  coil_u16_t sym1 = coil_object_add_symbol(
    obj, sym1_name, 0, text_section, COIL_SYMBOL_FUNC, COIL_SYMBOL_GLOBAL, arena
  );
  TEST_ASSERT(sym1 > 0, "Symbol addition should succeed");
  
  coil_u64_t sym2_name = coil_object_add_string(obj, "data_var", arena);
  TEST_ASSERT(sym2_name > 0, "Symbol name addition should succeed");
  
  coil_u16_t sym2 = coil_object_add_symbol(
    obj, sym2_name, 16, text_section, COIL_SYMBOL_OBJECT, COIL_SYMBOL_LOCAL, arena
  );
  TEST_ASSERT(sym2 > 0, "Symbol addition should succeed");
  
  // Get symbols by index
  coil_symbol_t symbol;
  err = coil_object_get_symbol(obj, sym1, &symbol);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Symbol retrieval should succeed");
  TEST_ASSERT_EQUAL_UINT64(sym1_name, symbol.name, "Symbol name should match");
  TEST_ASSERT_EQUAL_INT(0, symbol.value, "Symbol value should match");
  TEST_ASSERT_EQUAL_INT(text_section, symbol.section_index, "Symbol section should match");
  TEST_ASSERT_EQUAL_INT(COIL_SYMBOL_FUNC, symbol.type, "Symbol type should match");
  TEST_ASSERT_EQUAL_INT(COIL_SYMBOL_GLOBAL, symbol.binding, "Symbol binding should match");
  
  // Get symbols by name
  coil_u16_t found_sym = coil_object_get_symbol_index(obj, "main");
  TEST_ASSERT_EQUAL_INT(sym1, found_sym, "Symbol index by name should match");
  
  found_sym = coil_object_get_symbol_index(obj, "data_var");
  TEST_ASSERT_EQUAL_INT(sym2, found_sym, "Symbol index by name should match");
  
  // Try nonexistent symbol
  found_sym = coil_object_get_symbol_index(obj, "nonexistent");
  TEST_ASSERT_EQUAL_INT(0, found_sym, "Nonexistent symbol should return 0");
  
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
}

// Test saving and loading objects
void test_object_save_load(void) {
  char* filename = create_temp_filename();
  
  // Create an object with some content
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = create_basic_object(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Add a section with data
  coil_u64_t name = coil_object_add_string(obj, ".text", arena);
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  coil_object_add_section(obj, name, COIL_SECTION_FLAG_CODE, 
                          COIL_SECTION_PROGBITS, data, sizeof(data), arena);
  
  // Save to file
  coil_err_t err = coil_object_save_to_file(obj, filename);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Saving object to file should succeed");
  
  // Load into a new object
  coil_object_t* loaded = coil_object_create(NULL); // Use malloc this time
  TEST_ASSERT_NOT_NULL(loaded, "Object creation should succeed");
  
  err = coil_object_load_from_file(loaded, filename, NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Loading object from file should succeed");
  
  // Verify loaded object
  const coil_object_header_t* header = coil_object_get_header(loaded);
  TEST_ASSERT_NOT_NULL(header, "Header should be available");
  
  const uint8_t expected_magic[] = COIL_MAGIC_BYTES;
  TEST_ASSERT_EQUAL_INT(expected_magic[0], header->magic[0], "Magic byte 0 should match");
  
  TEST_ASSERT_EQUAL_INT(COIL_VERSION, header->version, "Version should match");
  
  // Verify section count (string table + text section)
  TEST_ASSERT_EQUAL_INT(2, header->section_count, "Section count should be 2");
  
  // Get text section
  coil_section_header_t section_header;
  const void* section_data;
  coil_u64_t section_size;
  
  err = coil_object_get_section(loaded, 2, &section_header, &section_data, &section_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Section retrieval should succeed");
  TEST_ASSERT_EQUAL_SIZE(sizeof(data), section_size, "Section size should match");
  TEST_ASSERT_EQUAL_INT(0, memcmp(data, section_data, sizeof(data)), "Section data should match");
  
  // Get section name
  char buffer[256];
  err = coil_object_get_string(loaded, section_header.name, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String retrieval should succeed");
  TEST_ASSERT_EQUAL_STR(".text", buffer, "Section name should match");
  
  // Clean up
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
  
  coil_object_destroy(loaded, NULL);
  
  // Remove temporary file
  remove(filename);
}

// Test saving and loading in memory
void test_object_save_load_memory(void) {
  // Create an object with some content
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = create_basic_object(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Add a section with data
  coil_u64_t name = coil_object_add_string(obj, ".data", arena);
  uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  coil_object_add_section(obj, name, COIL_SECTION_FLAG_WRITE, 
                          COIL_SECTION_PROGBITS, data, sizeof(data), arena);
  
  // Save to memory
  void* mem_data;
  size_t mem_size;
  coil_err_t err = coil_object_save_to_memory(obj, arena, &mem_data, &mem_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Saving object to memory should succeed");
  TEST_ASSERT_NOT_NULL(mem_data, "Memory data should be allocated");
  TEST_ASSERT(mem_size > 0, "Memory size should be positive");
  
  // Create a new object to load into
  coil_object_t* loaded = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(loaded, "Object creation should succeed");
  
  // Load from memory
  err = coil_object_load_from_memory(loaded, mem_data, mem_size, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Loading object from memory should succeed");
  
  // Verify section count (string table + data section)
  coil_u16_t count = coil_object_get_section_count(loaded);
  TEST_ASSERT_EQUAL_INT(2, count, "Section count should be 2");
  
  // Find section by name
  coil_u16_t section_idx = coil_object_get_section_index(loaded, ".data");
  TEST_ASSERT(section_idx > 0, "Section lookup by name should succeed");
  
  // Get section data
  coil_section_header_t section_header;
  const void* section_data;
  coil_u64_t section_size;
  
  err = coil_object_get_section(loaded, section_idx, &section_header, &section_data, &section_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Section retrieval should succeed");
  TEST_ASSERT_EQUAL_SIZE(sizeof(data), section_size, "Section size should match");
  TEST_ASSERT_EQUAL_INT(0, memcmp(data, section_data, sizeof(data)), "Section data should match");
  
  // Clean up
  coil_object_destroy(obj, arena);
  coil_object_destroy(loaded, arena);
  arena_destroy(arena);
}

// Test error handling
void test_object_errors(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Test invalid args to string table init
  coil_err_t err = coil_object_init_string_table(NULL, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "NULL object should fail");
  
  // Test memory model mismatch
  err = coil_object_init_string_table(obj, NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "Memory model mismatch should fail");
  
  // Initialize string table properly
  err = coil_object_init_string_table(obj, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String table init should succeed");
  
  // Test duplicate initialization (should succeed)
  err = coil_object_init_string_table(obj, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Duplicate string table init should succeed");
  
  // Test invalid args to string functions
  coil_u64_t str_offset = coil_object_add_string(NULL, "test", arena);
  TEST_ASSERT_EQUAL_INT(0, str_offset, "NULL object should fail");
  
  str_offset = coil_object_add_string(obj, NULL, arena);
  TEST_ASSERT_EQUAL_INT(0, str_offset, "NULL string should fail");
  
  str_offset = coil_object_add_string(obj, "test", NULL);
  TEST_ASSERT_EQUAL_INT(0, str_offset, "Memory model mismatch should fail");
  
  // Test invalid args to section functions
  coil_u16_t section_idx = coil_object_add_section(NULL, 0, 0, 0, NULL, 0, arena);
  TEST_ASSERT_EQUAL_INT(0, section_idx, "NULL object should fail");
  
  section_idx = coil_object_add_section(obj, 0, 0, 0, NULL, 0, NULL);
  TEST_ASSERT_EQUAL_INT(0, section_idx, "Memory model mismatch should fail");
  
  // Test invalid load/save operations
  err = coil_object_load_from_file(NULL, "nonexistent.file", arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL object should fail");
  
  err = coil_object_load_from_file(obj, NULL, arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL filename should fail");
  
  err = coil_object_load_from_file(obj, "nonexistent.file", arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "Nonexistent file should fail");
  
  err = coil_object_save_to_file(NULL, "output.file");
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL object should fail");
  
  err = coil_object_save_to_file(obj, NULL);
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL filename should fail");
  
  err = coil_object_load_from_memory(NULL, NULL, 0, arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL object should fail");
  
  err = coil_object_load_from_memory(obj, NULL, 10, arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "NULL data should fail");
  
  uint8_t bad_data[] = {0x01, 0x02, 0x03, 0x04};
  err = coil_object_load_from_memory(obj, bad_data, sizeof(bad_data), arena);
  TEST_ASSERT(err != COIL_ERR_GOOD, "Invalid magic number should fail");
  
  // Clean up
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
}

// Test extreme cases and edge cases
void test_object_extreme(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  coil_object_t* obj = create_basic_object(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Test empty section
  coil_u64_t name = coil_object_add_string(obj, ".empty", arena);
  coil_u16_t section_idx = coil_object_add_section(
    obj, name, 0, COIL_SECTION_NOBITS, NULL, 0, arena
  );
  TEST_ASSERT(section_idx > 0, "Empty section should be added");
  
  // Test very large string
  char* large_string = arena_alloc_default(arena, 1001); // Allocate 1001 bytes (1000 + null terminator)
  TEST_ASSERT_NOT_NULL(large_string, "Large string allocation should succeed");
  
  // Fill with 1000 'A's and null-terminate
  memset(large_string, 'A', 1000);
  large_string[1000] = '\0';
  
  if (g_test_verbose) {
    printf("Large string length: %zu\n", strlen(large_string));
  }
  
  coil_u64_t large_str_offset = coil_object_add_string(obj, large_string, arena);
  TEST_ASSERT(large_str_offset > 0, "Large string should be added");
  
  char buffer[1100];
  coil_err_t err = coil_object_get_string(obj, large_str_offset, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Large string retrieval should succeed");
  
  if (g_test_verbose) {
    printf("Retrieved string length: %zu\n", strlen(buffer));
    printf("First 20 chars: %.20s\n", buffer);
    printf("Last 20 chars: %s\n", buffer + strlen(buffer) - 20);
  }
  
  TEST_ASSERT_EQUAL_SIZE(1000, strlen(buffer), "Large string length should match");
  
  // Test small buffer for string retrieval
  char small_buffer[10];
  err = coil_object_get_string(obj, large_str_offset, small_buffer, sizeof(small_buffer));
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String retrieval with small buffer should succeed");
  TEST_ASSERT_EQUAL_SIZE(9, strlen(small_buffer), "Truncated string length should match");
  
  // Test large section data
  uint8_t* large_data = arena_alloc_default(arena, 10000);
  TEST_ASSERT_NOT_NULL(large_data, "Large data allocation should succeed");
  
  for (int i = 0; i < 10000; i++) {
    large_data[i] = (uint8_t)(i & 0xFF);
  }
  
  coil_u64_t large_name = coil_object_add_string(obj, ".large", arena);
  section_idx = coil_object_add_section(
    obj, large_name, COIL_SECTION_FLAG_ALLOC, 
    COIL_SECTION_PROGBITS, large_data, 10000, arena
  );
  TEST_ASSERT(section_idx > 0, "Large section should be added");
  
  if (g_test_verbose) {
    print_object_details(obj, "Object with large string and section");
  }
  
  // Clean up
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
}

// Define test array
test_t obj_tests[] = {
  {"Object Create/Destroy", test_object_create_destroy},
  {"Object String Table", test_object_string_table},
  {"Object Sections", test_object_sections},
  {"Object Symbols", test_object_symbols},
  {"Object Save/Load File", test_object_save_load},
  {"Object Save/Load Memory", test_object_save_load_memory},
  {"Object Error Handling", test_object_errors},
  {"Object Extreme Cases", test_object_extreme}
};

// Run object tests
void run_obj_tests(void) {
  run_tests(obj_tests, sizeof(obj_tests) / sizeof(obj_tests[0]));
}