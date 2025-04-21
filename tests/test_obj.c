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

// For combined test mode
#ifndef RUN_INDIVIDUAL
extern int test_verbosity;
#endif

/* Temporary file path for testing file I/O */
#define TEST_FILE_PATH "test_coil.obj"

/**
 * @brief Print object and section information for debugging
 */
static void debug_print_object_info(const coil_object_t* obj) {
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (!verbosity) return;
	
	const coil_object_header_t* header = coil_object_get_header(obj);
	printf("Object header:\n");
	printf("  ├─ Magic: %c%c%c%c\n", 
				 header->magic[0], header->magic[1], header->magic[2], header->magic[3]);
	printf("  ├─ Version: 0x%04X\n", header->version);
	printf("  ├─ Section count: %d\n", header->section_count);
	printf("  └─ File size: %llu bytes\n", (unsigned long long)header->file_size);
	
	/* Print section info if we have a string table */
	if (header->section_count > 0) {
		printf("\nSections:\n");
		
		for (coil_u16_t i = 1; i <= header->section_count; i++) {
			coil_section_header_t section_header;
			const void* section_data;
			coil_u64_t section_size;
			
			if (coil_object_get_section(obj, i, &section_header, &section_data, &section_size) 
					== COIL_ERR_GOOD) {
				
				char name_buffer[64] = "<unknown>";
				coil_object_get_string(obj, section_header.name, name_buffer, sizeof(name_buffer));
				
				printf("  Section %d: %s\n", i, name_buffer);
				printf("    ├─ Size: %llu bytes\n", (unsigned long long)section_header.size);
				printf("    ├─ Type: %d\n", section_header.type);
				printf("    └─ Flags: 0x%04X\n", section_header.flags);
			}
		}
	}
}

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
	if (header->version != COIL_VERSION) {
		fail_msg("Version mismatch: expected 0x%04X, got 0x%04X", COIL_VERSION, header->version);
	}
	
	/* Check section count */
	if (header->section_count != 0) {
		fail_msg("Expected 0 sections for new object, got %d", header->section_count);
	}
	
	/* Print debug info */
	debug_print_object_info(obj);
	
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
	coil_u16_t section_count = coil_object_get_section_count(obj);
	if (section_count != 1) {
		fail_msg("Expected 1 section after initializing string table, got %d", section_count);
	}
	
	/* Print debug info */
	debug_print_object_info(obj);
	
	/* Add strings */
	coil_u64_t str1_offset = coil_object_add_string(obj, "test_string_1", arena);
	if (str1_offset == 0) {
		fail_msg("Failed to add first string to string table");
	}
	
	coil_u64_t str2_offset = coil_object_add_string(obj, "test_string_2", arena);
	if (str2_offset == 0) {
		fail_msg("Failed to add second string to string table");
	}
	
	/* Verify strings are different */
	if (str1_offset == str2_offset) {
		fail_msg("Different strings unexpectedly have the same offset: %llu", 
				(unsigned long long)str1_offset);
	}
	
	/* Adding the same string should return the same offset */
	coil_u64_t str1_dup_offset = coil_object_add_string(obj, "test_string_1", arena);
	if (str1_offset != str1_dup_offset) {
		fail_msg("Duplicate string got different offset: %llu vs %llu", 
				(unsigned long long)str1_offset, (unsigned long long)str1_dup_offset);
	}
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nString table test:\n");
		printf("  ├─ String 1 offset: %llu\n", (unsigned long long)str1_offset);
		printf("  ├─ String 2 offset: %llu\n", (unsigned long long)str2_offset);
		printf("  └─ Duplicate string offset: %llu\n", (unsigned long long)str1_dup_offset);
	}
	
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
	
	if (text_index == 0) {
		fail_msg("Failed to add .text section");
	}
	
	coil_u16_t data_index = coil_object_add_section(
		obj,
		data_name,
		COIL_SECTION_FLAG_WRITE | COIL_SECTION_FLAG_ALLOC,
		COIL_SECTION_PROGBITS,
		data_data,
		sizeof(data_data),
		arena
	);
	
	if (data_index == 0) {
		fail_msg("Failed to add .data section");
	}
	
	/* Verify section indices */
	if (text_index == data_index) {
		fail_msg("Sections have the same index: %d", text_index);
	}
	
	/* Print debug info */
	debug_print_object_info(obj);
	
	/* Verify section count */
	coil_u16_t section_count = coil_object_get_section_count(obj);
	if (section_count != 3) { // 1 string table + 2 sections
		fail_msg("Expected 3 sections, got %d", section_count);
	}
	
	/* Get section by index */
	coil_section_header_t header;
	const void *section_data;
	coil_u64_t section_size;
	
	err = coil_object_get_section(obj, text_index, &header, &section_data, &section_size);
	assert_int_equal(err, COIL_ERR_GOOD);
	
	/* Verify section properties */
	if (header.name != text_name) {
		fail_msg("Section name offset mismatch: expected %llu, got %llu", 
				(unsigned long long)text_name, (unsigned long long)header.name);
	}
	
	if (header.flags != (COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC)) {
		fail_msg("Section flags mismatch: expected 0x%04X, got 0x%04X", 
				COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC, header.flags);
	}
	
	if (header.type != COIL_SECTION_PROGBITS) {
		fail_msg("Section type mismatch: expected %d, got %d", 
				COIL_SECTION_PROGBITS, header.type);
	}
	
	if (header.size != sizeof(text_data)) {
		fail_msg("Section size mismatch: expected %zu, got %llu", 
				sizeof(text_data), (unsigned long long)header.size);
	}
	
	assert_memory_equal(section_data, text_data, sizeof(text_data));
	assert_int_equal(section_size, sizeof(text_data));
	
	/* Get section by name */
	coil_u16_t found_index = coil_object_get_section_index(obj, ".text");
	if (found_index != text_index) {
		fail_msg("Lookup by name failed: expected index %d, got %d", 
				text_index, found_index);
	}
	
	found_index = coil_object_get_section_index(obj, ".data");
	if (found_index != data_index) {
		fail_msg("Lookup by name failed: expected index %d, got %d", 
				data_index, found_index);
	}
	
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
	
	if (func_sym == 0) {
		fail_msg("Failed to add function symbol");
	}
	
	coil_u16_t var_sym = coil_object_add_symbol(
		obj,
		var_name,
		4,                 /* Value */
		text_index,        /* Section index */
		COIL_SYMBOL_OBJECT, /* Type */
		COIL_SYMBOL_LOCAL,  /* Binding */
		arena
	);
	
	if (var_sym == 0) {
		fail_msg("Failed to add variable symbol");
	}
	
	/* Verify symbol indices */
	if (func_sym == var_sym) {
		fail_msg("Symbols have the same index: %d", func_sym);
	}
	
	/* Print debug info */
	debug_print_object_info(obj);
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nSymbol details:\n");
		
		coil_symbol_t symbol;
		coil_object_get_symbol(obj, func_sym, &symbol);
		printf("  Function symbol:\n");
		printf("    ├─ Index: %d\n", func_sym);
		printf("    ├─ Value: %u\n", symbol.value);
		printf("    ├─ Section: %d\n", symbol.section_index);
		printf("    ├─ Type: %d\n", symbol.type);
		printf("    └─ Binding: %d\n", symbol.binding);
		
		coil_object_get_symbol(obj, var_sym, &symbol);
		printf("  Variable symbol:\n");
		printf("    ├─ Index: %d\n", var_sym);
		printf("    ├─ Value: %u\n", symbol.value);
		printf("    ├─ Section: %d\n", symbol.section_index);
		printf("    ├─ Type: %d\n", symbol.type);
		printf("    └─ Binding: %d\n", symbol.binding);
	}
	
	/* Get symbols by index */
	coil_symbol_t symbol;
	
	err = coil_object_get_symbol(obj, func_sym, &symbol);
	assert_int_equal(err, COIL_ERR_GOOD);
	
	/* Verify symbol properties */
	if (symbol.name != func_name) {
		fail_msg("Symbol name offset mismatch: expected %llu, got %llu", 
				(unsigned long long)func_name, (unsigned long long)symbol.name);
	}
	
	if (symbol.value != 0) {
		fail_msg("Symbol value mismatch: expected 0, got %u", symbol.value);
	}
	
	if (symbol.section_index != text_index) {
		fail_msg("Symbol section index mismatch: expected %d, got %d", 
				text_index, symbol.section_index);
	}
	
	if (symbol.type != COIL_SYMBOL_FUNC) {
		fail_msg("Symbol type mismatch: expected %d, got %d", 
				COIL_SYMBOL_FUNC, symbol.type);
	}
	
	if (symbol.binding != COIL_SYMBOL_GLOBAL) {
		fail_msg("Symbol binding mismatch: expected %d, got %d", 
				COIL_SYMBOL_GLOBAL, symbol.binding);
	}
	
	/* Get symbol by name */
	coil_u16_t found_sym = coil_object_get_symbol_index(obj, "test_function");
	if (found_sym != func_sym) {
		fail_msg("Symbol lookup by name failed: expected index %d, got %d", 
				func_sym, found_sym);
	}
	
	found_sym = coil_object_get_symbol_index(obj, "test_variable");
	if (found_sym != var_sym) {
		fail_msg("Symbol lookup by name failed: expected index %d, got %d", 
				var_sym, found_sym);
	}
	
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
	
	/* Print original object debug info */
	printf("\nOriginal object:\n");
	debug_print_object_info(obj);
	
	/* Save to memory */
	void *buffer;
	size_t size;
	coil_err_t err = coil_object_save_to_memory(obj, arena, &buffer, &size);
	assert_int_equal(err, COIL_ERR_GOOD);
	if (!buffer) {
		fail_msg("Save to memory returned NULL buffer");
	}
	if (size == 0) {
		fail_msg("Save to memory returned zero size");
	}
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nSerialized object size: %zu bytes\n", size);
	}
	
	/* Create a new object to load into */
	coil_object_t *loaded_obj = coil_object_create(arena);
	assert_non_null(loaded_obj);
	
	/* Load from memory */
	err = coil_object_load_from_memory(loaded_obj, buffer, size, arena);
	assert_int_equal(err, COIL_ERR_GOOD);
	
	/* Print loaded object debug info */
	printf("\nLoaded object:\n");
	debug_print_object_info(loaded_obj);
	
	/* Verify loaded object */
	const coil_object_header_t *header = coil_object_get_header(loaded_obj);
	coil_u16_t section_count = header->section_count;
	if (section_count != 2) { // String table + test section
		fail_msg("Expected 2 sections in loaded object, got %d", section_count);
	}
	
	/* Get the section */
	coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".test");
	if (found_index == 0) {
		fail_msg("Failed to find .test section in loaded object");
	}
	
	coil_section_header_t section_header;
	const void *section_data;
	coil_u64_t section_size;
	
	err = coil_object_get_section(loaded_obj, found_index, &section_header, &section_data, &section_size);
	assert_int_equal(err, COIL_ERR_GOOD);
	
	/* Verify section data */
	if (section_size != sizeof(data)) {
		fail_msg("Section size mismatch: expected %zu, got %llu", 
				sizeof(data), (unsigned long long)section_size);
	}
	
	if (memcmp(section_data, data, sizeof(data)) != 0) {
		fail_msg("Section data mismatch");
		printf("Expected data: ");
		for (size_t i = 0; i < sizeof(data); i++) {
			printf("%02X ", data[i]);
		}
		printf("\nActual data:   ");
		for (size_t i = 0; i < section_size; i++) {
			printf("%02X ", ((uint8_t*)section_data)[i]);
		}
		printf("\n");
	}
	
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
	if (!file) {
		fail_msg("Failed to open test file after saving");
	}
	
	/* Get file size */
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fclose(file);
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nSaved file size: %ld bytes\n", file_size);
	}
	
	/* Create a new object to load into */
	coil_object_t *loaded_obj = coil_object_create(arena);
	assert_non_null(loaded_obj);
	
	/* Load from file */
	err = coil_object_load_from_file(loaded_obj, TEST_FILE_PATH, arena);
	assert_int_equal(err, COIL_ERR_GOOD);
	
	/* Print loaded object debug info */
	printf("\nObject loaded from file:\n");
	debug_print_object_info(loaded_obj);
	
	/* Verify loaded object */
	const coil_object_header_t *header = coil_object_get_header(loaded_obj);
	coil_u16_t section_count = header->section_count;
	if (section_count != 2) { // String table + test section
		fail_msg("Expected 2 sections in loaded object, got %d", section_count);
	}
	
	/* Get the section */
	coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".test");
	if (found_index == 0) {
		fail_msg("Failed to find .test section in loaded object");
	}
	
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
	if (coil_object_get_header(NULL) != NULL) {
		fail_msg("coil_object_get_header(NULL) should return NULL");
	}
	
	if (coil_object_get_section_count(NULL) != 0) {
		fail_msg("coil_object_get_section_count(NULL) should return 0");
	}
	
	if (coil_object_init_string_table(NULL, arena) != COIL_ERR_INVAL) {
		fail_msg("coil_object_init_string_table(NULL, arena) should return COIL_ERR_INVAL");
	}
	
	if (coil_object_add_string(NULL, "test", arena) != 0) {
		fail_msg("coil_object_add_string(NULL, ...) should return 0");
	}
	
	/* Test operations on object without string table */
	if (coil_object_add_string(obj, "test", NULL) != 0) {
		fail_msg("coil_object_add_string(obj, str, NULL) should return 0");
	}
	
	char buffer[256];
	if (coil_object_get_string(obj, 0, NULL, 0) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_string with NULL buffer should return COIL_ERR_INVAL");
	}
	
	/* Initialize string table */
	coil_object_init_string_table(obj, arena);
	
	/* Test invalid string operations */
	if (coil_object_add_string(obj, NULL, arena) != 0) {
		fail_msg("coil_object_add_string(obj, NULL, arena) should return 0");
	}
	
	if (coil_object_get_string(obj, 999, buffer, sizeof(buffer)) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_string with invalid offset should return COIL_ERR_INVAL");
	}
	
	if (coil_object_get_string(obj, 0, NULL, sizeof(buffer)) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_string with NULL buffer should return COIL_ERR_INVAL");
	}
	
	if (coil_object_get_string(obj, 0, buffer, 0) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_string with zero buffer size should return COIL_ERR_INVAL");
	}
	
	/* Test invalid section operations */
	if (coil_object_add_section(obj, 0, 0, 0, NULL, 0, NULL) != 0) {
		fail_msg("coil_object_add_section with invalid params should return 0");
	}
	
	if (coil_object_get_section(obj, 0, NULL, NULL, NULL) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_section with NULL header should return COIL_ERR_INVAL");
	}
	
	coil_section_header_t header = {0};
	if (coil_object_get_section(obj, 999, &header, NULL, NULL) != COIL_ERR_INVAL) {
		fail_msg("coil_object_get_section with invalid index should return COIL_ERR_INVAL");
	}
	
	/* Cleanup */
	coil_object_destroy(obj, arena);
}

