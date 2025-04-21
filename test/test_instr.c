/**
 * @file test_instr.c
 * @brief Tests for the instruction encoding and decoding
 */

#include "test_framework.h"
#include <coil/instr.h>
#include <coil/arena.h>
#include <string.h>
#include <stdint.h>

// Helper to check memory content
static int memory_compare(const void* actual, const uint8_t* expected, size_t size) {
  return memcmp(actual, expected, size) == 0;
}

// Helper to print memory content for debugging - disabled to avoid unused function warning
/*static void print_memory(const char* label, const void* data, size_t size) {
  printf("%s: ", label);
  const uint8_t* bytes = (const uint8_t*)data;
  for (size_t i = 0; i < size; i++) {
    printf("%02x ", bytes[i]);
  }
  printf("\n");
}*/

// Test encoding instructions without operands
void test_encode_instr_void(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Encode a NOP instruction
  encode_instr_void(arena, COIL_OP_NOP);
  
  // Check size increased by 1 byte
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 1, new_used, "Void instruction should use 1 byte");
  
  // Check encoded data
  uint8_t* data = (uint8_t*)arena->first_block->memory;
  TEST_ASSERT_EQUAL_INT(COIL_OP_NOP, data[0], "Encoded NOP should match opcode");
  
  // Try another instruction
  encode_instr_void(arena, COIL_OP_RET);
  
  // Check size increased by another byte
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 2, new_used, "Second void instruction should use 1 more byte");
  
  // Check encoded data
  TEST_ASSERT_EQUAL_INT(COIL_OP_RET, data[1], "Encoded RET should match opcode");
  
  arena_destroy(arena);
}

// Test encoding instructions with operand count
void test_encode_instr(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Encode an ADD instruction with 3 operands
  encode_instr(arena, COIL_OP_ADD, 3);
  
  // Check size increased by 2 bytes
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 2, new_used, "Instruction with count should use 2 bytes");
  
  // Check encoded data
  uint8_t* data = (uint8_t*)arena->first_block->memory;
  TEST_ASSERT_EQUAL_INT(COIL_OP_ADD, data[0], "Encoded ADD should match opcode");
  TEST_ASSERT_EQUAL_INT(3, data[1], "Encoded operand count should match");
  
  arena_destroy(arena);
}

// Test encoding immediate operands
void test_encode_operand_imm(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Test 8-bit immediate
  uint8_t val_u8 = 42;
  encode_operand_imm(arena, COIL_VAL_U8, COIL_MOD_CONST, &val_u8);
  
  // Check size increased (3 bytes header + 1 byte data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4, new_used, "U8 immediate should use 4 bytes");
  
  // Check encoded data
  uint8_t* data = (uint8_t*)arena->first_block->memory;
  uint8_t expected_u8_header[3] = {COIL_TYPEOP_IMM, COIL_VAL_U8, COIL_MOD_CONST};
  TEST_ASSERT(memory_compare(data, expected_u8_header, 3), "U8 header should match");
  TEST_ASSERT_EQUAL_INT(val_u8, data[3], "U8 value should match");
  
  // Test 16-bit immediate
  uint16_t val_u16 = 0x1234;
  encode_operand_imm(arena, COIL_VAL_U16, COIL_MOD_NONE, &val_u16);
  
  // Check size increased (3 bytes header + 2 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4 + 5, new_used, "U16 immediate should use 5 more bytes");
  
  // Check encoded data - should be in little-endian format
  uint8_t expected_u16_header[3] = {COIL_TYPEOP_IMM, COIL_VAL_U16, COIL_MOD_NONE};
  TEST_ASSERT(memory_compare(data + 4, expected_u16_header, 3), "U16 header should match");
  TEST_ASSERT_EQUAL_INT(val_u16 & 0xFF, data[7], "U16 low byte should match");
  TEST_ASSERT_EQUAL_INT((val_u16 >> 8) & 0xFF, data[8], "U16 high byte should match");
  
  // Test 32-bit immediate
  uint32_t val_u32 = 0x12345678;
  encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_VOL, &val_u32);
  
  // Check size increased (3 bytes header + 4 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4 + 5 + 7, new_used, "U32 immediate should use 7 more bytes");
  
  // Check encoded data - should be in little-endian format
  uint8_t expected_u32_header[3] = {COIL_TYPEOP_IMM, COIL_VAL_U32, COIL_MOD_VOL};
  TEST_ASSERT(memory_compare(data + 9, expected_u32_header, 3), "U32 header should match");
  TEST_ASSERT_EQUAL_INT(val_u32 & 0xFF, data[12], "U32 byte 0 should match");
  TEST_ASSERT_EQUAL_INT((val_u32 >> 8) & 0xFF, data[13], "U32 byte 1 should match");
  TEST_ASSERT_EQUAL_INT((val_u32 >> 16) & 0xFF, data[14], "U32 byte 2 should match");
  TEST_ASSERT_EQUAL_INT((val_u32 >> 24) & 0xFF, data[15], "U32 byte 3 should match");
  
  // Test 64-bit immediate
  uint64_t val_u64 = 0x123456789ABCDEF0ULL;
  encode_operand_imm(arena, COIL_VAL_U64, COIL_MOD_ATOMIC, &val_u64);
  
  // Check size increased (3 bytes header + 8 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4 + 5 + 7 + 11, new_used, "U64 immediate should use 11 more bytes");
  
  // Check encoded data - should be in little-endian format
  uint8_t expected_u64_header[3] = {COIL_TYPEOP_IMM, COIL_VAL_U64, COIL_MOD_ATOMIC};
  TEST_ASSERT(memory_compare(data + 16, expected_u64_header, 3), "U64 header should match");
  TEST_ASSERT_EQUAL_INT(val_u64 & 0xFF, data[19], "U64 byte 0 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 8) & 0xFF, data[20], "U64 byte 1 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 16) & 0xFF, data[21], "U64 byte 2 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 24) & 0xFF, data[22], "U64 byte 3 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 32) & 0xFF, data[23], "U64 byte 4 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 40) & 0xFF, data[24], "U64 byte 5 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 48) & 0xFF, data[25], "U64 byte 6 should match");
  TEST_ASSERT_EQUAL_INT((val_u64 >> 56) & 0xFF, data[26], "U64 byte 7 should match");
  
  // Test float immediate
  float val_f32 = 3.14159f;
  encode_operand_imm(arena, COIL_VAL_F32, COIL_MOD_NONE, &val_f32);
  
  // Check size increased (3 bytes header + 4 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4 + 5 + 7 + 11 + 7, new_used, "F32 immediate should use 7 more bytes");
  
  // Cannot easily check exact bit pattern, but we can check the header
  uint8_t expected_f32_header[3] = {COIL_TYPEOP_IMM, COIL_VAL_F32, COIL_MOD_NONE};
  TEST_ASSERT(memory_compare(data + 27, expected_f32_header, 3), "F32 header should match");
  
  arena_destroy(arena);
}

