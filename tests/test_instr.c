/**
 * @file test_instr.c
 * @brief Tests for the instruction serialization system
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <cmocka.h>

#include <coil/instr.h>
#include <coil/arena.h>

// For combined test mode
#ifndef RUN_INDIVIDUAL
extern int test_verbosity;
#endif

/**
 * @brief Debug print function for instruction data
 */
static void debug_print_bytes(const uint8_t* data, size_t size) {
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (!verbosity) return;
	
	printf("Instruction data (%zu bytes):\n", size);
	printf("  ");
	for (size_t i = 0; i < size; i++) {
		printf("%02X ", data[i]);
		if ((i + 1) % 8 == 0 && i < size - 1) {
			printf("\n  ");
		}
	}
	printf("\n");
}

/* Setup function for tests that need an arena */
static int setup_arena(void **state) {
	coil_arena_t *arena = arena_init(4096, 0);
	if (!arena) {
		return -1;
	}
	
	*state = arena;
	return 0;
}

/* Teardown function for tests that need an arena */
static int teardown_arena(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	if (arena) {
		arena_destroy(arena);
	}
	return 0;
}

/* Test encoding basic instructions */
static void test_encode_instr(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Encode a NOP instruction */
	encode_instr(arena, COIL_OP_NOP, 0);
	
	/* Buffer should now have 2 bytes: opcode and operand count */
	size_t used = arena_used(arena);
	if (used != 2) {
		fail_msg("Expected 2 bytes used, but got %zu bytes", used);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	/* Debug output */
	debug_print_bytes(data, arena_used(arena));
	
	/* Check that the data matches expectations */
	assert_int_equal(data[0], COIL_OP_NOP);
	assert_int_equal(data[1], 0);
	
	/* Encode an ADD instruction with 3 operands */
	arena_reset(arena);
	encode_instr(arena, COIL_OP_ADD, 3);
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	
	assert_int_equal(data[0], COIL_OP_ADD);
	assert_int_equal(data[1], 3);
}

/* Test encoding instructions without operand count */
static void test_encode_instr_void(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Encode a RET instruction */
	encode_instr_void(arena, COIL_OP_RET);
	
	/* Buffer should have 1 byte: just the opcode */
	size_t used = arena_used(arena);
	if (used != 1) {
		fail_msg("Expected 1 byte used, but got %zu bytes", used);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	/* Debug output */
	debug_print_bytes(data, arena_used(arena));
	
	/* Check that the data matches expectations */
	assert_int_equal(data[0], COIL_OP_RET);
}

/* Test encoding immediate operands */
static void test_encode_operand_imm(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Encode a 32-bit integer immediate */
	uint32_t value = 0x12345678;
	encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, &value);
	
	/* Calculate expected size: 1 (optype) + 1 (valtype) + 1 (mod) + 4 (value) */
	size_t expected_size = 7;
	size_t actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("Expected %zu bytes used, but got %zu bytes", expected_size, actual_size);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	/* Debug output */
	debug_print_bytes(data, arena_used(arena));
	
	/* Check that the data matches expectations */
	assert_int_equal(data[0], COIL_TYPEOP_IMM);
	assert_int_equal(data[1], COIL_VAL_U32);
	assert_int_equal(data[2], COIL_MOD_NONE);
	
	/* Check the value directly by reconstructing it */
	uint32_t decoded = 0;
	decoded |= (uint32_t)data[3];
	decoded |= (uint32_t)data[4] << 8;
	decoded |= (uint32_t)data[5] << 16;
	decoded |= (uint32_t)data[6] << 24;
	
	if (decoded != value) {
		fail_msg("Decoded value mismatch: expected 0x%08X, got 0x%08X", value, decoded);
	}
	
	/* Test with a modifier */
	arena_reset(arena);
	encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_CONST, &value);
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	assert_int_equal(data[2], COIL_MOD_CONST);
	
	/* Test with 8-bit value */
	arena_reset(arena);
	uint8_t small_value = 42;
	encode_operand_imm(arena, COIL_VAL_U8, COIL_MOD_NONE, &small_value);
	
	/* Size: 1 (optype) + 1 (valtype) + 1 (mod) + 1 (value) */
	expected_size = 4;
	actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("For 8-bit value: Expected %zu bytes used, but got %zu bytes", 
				expected_size, actual_size);
	}
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	assert_int_equal(data[3], small_value);
	
	/* Test with 64-bit value */
	arena_reset(arena);
	uint64_t large_value = 0x1234567890ABCDEF;
	encode_operand_imm(arena, COIL_VAL_U64, COIL_MOD_NONE, &large_value);
	
	/* Size: 1 (optype) + 1 (valtype) + 1 (mod) + 8 (value) */
	expected_size = 11;
	actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("For 64-bit value: Expected %zu bytes used, but got %zu bytes", 
				expected_size, actual_size);
	}
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	
	/* Check the 64-bit value directly by reconstructing it byte by byte */
	uint64_t decoded_large = 0;
	decoded_large |= (uint64_t)data[3];
	decoded_large |= (uint64_t)data[4] << 8;
	decoded_large |= (uint64_t)data[5] << 16;
	decoded_large |= (uint64_t)data[6] << 24;
	decoded_large |= (uint64_t)data[7] << 32;
	decoded_large |= (uint64_t)data[8] << 40;
	decoded_large |= (uint64_t)data[9] << 48;
	decoded_large |= (uint64_t)data[10] << 56;
	
	if (decoded_large != large_value) {
		fail_msg("Decoded 64-bit value mismatch: expected 0x%016llX, got 0x%016llX", 
				(unsigned long long)large_value, (unsigned long long)decoded_large);
	}
}

