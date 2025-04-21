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

// For combined test mode
#ifndef RUN_INDIVIDUAL
extern int test_verbosity;
#endif

#define TEST_FILE_PATH "test_integration.coil"

/* Custom error callback for testing */
static int error_callback_called = 0;
static coil_error_level_t last_error_level = 0;
static char last_error_message[256] = {0};

static void test_error_callback(
	coil_error_level_t level,
	const char* message,
	const coil_error_position_t* position,
	void* user_data
) {
	(void)user_data;
	
	error_callback_called = 1;
	last_error_level = level;
	
	if (message) {
		strncpy(last_error_message, message, sizeof(last_error_message) - 1);
		last_error_message[sizeof(last_error_message) - 1] = '\0';
	} else {
		last_error_message[0] = '\0';
	}
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("Error callback triggered:\n");
		printf("  ├─ Level: %d\n", level);
		printf("  ├─ Message: %s\n", message ? message : "(null)");
		if (position && position->file) {
			printf("  └─ Position: %s:%zu\n", position->file, position->line);
		} else {
			printf("  └─ Position: (null)\n");
		}
	}
}

/**
 * @brief Print object debug information
 */
static void debug_print_obj_info(const coil_object_t *obj, const char *title) {
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (!verbosity) return;
	
	const coil_object_header_t *header = coil_object_get_header(obj);
	if (!header) {
		printf("\n%s: <NULL HEADER>\n", title);
		return;
	}
	
	printf("\n%s:\n", title);
	printf("  ├─ Section count: %d\n", header->section_count);
	printf("  └─ File size: %llu bytes\n", (unsigned long long)header->file_size);
	
	// Print section names if we have them
	if (header->section_count > 0) {
		printf("  Sections:\n");
		
		for (coil_u16_t i = 1; i <= header->section_count; i++) {
			char name_buffer[64] = "<unnamed>";
			coil_section_header_t section;
			const void *data;
			coil_u64_t size;
			
			if (coil_object_get_section(obj, i, &section, &data, &size) == COIL_ERR_GOOD) {
				coil_object_get_string(obj, section.name, name_buffer, sizeof(name_buffer));
				printf("    ├─ %s (%llu bytes)\n", name_buffer, (unsigned long long)size);
				
				// For small sections, print the first few bytes for debug purposes
				if (size > 0 && size <= 16) {
					printf("      └─ Data: ");
					for (size_t j = 0; j < size; j++) {
						printf("%02X ", ((const uint8_t*)data)[j]);
					}
					printf("\n");
				}
			}
		}
	}
}

