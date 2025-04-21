/**
 * @file test_obj.c
 * @brief Tests for the COIL object format
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <cmocka.h>

#include <coil/obj.h>
#include <coil/err.h>
#include <coil/arena.h>

/* Temporary file path for testing file I/O */
#define TEST_FILE_PATH "test_coil.obj"

/* Setup function for tests */
static int setup(void **state) {
  /* Initialize error system */
  coil_err_t err = coil_error_init();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Create an arena */
  coil_arena_t *arena = arena_init(4096, 0);
  if (!arena) {
    return -1;
  }
  
  *state = arena;
  return 0;
}

/* Teardown function for tests */
static int teardown(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  if (arena) {
    arena_destroy(arena);
  }
  
  /* Remove temporary file if it exists */
  remove(TEST_FILE_PATH);
  
  /* Shutdown error system */
  coil_error_shutdown();
  
  return 0;
}

/* Test object creation and destruction */
static void test_object_create_destroy(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create object with arena */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Verify header */
  const coil_object_header_t *header = coil_object_get_header(obj);
  assert_non_null(header);
  
  /* Check magic number */
  const uint8_t expected_magic[] = COIL_MAGIC_BYTES;
  assert_memory_equal(header->magic, expected_magic, 4);
  
  /* Check version */
  assert_int_equal(header->version, COIL_VERSION);
  
  /* Check section count */
  assert_int_equal(header->section_count, 0);
  
  /* Destroy object */
  coil_object_destroy(obj, arena);
  
  /* Create object with malloc */
  obj = coil_object_create(NULL);
  assert_non_null(obj);
  
  /* Verify header */
  header = coil_object_get_header(obj);
  assert_non_null(header);
  
  /* Destroy object */
  coil_object_destroy(obj, NULL);
}

/* Test string table operations */
static void test_string_table(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  coil_err_t err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify section count */
  assert_int_equal(coil_object_get_section_count(obj), 1);
  
  /* Add strings */
  coil_u64_t str1_offset = coil_object_add_string(obj, "test_string_1", arena);
  assert_true(str1_offset > 0);
  
  coil_u64_t str2_offset = coil_object_add_string(obj, "test_string_2", arena);
  assert_true(str2_offset > 0);
  
  /* Verify strings are different */
  assert_int_not_equal(str1_offset, str2_offset);
  
  /* Adding the same string should return the same offset */
  coil_u64_t str1_dup_offset = coil_object_add_string(obj, "test_string_1", arena);
  assert_int_equal(str1_offset, str1_dup_offset);
  
  /* Retrieve strings */
  char buffer[256];
  
  err = coil_object_get_string(obj, str1_offset, buffer, sizeof(buffer));
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_string_equal(buffer, "test_string_1");
  
  err = coil_object_get_string(obj, str2_offset, buffer, sizeof(buffer));
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_string_equal(buffer, "test_string_2");
  
  /* Test invalid offset */
  err = coil_object_get_string(obj, 9999, buffer, sizeof(buffer));
  assert_int_equal(err, COIL_ERR_INVAL);
  
  /* Test small buffer (truncation) */
  char small_buffer[5];
  err = coil_object_get_string(obj, str1_offset, small_buffer, sizeof(small_buffer));
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_string_equal(small_buffer, "test");
  
  /* Destroy object */
  coil_object_destroy(obj, arena);
}