/* Test encoding register operands */
static void test_encode_operand_u32(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Encode a register operand */
	uint32_t reg = 5;
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, reg);
	
	/* Calculate expected size: 1 (optype) + 1 (valtype) + 1 (mod) + 4 (value) */
	size_t expected_size = 7;
	size_t actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("Expected %zu bytes used, but got %zu bytes", expected_size, actual_size);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	/* Debug output */
	debug_print_bytes(data, arena_used(arena));
	
	/* Check that the data matches expectations */
	assert_int_equal(data[0], COIL_TYPEOP_REG);
	assert_int_equal(data[1], COIL_VAL_REG);
	assert_int_equal(data[2], COIL_MOD_NONE);
	
	/* Check the value directly by reconstructing it byte by byte */
	uint32_t decoded = 0;
	decoded |= (uint32_t)data[3];
	decoded |= (uint32_t)data[4] << 8;
	decoded |= (uint32_t)data[5] << 16;
	decoded |= (uint32_t)data[6] << 24;
	
	if (decoded != reg) {
		fail_msg("Decoded register value mismatch: expected %u, got %u", reg, decoded);
	}
}

/* Test encoding u64 reference operands */
static void test_encode_operand_u64(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Encode a symbol reference */
	uint64_t sym_ref = 0x1234567890ABCDEF;
	encode_operand_u64(arena, COIL_TYPEOP_SYM, COIL_VAL_SYM, COIL_MOD_NONE, sym_ref);
	
	/* Calculate expected size: 1 (optype) + 1 (valtype) + 1 (mod) + 8 (value) */
	size_t expected_size = 11;
	size_t actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("Expected %zu bytes used, but got %zu bytes", expected_size, actual_size);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	/* Debug output */
	debug_print_bytes(data, arena_used(arena));
	
	/* Check that the data matches expectations */
	assert_int_equal(data[0], COIL_TYPEOP_SYM);
	assert_int_equal(data[1], COIL_VAL_SYM);
	assert_int_equal(data[2], COIL_MOD_NONE);
	
	/* Check the value directly by reconstructing it byte by byte */
	uint64_t decoded = 0;
	decoded |= (uint64_t)data[3];
	decoded |= (uint64_t)data[4] << 8;
	decoded |= (uint64_t)data[5] << 16;
	decoded |= (uint64_t)data[6] << 24;
	decoded |= (uint64_t)data[7] << 32;
	decoded |= (uint64_t)data[8] << 40;
	decoded |= (uint64_t)data[9] << 48;
	decoded |= (uint64_t)data[10] << 56;
	
	if (decoded != sym_ref) {
		fail_msg("Decoded symbol reference mismatch: expected 0x%016llX, got 0x%016llX", 
				(unsigned long long)sym_ref, (unsigned long long)decoded);
	}
}

/* Test encoding offset-based operands */
static void test_encode_operand_off(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Test offset + immediate */
	uint32_t value = 42;
	encode_operand_off_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, 10, 2, 4, &value);
	
	/* Size: 1 (offtype) + 1 (optype) + 1 (valtype) + 1 (mod) + 
					 8 (disp) + 8 (index) + 8 (scale) + 4 (value) */
	size_t expected_size = 32;
	size_t actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("Expected %zu bytes used, but got %zu bytes", expected_size, actual_size);
	}
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	
	/* Check header fields */
	if (data[0] != COIL_TYPEOP_OFF) {
		fail_msg("Expected offset type 0x%02X, got 0x%02X", COIL_TYPEOP_OFF, data[0]);
	}
	if (data[1] != COIL_TYPEOP_IMM) {
		fail_msg("Expected immediate type 0x%02X, got 0x%02X", COIL_TYPEOP_IMM, data[1]);
	}
	
	/* Test with offset + u64 */
	arena_reset(arena);
	uint64_t ref = 0x1234567890ABCDEF;
	encode_operand_off_u64(arena, COIL_TYPEOP_SYM, COIL_VAL_SYM, COIL_MOD_NONE, 10, 2, 4, ref);
	
	/* Size: 1 (offtype) + 1 (optype) + 1 (valtype) + 1 (mod) + 
					 8 (disp) + 8 (index) + 8 (scale) + 8 (value) */
	expected_size = 36;
	actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("For offset+u64: Expected %zu bytes used, but got %zu bytes", 
				expected_size, actual_size);
	}
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
	
	/* Test with offset + u32 */
	arena_reset(arena);
	uint32_t reg = 5;
	encode_operand_off_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 10, 2, 4, reg);
	
	/* Size: 1 (offtype) + 1 (optype) + 1 (valtype) + 1 (mod) + 
					 8 (disp) + 8 (index) + 8 (scale) + 4 (value) */
	expected_size = 32;
	actual_size = arena_used(arena);
	if (actual_size != expected_size) {
		fail_msg("For offset+u32: Expected %zu bytes used, but got %zu bytes", 
				expected_size, actual_size);
	}
	
	data = arena->first_block->memory;
	debug_print_bytes(data, arena_used(arena));
}