/* Setup function for tests */
static int setup(void **state) {
	/* Initialize the library */
	coil_err_t err = coil_initialize();
	if (err != COIL_ERR_GOOD) {
		printf("Failed to initialize COIL library: %s\n", coil_error_string(err));
		return -1;
	}
	
	/* Create an arena for testing */
	coil_arena_t *arena = arena_init(4096, 0);
	if (!arena) {
		printf("Failed to create arena\n");
		coil_shutdown();
		return -1;
	}
	
	/* Reset callback tracking */
	error_callback_called = 0;
	last_error_level = 0;
	last_error_message[0] = '\0';
	
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
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nCreating a complete program:\n");
	}
	
	/* Create an object */
	coil_object_t *obj = coil_object_create(arena);
	assert_non_null(obj);
	
	/* Initialize string table */
	coil_err_t err = coil_object_init_string_table(obj, arena);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to initialize string table: %s", coil_error_string(err));
	}
	
	/* Create a code section */
	coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
	if (text_name == 0) {
		fail_msg("Failed to add .text string to string table");
	}
	
	/* Create a data section */
	coil_u64_t data_name = coil_object_add_string(obj, ".data", arena);
	if (data_name == 0) {
		fail_msg("Failed to add .data string to string table");
	}
	
	/* Add section names to string table */
	coil_u64_t main_name = coil_object_add_string(obj, "main", arena);
	if (main_name == 0) {
		fail_msg("Failed to add main symbol name to string table");
	}
	
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
	
	if (verbosity) {
		printf("  Generating program code:\n");
		printf("    int main() {\n");
		printf("        int a = 42;\n");
		printf("        int b = 13;\n");
		printf("        return a + b;\n");
		printf("    }\n");
	}
	
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
	if (code_size == 0) {
		fail_msg("No code was generated");
	}
	
	void *code_data = code_arena->first_block->memory;
	if (!code_data) {
		fail_msg("Code data pointer is NULL");
	}
	
	if (verbosity) {
		printf("  Generated code size: %zu bytes\n", code_size);
		printf("  Code data pointer: %p\n", code_data);
	}
	
	coil_u16_t text_index = coil_object_add_section(
		obj,
		text_name,
		COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
		COIL_SECTION_PROGBITS,
		code_data,
		code_size,
		arena
	);
	
	if (text_index == 0) {
		fail_msg("Failed to add .text section");
	}
	
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
	
	if (data_index == 0) {
		fail_msg("Failed to add .data section");
	}
	
	/* Initialize symbol table */
	err = coil_object_init_symbol_table(obj, arena);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to initialize symbol table: %s", coil_error_string(err));
	}
	
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
	
	if (main_sym == 0) {
		fail_msg("Failed to add main symbol");
	}
	
	/* Print object info */
	debug_print_obj_info(obj, "Program object");
	
	/* Save the object to a file */
	err = coil_object_save_to_file(obj, TEST_FILE_PATH);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to save object to file: %s", coil_error_string(err));
	}
	
	if (verbosity) {
		printf("  Saved program to: %s\n", TEST_FILE_PATH);
		
		/* Get file size */
		FILE *f = fopen(TEST_FILE_PATH, "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			long file_size = ftell(f);
			fclose(f);
			printf("  File size: %ld bytes\n", file_size);
			
			// Verify file is not empty
			if (file_size <= 0) {
				fail_msg("Saved file is empty or invalid");
			}
		} else {
			fail_msg("Could not open file for verification");
		}
	}
	
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
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nLoading and inspecting program:\n");
	}
	
	/* Load the object from the file */
	coil_object_t *obj = coil_object_create(arena);
	assert_non_null(obj);
	
	coil_err_t err = coil_object_load_from_file(obj, TEST_FILE_PATH, arena);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to load object from file: %s", coil_error_string(err));
	}
	
	/* Verify the loaded object */
	const coil_object_header_t *header = coil_object_get_header(obj);
	assert_non_null(header);
	
	/* Should have string table, text section, data section, and symbol table */
	if (header->section_count != 4) {
		fail_msg("Expected 4 sections in loaded object, got %d", header->section_count);
	}
	
	/* Print loaded object info */
	debug_print_obj_info(obj, "Loaded program object");
	
	/* Verify sections by name */
	coil_u16_t text_index = coil_object_get_section_index(obj, ".text");
	if (text_index == 0) {
		fail_msg("Failed to find .text section in loaded object");
	}
	
	coil_u16_t data_index = coil_object_get_section_index(obj, ".data");
	if (data_index == 0) {
		fail_msg("Failed to find .data section in loaded object");
	}
	
	/* Verify section contents */
	coil_section_header_t text_header;
	const void *text_data;
	coil_u64_t text_size;
	
	err = coil_object_get_section(obj, text_index, &text_header, &text_data, &text_size);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to get .text section: %s", coil_error_string(err));
	}
	if (text_size == 0 || !text_data) {
		fail_msg("Text section is empty or has NULL data pointer");
	}
	
	if (verbosity) {
		printf("  .text section size: %llu bytes\n", (unsigned long long)text_size);
	}
	
	/* Verify data section */
	coil_section_header_t data_header;
	const void *data_data;
	coil_u64_t data_size;
	
	err = coil_object_get_section(obj, data_index, &data_header, &data_data, &data_size);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to get .data section: %s", coil_error_string(err));
	}
	
	if (data_size != 5 * sizeof(uint32_t)) {
		fail_msg("Data section size mismatch: expected %zu, got %llu", 
				5 * sizeof(uint32_t), (unsigned long long)data_size);
	}
	
	if (verbosity) {
		printf("  .data section size: %llu bytes\n", (unsigned long long)data_size);
		printf("  .data values: ");
		
		const uint32_t *values = (const uint32_t *)data_data;
		for (size_t i = 0; i < data_size / sizeof(uint32_t); i++) {
			printf("%d ", values[i]);
		}
		printf("\n");
	}
	
	/* Verify the data values */
	const uint32_t *data_values = (const uint32_t *)data_data;
	const uint32_t expected_values[] = {1, 2, 3, 4, 5};
	for (size_t i = 0; i < 5; i++) {
		if (data_values[i] != expected_values[i]) {
			fail_msg("Data value at index %zu mismatch: expected %u, got %u", 
					i, expected_values[i], data_values[i]);
		}
	}
	
	/* Verify the symbol */
	coil_u16_t main_index = coil_object_get_symbol_index(obj, "main");
	if (main_index == 0) {
		fail_msg("Failed to find 'main' symbol in loaded object");
	}
	
	coil_symbol_t main_sym;
	err = coil_object_get_symbol(obj, main_index, &main_sym);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to get main symbol: %s", coil_error_string(err));
	}
	
	if (main_sym.section_index != text_index) {
		fail_msg("Main symbol section index mismatch: expected %d, got %d", 
				text_index, main_sym.section_index);
	}
	
	if (main_sym.type != COIL_SYMBOL_FUNC) {
		fail_msg("Main symbol type mismatch: expected %d, got %d", 
				COIL_SYMBOL_FUNC, main_sym.type);
	}
	
	if (main_sym.binding != COIL_SYMBOL_GLOBAL) {
		fail_msg("Main symbol binding mismatch: expected %d, got %d", 
				COIL_SYMBOL_GLOBAL, main_sym.binding);
	}
	
	if (verbosity) {
		printf("  Found 'main' symbol at index %d\n", main_index);
		printf("    ├─ Section: %d\n", main_sym.section_index);
		printf("    ├─ Type: %d\n", main_sym.type);
		printf("    └─ Binding: %d\n", main_sym.binding);
	}
	
	/* Cleanup */
	coil_object_destroy(obj, arena);
}

