/**
 * @file test_integration.c
 * @brief Integration tests for the COIL library
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <cmocka.h>

#include <coil/coil.h>
#include <coil/arena.h>
#include <coil/err.h>
#include <coil/instr.h>
#include <coil/obj.h>

#define TEST_FILE_PATH "test_integration.coil"

/* Custom error callback for testing */
static int error_callback_called = 0;
static coil_error_level_t last_error_level = 0;

static void test_error_callback(
  coil_error_level_t level,
  const char* message,
  const coil_error_position_t* position,
  void* user_data
) {
  (void)message;
  (void)position;
  (void)user_data;
  
  error_callback_called = 1;
  last_error_level = level;
}

/* Setup function for tests */
static int setup(void **state) {
  /* Initialize the library */
  coil_err_t err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Create an arena for testing */
  coil_arena_t *arena = arena_init(4096, 0);
  if (!arena) {
    coil_shutdown();
    return -1;
  }
  
  /* Reset callback tracking */
  error_callback_called = 0;
  last_error_level = 0;
  
  *state = arena;
  return 0;
}

/* Teardown function for tests */
static int teardown(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  if (arena) {
    arena_destroy(arena);
  }
  
  /* Remove test file if it exists */
  remove(TEST_FILE_PATH);
  
  /* Shutdown the library */
  coil_shutdown();
  
  return 0;
}