/* Test creating a full instruction sequence */
static void test_full_instruction_sequence(void **state) {
	coil_arena_t *arena = (coil_arena_t *)*state;
	
	/* Reset arena */
	arena_reset(arena);
	
	/* Create a simple instruction sequence:
	 * MOV r1, #42
	 * MOV r2, #13
	 * ADD r3, r1, r2
	 */
	
#ifdef RUN_INDIVIDUAL
	static int verbosity = 1; // Always verbose in individual mode
#else
	int verbosity = test_verbosity;
#endif

	if (verbosity) {
		printf("\nGenerating instruction sequence:\n");
		printf("  MOV r1, #42\n");
		printf("  MOV r2, #13\n");
		printf("  ADD r3, r1, r2\n");
	}
	
	/* MOV r1, #42 */
	encode_instr(arena, COIL_OP_MOV, 2);
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
	uint32_t value1 = 42;
	encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, &value1);
	
	/* MOV r2, #13 */
	encode_instr(arena, COIL_OP_MOV, 2);
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
	uint32_t value2 = 13;
	encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, &value2);
	
	/* ADD r3, r1, r2 */
	encode_instr(arena, COIL_OP_ADD, 3);
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 3);
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
	encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
	
	/* Verify the size is correct (we'll calculate it properly instead of hardcoding) */
	/* Expected size:
	 * - MOV r1, #42:  2 (instr) + 7 (reg) + 7 (imm) = 16 bytes
	 * - MOV r2, #13:  2 (instr) + 7 (reg) + 7 (imm) = 16 bytes
	 * - ADD r3, r1, r2: 2 (instr) + 7 (reg) + 7 (reg) + 7 (reg) = 23 bytes
	 * Total: 55 bytes
	 */
	size_t expected_size = 16 + 16 + 23;
	size_t actual_size = arena_used(arena);
	
	/* Get the encoded data */
	uint8_t *data = arena->first_block->memory;
	
	if (verbosity) {
		printf("\nEncoded instruction sequence (%zu bytes):\n", arena_used(arena));
		debug_print_bytes(data, arena_used(arena));
	}
	
	if (actual_size != expected_size) {
		fail_msg("Instruction sequence size mismatch: expected %zu bytes, got %zu bytes",
			expected_size, actual_size);
		// Show detailed breakdown for debugging
		printf("Detailed size breakdown:\n");
		printf("- MOV r1, #42:      expected 16 bytes\n");
		printf("- MOV r2, #13:      expected 16 bytes\n");
		printf("- ADD r3, r1, r2:   expected 23 bytes\n");
		printf("Total:             expected %zu bytes, got %zu bytes\n",
			expected_size, actual_size);
	}
}

/* Get instruction tests for combined testing */
struct CMUnitTest *get_instr_tests(int *count) {
	static struct CMUnitTest instr_tests[] = {
		cmocka_unit_test_setup_teardown(test_encode_instr, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_encode_instr_void, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_encode_operand_imm, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_encode_operand_u32, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_encode_operand_u64, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_encode_operand_off, setup_arena, teardown_arena),
		cmocka_unit_test_setup_teardown(test_full_instruction_sequence, setup_arena, teardown_arena),
	};
	
	*count = sizeof(instr_tests) / sizeof(instr_tests[0]);
	printf("[get_instr_tests] Returning %d tests\n", *count);
	return instr_tests;
}

/* Individual test main function */
#ifdef RUN_INDIVIDUAL
int main(void) {
	printf("Running instruction tests individually\n");
	
	int count;
	struct CMUnitTest *tests = get_instr_tests(&count);
	printf("Running %d tests\n", count);
	
	return _cmocka_run_group_tests("Instruction Tests", tests, count, NULL, NULL);
}
#endif