/* Test error handling integration */
static void test_error_handling(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nTesting error handling:\n");
	}
	
	/* Set custom error callback */
	coil_error_set_callback(test_error_callback, NULL);
	
	/* Create a test error condition - try to load a non-existent file */
	coil_object_t *obj = coil_object_create(arena);
	assert_non_null(obj);
	
	if (verbosity) {
		printf("  Attempting to load non-existent file...\n");
	}
	
	coil_err_t err = coil_object_load_from_file(obj, "nonexistent_file.coil", arena);
	if (err != COIL_ERR_IO) {
		fail_msg("Expected COIL_ERR_IO when loading non-existent file, got %d (%s)", 
				err, coil_error_string(err));
	}
	
	/* Verify callback was called */
	if (!error_callback_called) {
		fail_msg("Error callback was not called");
	}
	
	if (last_error_level != COIL_LEVEL_ERROR) {
		fail_msg("Expected error level COIL_LEVEL_ERROR, got %d", last_error_level);
	}
	
	/* Get the last error */
	const coil_error_context_t *ctx = coil_error_get_last();
	assert_non_null(ctx);
	
	if (ctx->code != COIL_ERR_IO) {
		fail_msg("Last error code mismatch: expected COIL_ERR_IO, got %d (%s)", 
				ctx->code, coil_error_string(ctx->code));
	}
	
	if (verbosity) {
		printf("  Error callback properly triggered\n");
		printf("  Last error: %s\n", coil_error_string(ctx->code));
		printf("  Error message: %s\n", ctx->message);
	}
	
	/* Test error system with COIL macros */
	error_callback_called = 0;
	last_error_message[0] = '\0';
	
	if (verbosity) {
		printf("  Testing warning macro...\n");
	}
	
	const char *test_message = "Test warning message";
	COIL_WARNING(COIL_ERR_NOTFOUND, test_message);
	
	if (!error_callback_called) {
		fail_msg("Error callback was not called for warning");
	}
	
	if (last_error_level != COIL_LEVEL_WARNING) {
		fail_msg("Expected error level COIL_LEVEL_WARNING, got %d", last_error_level);
	}
	
	if (strcmp(last_error_message, test_message) != 0) {
		fail_msg("Error message mismatch: expected '%s', got '%s'", 
				test_message, last_error_message);
	}
	
	/* Clean up */
	coil_object_destroy(obj, arena);
}