// Test encoding reference operands
void test_encode_operand_refs(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Test register reference
  uint32_t reg = 5;
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, reg);
  
  // Check size increased (3 bytes header + 4 bytes data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 7, new_used, "Register reference should use 7 bytes");
  
  // Check encoded data
  uint8_t* data = (uint8_t*)arena->first_block->memory;
  uint8_t expected_reg_header[3] = {COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE};
  TEST_ASSERT(memory_compare(data, expected_reg_header, 3), "Register header should match");
  TEST_ASSERT_EQUAL_INT(reg & 0xFF, data[3], "Register byte 0 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 8) & 0xFF, data[4], "Register byte 1 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 16) & 0xFF, data[5], "Register byte 2 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 24) & 0xFF, data[6], "Register byte 3 should match");
  
  // Test variable reference
  uint64_t var_ref = 0xABCDEF0123456789ULL;
  encode_operand_u64(arena, COIL_TYPEOP_VAR, COIL_VAL_VAR, COIL_MOD_CONST, var_ref);
  
  // Check size increased (3 bytes header + 8 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 7 + 11, new_used, "Variable reference should use 11 more bytes");
  
  // Check encoded data
  uint8_t expected_var_header[3] = {COIL_TYPEOP_VAR, COIL_VAL_VAR, COIL_MOD_CONST};
  TEST_ASSERT(memory_compare(data + 7, expected_var_header, 3), "Variable header should match");
  
  // Check all 8 bytes of the u64 value in little-endian order
  for (int i = 0; i < 8; i++) {
    uint8_t expected_byte = (var_ref >> (i*8)) & 0xFF;
    TEST_ASSERT_EQUAL_INT(expected_byte, data[10 + i], "Variable reference byte should match");
  }
  
  arena_destroy(arena);
}