/* Get object tests for combined testing */
struct CMUnitTest *get_obj_tests(int *count) {
	static struct CMUnitTest obj_tests[] = {
		cmocka_unit_test_setup_teardown(test_object_create_destroy, setup, teardown),
		cmocka_unit_test_setup_teardown(test_string_table, setup, teardown),
		cmocka_unit_test_setup_teardown(test_sections, setup, teardown),
		cmocka_unit_test_setup_teardown(test_symbol_table, setup, teardown),
		cmocka_unit_test_setup_teardown(test_save_load_memory, setup, teardown),
		cmocka_unit_test_setup_teardown(test_save_load_file, setup, teardown),
		cmocka_unit_test_setup_teardown(test_edge_cases, setup, teardown),
	};
	
	*count = sizeof(obj_tests) / sizeof(obj_tests[0]);
	printf("[get_obj_tests] Returning %d tests\n", *count);
	return obj_tests;
}

/* Individual test main function */
#ifdef RUN_INDIVIDUAL
int main(void) {
	printf("Running object format tests individually\n");
	
	int count;
	struct CMUnitTest *tests = get_obj_tests(&count);
	printf("Running %d tests\n", count);
	
	return _cmocka_run_group_tests("Object Format Tests", tests, count, NULL, NULL);
}
#endif