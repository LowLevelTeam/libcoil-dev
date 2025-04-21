/**
 * @file test_coil.c
 * @brief Tests for the main COIL library functionality
 */

#include "test_framework.h"
#include <coil/coil.h>
#include <string.h>

// Test library initialization and shutdown
void test_coil_init_shutdown(void) {
  // Should not be initialized initially
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should start uninitialized");
  
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should be initialized after init");
  
  // Try to initialize again (should succeed but be a no-op)
  err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Double initialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should still be initialized");
  
  // Shutdown
  coil_shutdown();
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should be uninitialized after shutdown");
  
  // Shutdown again (should be a no-op)
  coil_shutdown();
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should still be uninitialized");
  
  // Initialize again to ensure we can reinitialize
  err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Reinitialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should be initialized after reinit");
  
  // Clean up
  coil_shutdown();
}

// Test version information retrieval
void test_coil_version(void) {
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  
  // Get version
  coil_version_t version;
  err = coil_get_version(&version);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Get version should succeed");
  
  // Check version information
  TEST_ASSERT(version.major >= 0, "Major version should be valid");
  TEST_ASSERT(version.minor >= 0, "Minor version should be valid");
  TEST_ASSERT(version.patch >= 0, "Patch version should be valid");
  
  // String should contain version numbers
  char expected_prefix[32];
  sprintf(expected_prefix, "COIL %d.%d.%d", version.major, version.minor, version.patch);
  TEST_ASSERT(strstr(version.string, expected_prefix) != NULL, "Version string should contain version numbers");
  
  // Build string should not be empty
  TEST_ASSERT(version.build != NULL && strlen(version.build) > 0, "Build string should not be empty");
  
  // Test with NULL pointer
  err = coil_get_version(NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "NULL pointer should fail");
  
  // Clean up
  coil_shutdown();
}

// Test configuration information retrieval
void test_coil_configuration(void) {
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  
  // Get configuration
  coil_configuration_t config;
  err = coil_get_configuration(&config);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Get configuration should succeed");
  
  // Check configuration validity (debug and asserts can be either 0 or 1)
  TEST_ASSERT(config.debug_enabled == 0 || config.debug_enabled == 1, 
              "Debug enabled should be valid boolean");
  TEST_ASSERT(config.asserts_enabled == 0 || config.asserts_enabled == 1, 
              "Asserts enabled should be valid boolean");
              
  // Test with NULL pointer
  err = coil_get_configuration(NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "NULL pointer should fail");
  
  // Clean up
  coil_shutdown();
}

// Test end-to-end integration
void test_coil_integration(void) {
  // Initialize library
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  
  // Create arena
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Create object
  coil_object_t* obj = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(obj, "Object creation should succeed");
  
  // Initialize string table
  err = coil_object_init_string_table(obj, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "String table init should succeed");
  
  // Add string
  coil_u64_t name_offset = coil_object_add_string(obj, ".text", arena);
  TEST_ASSERT(name_offset > 0, "String addition should succeed");
  
  // Create some instructions in the arena
  coil_arena_t* instr_arena = arena_init(1024, 0);
  TEST_ASSERT_NOT_NULL(instr_arena, "Instruction arena creation should succeed");
  
  // MOV r1, #42
  encode_instr(instr_arena, COIL_OP_MOV, 2);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  uint32_t val = 42;
  encode_operand_imm(instr_arena, COIL_VAL_U32, COIL_MOD_CONST, &val);
  
  // MOV r2, #13
  encode_instr(instr_arena, COIL_OP_MOV, 2);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
  val = 13;
  encode_operand_imm(instr_arena, COIL_VAL_U32, COIL_MOD_CONST, &val);
  
  // ADD r3, r1, r2
  encode_instr(instr_arena, COIL_OP_ADD, 3);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 3);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
  
  // Get instruction data
  size_t code_size = arena_used(instr_arena);
  void* code_data = instr_arena->first_block->memory;
  
  // Add code section
  coil_u16_t section_idx = coil_object_add_section(
    obj,
    name_offset,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    code_data,
    code_size,
    arena
  );
  TEST_ASSERT(section_idx > 0, "Section addition should succeed");
  
  // Save object to memory
  void* obj_data;
  size_t obj_size;
  err = coil_object_save_to_memory(obj, arena, &obj_data, &obj_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Object saving should succeed");
  TEST_ASSERT_NOT_NULL(obj_data, "Object data should be allocated");
  TEST_ASSERT(obj_size > 0, "Object size should be positive");
  
  // Create a new object
  coil_object_t* new_obj = coil_object_create(arena);
  TEST_ASSERT_NOT_NULL(new_obj, "New object creation should succeed");
  
  // Load the saved data
  err = coil_object_load_from_memory(new_obj, obj_data, obj_size, arena);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Object loading should succeed");
  
  // Verify the loaded object
  coil_section_header_t header;
  const void* loaded_data;
  coil_u64_t loaded_size;
  err = coil_object_get_section(new_obj, section_idx, &header, &loaded_data, &loaded_size);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Section retrieval should succeed");
  TEST_ASSERT_EQUAL_SIZE(code_size, loaded_size, "Code size should match");
  TEST_ASSERT_EQUAL_INT(0, memcmp(code_data, loaded_data, code_size), "Code data should match");
  
  // Clean up
  arena_destroy(instr_arena);
  coil_object_destroy(obj, arena);
  coil_object_destroy(new_obj, arena);
  arena_destroy(arena);
  coil_shutdown();
}

// Define test array
test_t coil_tests[] = {
  {"COIL Init/Shutdown", test_coil_init_shutdown},
  {"COIL Version", test_coil_version},
  {"COIL Configuration", test_coil_configuration},
  {"COIL Integration", test_coil_integration}
};

// Run COIL tests
void run_coil_tests(void) {
  run_tests(coil_tests, sizeof(coil_tests) / sizeof(coil_tests[0]));
}