/* Test creating and saving a complete program */
static void test_create_full_program(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create an object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  coil_err_t err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Create a code section */
  coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
  assert_true(text_name > 0);
  
  /* Create a data section */
  coil_u64_t data_name = coil_object_add_string(obj, ".data", arena);
  assert_true(data_name > 0);
  
  /* Add section names to string table */
  coil_u64_t main_name = coil_object_add_string(obj, "main", arena);
  assert_true(main_name > 0);
  
  /* Create a temporary arena for generating code */
  coil_arena_t *code_arena = arena_init(1024, 0);
  assert_non_null(code_arena);
  
  /*
   * Generate code for a simple program:
   * int main() {
   *     int a = 42;
   *     int b = 13;
   *     return a + b;
   * }
   */
  
  /* MOV r1, #42 */
  encode_instr(code_arena, COIL_OP_MOV, 2);
  encode_operand_u32(code_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  uint32_t val_a = 42;
  encode_operand_imm(code_arena, COIL_VAL_I32, COIL_MOD_NONE, &val_a);
  
  /* MOV r2, #13 */
  encode_instr(code_arena, COIL_OP_MOV, 2);
  encode_operand_u32(code_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
  uint32_t val_b = 13;
  encode_operand_imm(code_arena, COIL_VAL_I32, COIL_MOD_NONE, &val_b);
  
  /* ADD r0, r1, r2 */
  encode_instr(code_arena, COIL_OP_ADD, 3);
  encode_operand_u32(code_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_u32(code_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  encode_operand_u32(code_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
  
  /* RET */
  encode_instr_void(code_arena, COIL_OP_RET);
  
  /* Add code section */
  size_t code_size = arena_used(code_arena);
  void *code_data = arena_alloc(code_arena, 0, 1); /* Get pointer to start of arena */
  
  coil_u16_t text_index = coil_object_add_section(
    obj,
    text_name,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    code_data,
    code_size,
    arena
  );
  
  assert_true(text_index > 0);
  
  /* Create data section with some example data */
  uint32_t data_values[] = {1, 2, 3, 4, 5};
  
  coil_u16_t data_index = coil_object_add_section(
    obj,
    data_name,
    COIL_SECTION_FLAG_WRITE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data_values,
    sizeof(data_values),
    arena
  );
  
  assert_true(data_index > 0);
  
  /* Initialize symbol table */
  err = coil_object_init_symbol_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add main symbol */
  coil_u16_t main_sym = coil_object_add_symbol(
    obj,
    main_name,
    0,                 /* Value (offset) */
    text_index,        /* Section index */
    COIL_SYMBOL_FUNC,  /* Type */
    COIL_SYMBOL_GLOBAL, /* Binding */
    arena
  );
  
  assert_true(main_sym > 0);
  
  /* Save the object to a file */
  err = coil_object_save_to_file(obj, TEST_FILE_PATH);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Cleanup code arena */
  arena_destroy(code_arena);
  
  /* Destroy the object */
  coil_object_destroy(obj, arena);
}

/* Test loading and inspecting a COIL object */
static void test_load_and_inspect(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* First, create a test file */
  test_create_full_program(state);
  
  /* Load the object from the file */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  coil_err_t err = coil_object_load_from_file(obj, TEST_FILE_PATH, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify the loaded object */
  const coil_object_header_t *header = coil_object_get_header(obj);
  assert_non_null(header);
  
  /* Should have string table, text section, data section, and symbol table */
  assert_int_equal(header->section_count, 4);
  
  /* Verify sections by name */
  coil_u16_t text_index = coil_object_get_section_index(obj, ".text");
  assert_true(text_index > 0);
  
  coil_u16_t data_index = coil_object_get_section_index(obj, ".data");
  assert_true(data_index > 0);
  
  /* Verify section contents */
  coil_section_header_t text_header;
  const void *text_data;
  coil_u64_t text_size;
  
  err = coil_object_get_section(obj, text_index, &text_header, &text_data, &text_size);
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_true(text_size > 0);
  assert_non_null(text_data);
  
  /* Verify data section */
  coil_section_header_t data_header;
  const void *data_data;
  coil_u64_t data_size;
  
  err = coil_object_get_section(obj, data_index, &data_header, &data_data, &data_size);
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_int_equal(data_size, 5 * sizeof(uint32_t));
  
  /* Verify the data values */
  const uint32_t *data_values = (const uint32_t *)data_data;
  assert_int_equal(data_values[0], 1);
  assert_int_equal(data_values[1], 2);
  assert_int_equal(data_values[2], 3);
  assert_int_equal(data_values[3], 4);
  assert_int_equal(data_values[4], 5);
  
  /* Verify the symbol */
  coil_u16_t main_index = coil_object_get_symbol_index(obj, "main");
  assert_true(main_index > 0);
  
  coil_symbol_t main_sym;
  err = coil_object_get_symbol(obj, main_index, &main_sym);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  assert_int_equal(main_sym.section_index, text_index);
  assert_int_equal(main_sym.type, COIL_SYMBOL_FUNC);
  assert_int_equal(main_sym.binding, COIL_SYMBOL_GLOBAL);
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
}

/* Test error handling integration */
static void test_error_handling(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Set custom error callback */
  coil_error_set_callback(test_error_callback, NULL);
  
  /* Create a test error condition - try to load a non-existent file */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  coil_err_t err = coil_object_load_from_file(obj, "nonexistent_file.coil", arena);
  assert_int_equal(err, COIL_ERR_IO);
  
  /* Verify callback was called */
  assert_true(error_callback_called);
  assert_int_equal(last_error_level, COIL_LEVEL_ERROR);
  
  /* Get the last error */
  const coil_error_context_t *ctx = coil_error_get_last();
  assert_non_null(ctx);
  assert_int_equal(ctx->code, COIL_ERR_IO);
  
  /* Test error system with COIL macros */
  error_callback_called = 0;
  COIL_WARNING(COIL_ERR_NOTFOUND, "Test warning message");
  
  assert_true(error_callback_called);
  assert_int_equal(last_error_level, COIL_LEVEL_WARNING);
  
  /* Clean up */
  coil_object_destroy(obj, arena);
}

/* Test with a complete workflow using all components */
static void test_complete_workflow(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Get library version information */
  coil_version_t version;
  coil_err_t err = coil_get_version(&version);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Create a new temporary arena for encoding instructions */
  coil_arena_t *instr_arena = arena_init(1024, 0);
  assert_non_null(instr_arena);
  
  /* Encode a small program */
  
  /* A function that computes factorial:
   * int factorial(int n) {
   *     if (n <= 1) return 1;
   *     return n * factorial(n-1);
   * }
   */
  
  /* Start encoding instructions */
  
  /* CMP r0, #1 (compare n to 1) */
  encode_instr(instr_arena, COIL_OP_CMP, 2);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  uint32_t one = 1;
  encode_operand_imm(instr_arena, COIL_VAL_I32, COIL_MOD_NONE, &one);
  
  /* JMP gt, else_branch */
  encode_instr(instr_arena, COIL_OP_BR, 2);
  uint32_t gt_flag = 1; /* Greater than flag */
  encode_operand_imm(instr_arena, COIL_VAL_FLAG0, COIL_MOD_NONE, &gt_flag);
  uint32_t else_branch_offset = 16; /* Offset to else branch */
  encode_operand_imm(instr_arena, COIL_VAL_U32, COIL_MOD_NONE, &else_branch_offset);
  
  /* MOV r0, #1 (return 1) */
  encode_instr(instr_arena, COIL_OP_MOV, 2);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_imm(instr_arena, COIL_VAL_I32, COIL_MOD_NONE, &one);
  
  /* RET */
  encode_instr_void(instr_arena, COIL_OP_RET);
  
  /* else_branch: */
  /* PUSH r0 (save n) */
  encode_instr(instr_arena, COIL_OP_PUSH, 1);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  
  /* SUB r0, r0, #1 (n-1) */
  encode_instr(instr_arena, COIL_OP_SUB, 3);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_imm(instr_arena, COIL_VAL_I32, COIL_MOD_NONE, &one);
  
  /* CALL factorial (recursive call) */
  encode_instr(instr_arena, COIL_OP_CALL, 1);
  uint32_t factorial_offset = 0; /* Offset to start of function */
  encode_operand_imm(instr_arena, COIL_VAL_U32, COIL_MOD_NONE, &factorial_offset);
  
  /* MOV r1, r0 (save factorial result) */
  encode_instr(instr_arena, COIL_OP_MOV, 2);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  
  /* POP r0 (restore n) */
  encode_instr(instr_arena, COIL_OP_POP, 1);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  
  /* MUL r0, r0, r1 (n * factorial(n-1)) */
  encode_instr(instr_arena, COIL_OP_MUL, 3);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 0);
  encode_operand_u32(instr_arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  
  /* RET */
  encode_instr_void(instr_arena, COIL_OP_RET);
  
  /* Create a COIL object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add section and symbol names */
  coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
  coil_u64_t factorial_name = coil_object_add_string(obj, "factorial", arena);
  
  /* Add code section */
  size_t code_size = arena_used(instr_arena);
  void *code_data = arena_alloc(instr_arena, 0, 1); /* Get pointer to the start of arena */
  
  coil_u16_t text_index = coil_object_add_section(
    obj,
    text_name,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    code_data,
    code_size,
    arena
  );
  
  assert_true(text_index > 0);
  
  /* Initialize symbol table */
  err = coil_object_init_symbol_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add factorial symbol */
  coil_u16_t factorial_sym = coil_object_add_symbol(
    obj,
    factorial_name,
    0,                 /* Value (offset) */
    text_index,        /* Section index */
    COIL_SYMBOL_FUNC,  /* Type */
    COIL_SYMBOL_GLOBAL, /* Binding */
    arena
  );
  
  assert_true(factorial_sym > 0);
  
  /* Save to file */
  err = coil_object_save_to_file(obj, TEST_FILE_PATH);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Load the file back to verify */
  coil_object_t *loaded_obj = coil_object_create(arena);
  assert_non_null(loaded_obj);
  
  err = coil_object_load_from_file(loaded_obj, TEST_FILE_PATH, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify the object */
  coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".text");
  assert_true(found_index > 0);
  
  coil_u16_t found_sym = coil_object_get_symbol_index(loaded_obj, "factorial");
  assert_true(found_sym > 0);
  
  /* Cleanup */
  arena_destroy(instr_arena);
  coil_object_destroy(obj, arena);
  coil_object_destroy(loaded_obj, arena);
}

/* Main function running all tests */
int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_create_full_program, setup, teardown),
    cmocka_unit_test_setup_teardown(test_load_and_inspect, setup, teardown),
    cmocka_unit_test_setup_teardown(test_error_handling, setup, teardown),
    cmocka_unit_test_setup_teardown(test_complete_workflow, setup, teardown),
  };
  
  return cmocka_run_group_tests(tests, NULL, NULL);
}