/* Test with a complete workflow using all components */
static void test_complete_workflow(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nTesting complete workflow:\n");
	}
	
	/* Get library version information */
	coil_version_t version;
	coil_err_t err = coil_get_version(&version);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to get version: %s", coil_error_string(err));
	}
	
	if (verbosity) {
		printf("  COIL version: %s\n", version.string);
		printf("  Major: %d, Minor: %d, Patch: %d\n", 
				version.major, version.minor, version.patch);
	}
	
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
	
	if (verbosity) {
		printf("  Creating factorial function...\n");
	}
	
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
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to initialize string table: %s", coil_error_string(err));
	}
	
	/* Add section and symbol names */
	coil_u64_t text_name = coil_object_add_string(obj, ".text", arena);
	coil_u64_t factorial_name = coil_object_add_string(obj, "factorial", arena);
	
	/* Add code section */
	size_t code_size = arena_used(instr_arena);
	void *code_data = instr_arena->first_block->memory; /* Get pointer to the start of arena */
	
	if (!code_data || code_size == 0) {
		fail_msg("Invalid code data: pointer=%p, size=%zu", code_data, code_size);
	}
	
	if (verbosity) {
		printf("  Generated code size: %zu bytes\n", code_size);
	}
	
	coil_u16_t text_index = coil_object_add_section(
		obj,
		text_name,
		COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
		COIL_SECTION_PROGBITS,
		code_data,
		code_size,
		arena
	);
	
	if (text_index == 0) {
		fail_msg("Failed to add .text section");
	}
	
	/* Initialize symbol table */
	err = coil_object_init_symbol_table(obj, arena);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to initialize symbol table: %s", coil_error_string(err));
	}
	
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
	
	if (factorial_sym == 0) {
		fail_msg("Failed to add factorial symbol");
	}
	
	/* Print object info */
	debug_print_obj_info(obj, "Factorial function object");
	
	/* Save to file */
	err = coil_object_save_to_file(obj, TEST_FILE_PATH);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to save object to file: %s", coil_error_string(err));
	}
	
	if (verbosity) {
		printf("  Saved factorial function to file: %s\n", TEST_FILE_PATH);
	}
	
	/* Load the file back to verify */
	coil_object_t *loaded_obj = coil_object_create(arena);
	assert_non_null(loaded_obj);
	
	err = coil_object_load_from_file(loaded_obj, TEST_FILE_PATH, arena);
	if (err != COIL_ERR_GOOD) {
		fail_msg("Failed to load object from file: %s", coil_error_string(err));
	}
	
	if (verbosity) {
		printf("  Successfully loaded file back\n");
	}
	
	/* Verify the object */
	coil_u16_t found_index = coil_object_get_section_index(loaded_obj, ".text");
	if (found_index == 0) {
		fail_msg("Failed to find .text section in loaded object");
	}
	
	coil_u16_t found_sym = coil_object_get_symbol_index(loaded_obj, "factorial");
	if (found_sym == 0) {
		fail_msg("Failed to find factorial symbol in loaded object");
	}
	
	if (verbosity) {
		printf("  Verified section and symbol exist in loaded object\n");
		
		// Verify section contents size
		coil_section_header_t section;
		const void *section_data;
		coil_u64_t section_size;
		
		if (coil_object_get_section(loaded_obj, found_index, &section, &section_data, &section_size) == COIL_ERR_GOOD) {
			printf("  Loaded .text section size: %llu bytes\n", (unsigned long long)section_size);
			if (section_size != code_size) {
				fail_msg("Loaded section size mismatch: expected %zu, got %llu", 
						code_size, (unsigned long long)section_size);
			}
		}
	}
	
	/* Cleanup */
	arena_destroy(instr_arena);
	coil_object_destroy(obj, arena);
	coil_object_destroy(loaded_obj, arena);
}

/* Get integration tests for combined testing */
struct CMUnitTest *get_integration_tests(int *count) {
	static struct CMUnitTest integration_tests[] = {
		cmocka_unit_test_setup_teardown(test_create_full_program, setup, teardown),
		cmocka_unit_test_setup_teardown(test_load_and_inspect, setup, teardown),
		cmocka_unit_test_setup_teardown(test_error_handling, setup, teardown),
		cmocka_unit_test_setup_teardown(test_complete_workflow, setup, teardown),
	};
	
	*count = sizeof(integration_tests) / sizeof(integration_tests[0]);
	printf("[get_integration_tests] Returning %d tests\n", *count);
	return integration_tests;
}

/* Individual test main function */
#ifdef RUN_INDIVIDUAL
int main(void) {
	printf("Running integration tests individually\n");
	
	int count;
	struct CMUnitTest *tests = get_integration_tests(&count);
	printf("Running %d tests\n", count);
	
	return _cmocka_run_group_tests("Integration Tests", tests, count, NULL, NULL);
}
#endif