/* Test section operations */
static void test_sections(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  coil_err_t err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add section names */
  coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
  coil_u64_t data_name = coil_object_add_string(obj, ".data", arena);
  
  /* Create test data */
  uint8_t text_data[] = {0x01, 0x02, 0x03, 0x04};
  uint8_t data_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  
  /* Add sections */
  coil_u16_t text_index = coil_object_add_section(
    obj,
    text_name,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    text_data,
    sizeof(text_data),
    arena
  );
  
  coil_u16_t data_index = coil_object_add_section(
    obj,
    data_name,
    COIL_SECTION_FLAG_WRITE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data_data,
    sizeof(data_data),
    arena
  );
  
  /* Verify section indices */
  assert_int_not_equal(text_index, 0);
  assert_int_not_equal(data_index, 0);
  assert_int_not_equal(text_index, data_index);
  
  /* Verify section count */
  assert_int_equal(coil_object_get_section_count(obj), 3); /* 1 string table + 2 sections */
  
  /* Get section by index */
  coil_section_header_t header;
  const void *section_data;
  coil_u64_t section_size;
  
  err = coil_object_get_section(obj, text_index, &header, &section_data, &section_size);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify section properties */
  assert_int_equal(header.name, text_name);
  assert_int_equal(header.flags, COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC);
  assert_int_equal(header.type, COIL_SECTION_PROGBITS);
  assert_int_equal(header.size, sizeof(text_data));
  assert_memory_equal(section_data, text_data, sizeof(text_data));
  assert_int_equal(section_size, sizeof(text_data));
  
  /* Get section by name */
  coil_u16_t found_index = coil_object_get_section_index(obj, ".text");
  assert_int_equal(found_index, text_index);
  
  found_index = coil_object_get_section_index(obj, ".data");
  assert_int_equal(found_index, data_index);
  
  /* Test getting nonexistent section */
  found_index = coil_object_get_section_index(obj, ".nonexistent");
  assert_int_equal(found_index, 0);
  
  /* Destroy object */
  coil_object_destroy(obj, arena);
}

/* Test symbol table operations */
static void test_symbol_table(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  coil_err_t err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Initialize symbol table */
  err = coil_object_init_symbol_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add section name and section */
  coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
  uint8_t text_data[] = {0x01, 0x02, 0x03, 0x04};
  
  coil_u16_t text_index = coil_object_add_section(
    obj,
    text_name,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    text_data,
    sizeof(text_data),
    arena
  );
  
  /* Add symbol names */
  coil_u64_t func_name = coil_object_add_string(obj, "test_function", arena);
  coil_u64_t var_name = coil_object_add_string(obj, "test_variable", arena);
  
  /* Add symbols */
  coil_u16_t func_sym = coil_object_add_symbol(
    obj,
    func_name,
    0,                /* Value */
    text_index,       /* Section index */
    COIL_SYMBOL_FUNC, /* Type */
    COIL_SYMBOL_GLOBAL, /* Binding */
    arena
  );
  
  coil_u16_t var_sym = coil_object_add_symbol(
    obj,
    var_name,
    4,                 /* Value */
    text_index,        /* Section index */
    COIL_SYMBOL_OBJECT, /* Type */
    COIL_SYMBOL_LOCAL,  /* Binding */
    arena
  );
  
  /* Verify symbol indices */
  assert_int_not_equal(func_sym, 0);
  assert_int_not_equal(var_sym, 0);
  
  /* Get symbols by index */
  coil_symbol_t symbol;
  
  err = coil_object_get_symbol(obj, func_sym, &symbol);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify symbol properties */
  assert_int_equal(symbol.name, func_name);
  assert_int_equal(symbol.value, 0);
  assert_int_equal(symbol.section_index, text_index);
  assert_int_equal(symbol.type, COIL_SYMBOL_FUNC);
  assert_int_equal(symbol.binding, COIL_SYMBOL_GLOBAL);
  
  /* Get symbol by name */
  coil_u16_t found_sym = coil_object_get_symbol_index(obj, "test_function");
  assert_int_equal(found_sym, func_sym);
  
  found_sym = coil_object_get_symbol_index(obj, "test_variable");
  assert_int_equal(found_sym, var_sym);
  
  /* Test getting nonexistent symbol */
  found_sym = coil_object_get_symbol_index(obj, "nonexistent_symbol");
  assert_int_equal(found_sym, 0);
  
  /* Destroy object */
  coil_object_destroy(obj, arena);
}

/* Test serialization to memory */
static void test_save_load_memory(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create and set up a test object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Add a string table and section */
  coil_object_init_string_table(obj, arena);
  coil_u64_t name = coil_object_add_string(obj, ".test", arena);
  
  /* Add test data */
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  coil_object_add_section(
    obj,
    name,
    COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data,
    sizeof(data),
    arena
  );
  
  /* Save to memory */
  void *buffer;
  size_t size;
  coil_err_t err = coil_object_save_to_memory(obj, arena, &buffer, &size);
  assert_int_equal(err, COIL_ERR_GOOD);
  assert_non_null(buffer);
  assert_true(size > 0);
  
  /* Create a new object to load into */
  coil_object_t *loaded_obj = coil_object_create(arena);
  assert_non_null(loaded_obj);
  
  /* Load from memory */
  err = coil_object_load_from_memory(loaded_obj, buffer, size, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify loaded object */
  const coil_object_header_t *header = coil_object_get_header(loaded_obj);
  assert_int_equal(header->section_count, 2); /* String table + test section */
  
  /* Get the section */
  coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".test");
  assert_true(found_index > 0);
  
  coil_section_header_t section_header;
  const void *section_data;
  coil_u64_t section_size;
  
  err = coil_object_get_section(loaded_obj, found_index, &section_header, &section_data, &section_size);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify section data */
  assert_int_equal(section_size, sizeof(data));
  assert_memory_equal(section_data, data, sizeof(data));
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
  coil_object_destroy(loaded_obj, arena);
}