// Test encoding offset operands
void test_encode_operand_offset(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Test offset with immediate
  uint64_t disp = 16;
  uint64_t index = 3;
  uint64_t scale = 4;
  uint32_t val = 0x12345678;
  
  encode_operand_off_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, disp, index, scale, &val);
  
  // Check size increased (4 bytes header + 8+8+8 bytes offset + 4 bytes data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 32, new_used, "Offset immediate should use 32 bytes");
  
  // Check encoded data
  uint8_t* data = (uint8_t*)arena->first_block->memory;
  uint8_t expected_header[4] = {COIL_TYPEOP_OFF, COIL_TYPEOP_IMM, COIL_VAL_U32, COIL_MOD_NONE};
  TEST_ASSERT(memory_compare(data, expected_header, 4), "Offset header should match");
  
  // Check displacement value
  for (int i = 0; i < 8; i++) {
    uint8_t expected_byte = (disp >> (i*8)) & 0xFF;
    TEST_ASSERT_EQUAL_INT(expected_byte, data[4 + i], "Displacement byte should match");
  }
  
  // Check index value
  for (int i = 0; i < 8; i++) {
    uint8_t expected_byte = (index >> (i*8)) & 0xFF;
    TEST_ASSERT_EQUAL_INT(expected_byte, data[12 + i], "Index byte should match");
  }
  
  // Check scale value
  for (int i = 0; i < 8; i++) {
    uint8_t expected_byte = (scale >> (i*8)) & 0xFF;
    TEST_ASSERT_EQUAL_INT(expected_byte, data[20 + i], "Scale byte should match");
  }
  
  // Check immediate value in little-endian format
  TEST_ASSERT_EQUAL_INT(val & 0xFF, data[28], "Immediate byte 0 should match");
  TEST_ASSERT_EQUAL_INT((val >> 8) & 0xFF, data[29], "Immediate byte 1 should match");
  TEST_ASSERT_EQUAL_INT((val >> 16) & 0xFF, data[30], "Immediate byte 2 should match");
  TEST_ASSERT_EQUAL_INT((val >> 24) & 0xFF, data[31], "Immediate byte 3 should match");
  
  // Test offset with register
  uint32_t reg = 7;
  arena_reset(arena);
  encode_operand_off_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 
                         disp, index, scale, reg);
  
  // Check size increased (4 bytes header + 8+8+8 bytes offset + 4 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(32, new_used, "Offset register should use 32 bytes");
  
  // Check encoded data
  data = (uint8_t*)arena->first_block->memory;
  uint8_t expected_reg_header[4] = {COIL_TYPEOP_OFF, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE};
  TEST_ASSERT(memory_compare(data, expected_reg_header, 4), "Offset register header should match");
  
  // Check register value in little-endian format
  TEST_ASSERT_EQUAL_INT(reg & 0xFF, data[28], "Register byte 0 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 8) & 0xFF, data[29], "Register byte 1 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 16) & 0xFF, data[30], "Register byte 2 should match");
  TEST_ASSERT_EQUAL_INT((reg >> 24) & 0xFF, data[31], "Register byte 3 should match");
  
  arena_destroy(arena);
}

// Test complex instruction encoding
void test_complex_instruction(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Encode a complete MOV instruction: MOV r1, #42
  encode_instr(arena, COIL_OP_MOV, 2); // MOV with 2 operands
  
  // Destination: register r1
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  
  // Source: immediate value 42
  uint32_t value = 42;
  encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_CONST, &value);
  
  // Verify total size
  size_t total_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(2 + 7 + 7, total_size, "Complete MOV instruction should use 16 bytes");
  
  // Reset arena
  arena_reset(arena);
  
  // Encode an ADD instruction: ADD r3, r1, r2
  encode_instr(arena, COIL_OP_ADD, 3); // ADD with 3 operands
  
  // Destination: register r3
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 3);
  
  // Source 1: register r1
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  
  // Source 2: register r2
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 2);
  
  // Verify total size
  total_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(2 + 7 + 7 + 7, total_size, "Complete ADD instruction should use 23 bytes");
  
  arena_destroy(arena);
}

// Test extreme cases
void test_instruction_extreme(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Test with NULL arena (should do nothing but not crash)
  encode_instr_void(NULL, COIL_OP_NOP);
  encode_instr(NULL, COIL_OP_NOP, 0);
  
  uint32_t val = 42;
  encode_operand_imm(NULL, COIL_VAL_U32, COIL_MOD_NONE, &val);
  encode_operand_u32(NULL, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 1);
  encode_operand_u64(NULL, COIL_TYPEOP_VAR, COIL_VAL_VAR, COIL_MOD_NONE, 123);
  
  // Test with NULL data
  encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_NONE, NULL);
  
  // Test with unusual opcode values
  encode_instr_void(arena, 0xFF);  // Extension opcode
  
  // Test with all modifier bits set
  encode_operand_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, 
                    COIL_MOD_CONST | COIL_MOD_VOL | COIL_MOD_ATOMIC | COIL_MOD_MUT, 1);
  
  arena_destroy(arena);
}

// Define test array
test_t instr_tests[] = {
  {"Encode Instruction Void", test_encode_instr_void},
  {"Encode Instruction with Count", test_encode_instr},
  {"Encode Immediate Operands", test_encode_operand_imm},
  {"Encode Reference Operands", test_encode_operand_refs},
  {"Encode Offset Operands", test_encode_operand_offset},
  {"Encode Complex Instructions", test_complex_instruction},
  {"Instruction Extreme Cases", test_instruction_extreme}
};

// Run instruction tests
void run_instr_tests(void) {
  run_tests(instr_tests, sizeof(instr_tests) / sizeof(instr_tests[0]));
}