/* Test serialization to file */
static void test_save_load_file(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create and set up a test object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Add a string table and section */
  coil_object_init_string_table(obj, arena);
  coil_u64_t name = coil_object_add_string(obj, ".test", arena);
  
  /* Add test data */
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  coil_object_add_section(
    obj,
    name,
    COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    data,
    sizeof(data),
    arena
  );
  
  /* Save to file */
  coil_err_t err = coil_object_save_to_file(obj, TEST_FILE_PATH);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify file exists */
  FILE *file = fopen(TEST_FILE_PATH, "rb");
  assert_non_null(file);
  fclose(file);
  
  /* Create a new object to load into */
  coil_object_t *loaded_obj = coil_object_create(arena);
  assert_non_null(loaded_obj);
  
  /* Load from file */
  err = coil_object_load_from_file(loaded_obj, TEST_FILE_PATH, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify loaded object */
  const coil_object_header_t *header = coil_object_get_header(loaded_obj);
  assert_int_equal(header->section_count, 2); /* String table + test section */
  
  /* Get the section */
  coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".test");
  assert_true(found_index > 0);
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
  coil_object_destroy(loaded_obj, arena);
}

/* Test edge cases */
static void test_edge_cases(void **state) {
  coil_arena_t *arena = (coil_arena_t *)*state;
  
  /* Create object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Test null object operations */
  assert_null(coil_object_get_header(NULL));
  assert_int_equal(coil_object_get_section_count(NULL), 0);
  assert_int_equal(coil_object_init_string_table(NULL, arena), COIL_ERR_INVAL);
  assert_int_equal(coil_object_add_string(NULL, "test", arena), 0);
  
  /* Test operations on object without string table */
  assert_int_equal(coil_object_add_string(obj, "test", NULL), 0);
  assert_int_equal(coil_object_get_string(obj, 0, NULL, 0), COIL_ERR_INVAL);
  
  /* Initialize string table */
  coil_object_init_string_table(obj, arena);
  
  /* Test invalid string operations */
  assert_int_equal(coil_object_add_string(obj, NULL, arena), 0);
  
  char buffer[256];
  assert_int_equal(coil_object_get_string(obj, 999, buffer, sizeof(buffer)), COIL_ERR_INVAL);
  assert_int_equal(coil_object_get_string(obj, 0, NULL, sizeof(buffer)), COIL_ERR_INVAL);
  assert_int_equal(coil_object_get_string(obj, 0, buffer, 0), COIL_ERR_INVAL);
  
  /* Test invalid section operations */
  assert_int_equal(coil_object_add_section(obj, 0, 0, 0, NULL, 0, NULL), 0);
  assert_int_equal(coil_object_get_section(obj, 0, NULL, NULL, NULL), COIL_ERR_INVAL);
  assert_int_equal(coil_object_get_section(obj, 999, &(coil_section_header_t){0}, NULL, NULL), COIL_ERR_INVAL);
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
}

/* Main function running all tests */
int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_object_create_destroy, setup, teardown),
    cmocka_unit_test_setup_teardown(test_string_table, setup, teardown),
    cmocka_unit_test_setup_teardown(test_sections, setup, teardown),
    cmocka_unit_test_setup_teardown(test_symbol_table, setup, teardown),
    cmocka_unit_test_setup_teardown(test_save_load_memory, setup, teardown),
    cmocka_unit_test_setup_teardown(test_save_load_file, setup, teardown),
    cmocka_unit_test_setup_teardown(test_edge_cases, setup, teardown),
  };
  
  return cmocka_run_group_tests(tests, NULL, NULL);
}