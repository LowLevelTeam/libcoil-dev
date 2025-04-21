/**
 * @file test_instr.c
 * @brief Tests for the instruction encoding and decoding
 */

#include "test_framework.h"
#include <coil/instr.h>
#include <coil/arena.h>
#include <string.h>
#include <stdint.h>

// Helper to print opcode name
const char* opcode_to_string(coil_opcode_t opcode) {
  switch (opcode) {
    case COIL_OP_NOP: return "NOP";
    case COIL_OP_BR: return "BR";
    case COIL_OP_JMP: return "JMP";
    case COIL_OP_CALL: return "CALL";
    case COIL_OP_RET: return "RET";
    case COIL_OP_CMP: return "CMP";
    case COIL_OP_TEST: return "TEST";
    case COIL_OP_MOV: return "MOV";
    case COIL_OP_PUSH: return "PUSH";
    case COIL_OP_POP: return "POP";
    case COIL_OP_LEA: return "LEA";
    case COIL_OP_LOAD: return "LOAD";
    case COIL_OP_STOR: return "STOR";
    case COIL_OP_ADD: return "ADD";
    case COIL_OP_SUB: return "SUB";
    case COIL_OP_MUL: return "MUL";
    case COIL_OP_DIV: return "DIV";
    case COIL_OP_MOD: return "MOD";
    case COIL_OP_INC: return "INC";
    case COIL_OP_DEC: return "DEC";
    case COIL_OP_NEG: return "NEG";
    // Add other cases as needed
    default: return "UNKNOWN";
  }
}

// Helper to print value type
const char* value_type_to_string(coil_value_type_t type) {
  switch (type) {
    case COIL_VAL_I8: return "I8";
    case COIL_VAL_I16: return "I16";
    case COIL_VAL_I32: return "I32";
    case COIL_VAL_I64: return "I64";
    case COIL_VAL_U8: return "U8";
    case COIL_VAL_U16: return "U16";
    case COIL_VAL_U32: return "U32";
    case COIL_VAL_U64: return "U64";
    case COIL_VAL_F32: return "F32";
    case COIL_VAL_F64: return "F64";
    case COIL_VAL_PTR: return "PTR";
    case COIL_VAL_REG: return "REG";
    // Add other cases as needed
    default: return "UNKNOWN";
  }
}

// Helper to print operand type
const char* operand_type_to_string(coil_operand_type_t type) {
  switch (type) {
    case COIL_TYPEOP_NONE: return "NONE";
    case COIL_TYPEOP_REG: return "REG";
    case COIL_TYPEOP_VAR: return "VAR";
    case COIL_TYPEOP_EXP: return "EXP";
    case COIL_TYPEOP_IMM: return "IMM";
    case COIL_TYPEOP_SYM: return "SYM";
    case COIL_TYPEOP_OFF: return "OFF";
    default: return "UNKNOWN";
  }
}

// Helper to dump instruction data from arena
void dump_instruction_data(coil_arena_t* arena, const char* label) {
  if (!g_test_verbose || !arena) return;
  
  printf("Instruction data for %s:\n", label);
  size_t used = arena_used(arena);
  
  if (used == 0) {
    printf("  <empty>\n\n");
    return;
  }
  
  void* data = arena->first_block->memory;
  hexdump(data, used, "encoded instruction");
  
  // Try to decode basic structure
  if (used >= 1) {
    uint8_t* bytes = (uint8_t*)data;
    coil_opcode_t opcode = decode_opcode(bytes);
    printf("  Opcode: 0x%02x (%s)\n", opcode, opcode_to_string(opcode));
    
    if (has_operand_count(bytes, opcode)) {
      uint8_t operand_count = decode_operand_count(bytes);
      printf("  Operand count: %d\n", operand_count);
    }
  }
  printf("\n");
}

// Test encoding instructions without operands
void test_encode_instr_void(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Get initial used size
  size_t initial_used = arena_used(arena);
  
  // Encode a NOP instruction
  encode_instr_void(arena, COIL_OP_NOP);
  
  if (g_test_verbose) {
    printf("After encoding NOP:\n");
    dump_instruction_data(arena, "NOP instruction");
  }
  
  // Check size increased by 1 byte
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 1, new_used, "Void instruction should use 1 byte");
  
  // Check encoded data using decoder
  void* data = arena->first_block->memory;
  coil_opcode_t decoded_op = decode_opcode(data);
  TEST_ASSERT_EQUAL_INT(COIL_OP_NOP, decoded_op, "Decoded NOP should match opcode");
  
  // Save the current position in the arena
  size_t second_position = new_used;
  
  // Try another instruction
  encode_instr_void(arena, COIL_OP_RET);
  
  if (g_test_verbose) {
    printf("After encoding RET:\n");
    dump_instruction_data(arena, "RET instruction");
    
    // Debug: Print the memory content
    printf("Memory content: ");
    uint8_t* bytes = (uint8_t*)arena->first_block->memory;
    for (size_t i = 0; i < arena_used(arena); i++) {
      printf("%02x ", bytes[i]);
    }
    printf("\n");
  }
  
  // Check size increased by another byte
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(second_position + 1, new_used, "Second void instruction should use 1 more byte");
  
  // Check encoded data - the RET opcode should be at position corresponding to second_position
  void* second_instr = (uint8_t*)data + second_position;
  coil_opcode_t second_op = decode_opcode(second_instr);
  TEST_ASSERT_EQUAL_INT(COIL_OP_RET, second_op, "Decoded RET should match opcode");
  
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
  
  if (g_test_verbose) {
    printf("After encoding ADD with 3 operands:\n");
    dump_instruction_data(arena, "ADD instruction");
  }
  
  // Check size increased by 2 bytes
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 2, new_used, "Instruction with count should use 2 bytes");
  
  // Check encoded data using decoders
  void* data = arena->first_block->memory;
  coil_opcode_t decoded_op = decode_opcode(data);
  TEST_ASSERT_EQUAL_INT(COIL_OP_ADD, decoded_op, "Decoded ADD should match opcode");
  
  uint8_t operand_count = decode_operand_count(data);
  TEST_ASSERT_EQUAL_INT(3, operand_count, "Decoded operand count should match");
  
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
  
  if (g_test_verbose) {
    printf("After encoding U8 immediate (42):\n");
    dump_instruction_data(arena, "U8 immediate");
  }
  
  // Check size increased (3 bytes header + 1 byte data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 4, new_used, "U8 immediate should use 4 bytes");
  
  // Check encoded data using decoder functions
  void* data = arena->first_block->memory;
  coil_operand_header_t header;
  decode_operand_header(data, &header);
  
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_IMM, header.type, "Operand type should be immediate");
  TEST_ASSERT_EQUAL_INT(COIL_VAL_U8, header.value_type, "Value type should be U8");
  TEST_ASSERT_EQUAL_INT(COIL_MOD_CONST, header.modifier, "Modifier should be CONST");
  
  // Decode the actual value
  uint8_t decoded_value;
  int result = decode_operand_u8(data, &decoded_value);
  TEST_ASSERT(result, "Should successfully decode U8 value");
  TEST_ASSERT_EQUAL_INT(val_u8, decoded_value, "U8 value should match");
  
  // Test 16-bit immediate
  uint16_t val_u16 = 0x1234;
  encode_operand_imm(arena, COIL_VAL_U16, COIL_MOD_NONE, &val_u16);
  
  if (g_test_verbose) {
    printf("After encoding U16 immediate (0x1234):\n");
    dump_instruction_data(arena, "U16 immediate");
  }
  
  // Check size increased (3 bytes header + 2 bytes data)
  size_t u16_position = new_used;
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(u16_position + 5, new_used, "U16 immediate should use 5 more bytes");
  
  // Check encoded data using decoder
  void* u16_data = (uint8_t*)data + u16_position;
  uint16_t decoded_u16;
  result = decode_operand_u16(u16_data, &decoded_u16);
  TEST_ASSERT(result, "Should successfully decode U16 value");
  TEST_ASSERT_EQUAL_INT(val_u16, decoded_u16, "U16 value should match");
  
  // Test 32-bit immediate
  uint32_t val_u32 = 0x12345678;
  encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_VOL, &val_u32);
  
  if (g_test_verbose) {
    printf("After encoding U32 immediate (0x12345678):\n");
    dump_instruction_data(arena, "U32 immediate");
  }
  
  // Check size increased (3 bytes header + 4 bytes data)
  size_t u32_position = new_used;
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(u32_position + 7, new_used, "U32 immediate should use 7 more bytes");
  
  // Check encoded data using decoder
  void* u32_data = (uint8_t*)data + u32_position;
  uint32_t decoded_u32;
  result = decode_operand_u32(u32_data, &decoded_u32);
  TEST_ASSERT(result, "Should successfully decode U32 value");
  TEST_ASSERT_EQUAL_INT(val_u32, decoded_u32, "U32 value should match");
  
  // Test 64-bit immediate
  uint64_t val_u64 = 0x123456789ABCDEF0ULL;
  encode_operand_imm(arena, COIL_VAL_U64, COIL_MOD_ATOMIC, &val_u64);
  
  if (g_test_verbose) {
    printf("After encoding U64 immediate (0x123456789ABCDEF0):\n");
    dump_instruction_data(arena, "U64 immediate");
  }
  
  // Check size increased (3 bytes header + 8 bytes data)
  size_t u64_position = new_used;
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(u64_position + 11, new_used, "U64 immediate should use 11 more bytes");
  
  // Check encoded data using decoder
  void* u64_data = (uint8_t*)data + u64_position;
  uint64_t decoded_u64;
  result = decode_operand_u64(u64_data, &decoded_u64);
  TEST_ASSERT(result, "Should successfully decode U64 value");
  TEST_ASSERT_EQUAL_UINT64(val_u64, decoded_u64, "U64 value should match");
  
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
  
  if (g_test_verbose) {
    printf("After encoding register reference (r5):\n");
    dump_instruction_data(arena, "Register reference");
  }
  
  // Check size increased (3 bytes header + 4 bytes data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 7, new_used, "Register reference should use 7 bytes");
  
  // Check encoded data using decoder
  void* data = arena->first_block->memory;
  uint32_t decoded_reg;
  int result = decode_operand_u32(data, &decoded_reg);
  TEST_ASSERT(result, "Should successfully decode register value");
  TEST_ASSERT_EQUAL_INT(reg, decoded_reg, "Register value should match");
  
  // Test variable reference
  uint64_t var_ref = 0xABCDEF0123456789ULL;
  encode_operand_u64(arena, COIL_TYPEOP_VAR, COIL_VAL_VAR, COIL_MOD_CONST, var_ref);
  
  if (g_test_verbose) {
    printf("After encoding variable reference (0x%llx):\n", (unsigned long long)var_ref);
    dump_instruction_data(arena, "Variable reference");
  }
  
  // Check size increased (3 bytes header + 8 bytes data)
  size_t var_position = new_used;
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(var_position + 11, new_used, "Variable reference should use 11 more bytes");
  
  // Check encoded data using decoder
  void* var_data = (uint8_t*)data + var_position;
  uint64_t decoded_var;
  result = decode_operand_u64(var_data, &decoded_var);
  TEST_ASSERT(result, "Should successfully decode variable reference");
  TEST_ASSERT_EQUAL_UINT64(var_ref, decoded_var, "Variable reference should match");
  
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
  
  if (g_test_verbose) {
    printf("After encoding offset immediate:\n");
    dump_instruction_data(arena, "Offset immediate");
    
    // Debug data
    uint8_t* bytes = (uint8_t*)arena->first_block->memory;
    size_t new_used = arena_used(arena);
    printf("First 32 bytes: ");
    for (size_t i = 0; i < 32 && i < new_used; i++) {
      printf("%02x ", bytes[i]);
      if ((i+1) % 8 == 0) printf(" ");
    }
    printf("\n");
  }
  
  // Check size increased (4 bytes header + 8+8+8 bytes offset + 4 bytes data)
  size_t new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_used + 32, new_used, "Offset immediate should use 32 bytes");
  
  // Check encoded data using decoder functions
  void* data = arena->first_block->memory;
  
  // Check offset components
  uint64_t decoded_disp;
  int result = decode_offset_displacement(data, &decoded_disp);
  TEST_ASSERT(result, "Should successfully decode displacement");
  TEST_ASSERT_EQUAL_UINT64(disp, decoded_disp, "Displacement should match");
  
  uint64_t decoded_index;
  result = decode_offset_index(data, &decoded_index);
  TEST_ASSERT(result, "Should successfully decode index");
  TEST_ASSERT_EQUAL_UINT64(index, decoded_index, "Index should match");
  
  uint64_t decoded_scale;
  result = decode_offset_scale(data, &decoded_scale);
  TEST_ASSERT(result, "Should successfully decode scale");
  TEST_ASSERT_EQUAL_UINT64(scale, decoded_scale, "Scale should match");
  
  // Test offset with register
  uint32_t reg = 7;
  arena_reset(arena);
  encode_operand_off_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 
                         disp, index, scale, reg);
  
  if (g_test_verbose) {
    printf("After encoding offset register:\n");
    dump_instruction_data(arena, "Offset register");
  }
  
  // Check size increased (4 bytes header + 8+8+8 bytes offset + 4 bytes data)
  new_used = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(32, new_used, "Offset register should use 32 bytes");
  
  // Check offset components again
  data = arena->first_block->memory;
  result = decode_offset_displacement(data, &decoded_disp);
  TEST_ASSERT(result, "Should successfully decode displacement");
  TEST_ASSERT_EQUAL_UINT64(disp, decoded_disp, "Displacement should match");
  
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
  
  if (g_test_verbose) {
    printf("After encoding MOV r1, #42:\n");
    dump_instruction_data(arena, "MOV instruction");
  }
  
  // Verify total size
  size_t total_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(2 + 7 + 7, total_size, "Complete MOV instruction should use 16 bytes");
  
  // Verify using decoder functions
  void* data = arena->first_block->memory;
  coil_opcode_t decoded_op = decode_opcode(data);
  TEST_ASSERT_EQUAL_INT(COIL_OP_MOV, decoded_op, "Decoded MOV should match opcode");
  
  uint8_t operand_count = decode_operand_count(data);
  TEST_ASSERT_EQUAL_INT(2, operand_count, "Operand count should be 2");
  
  // Get first operand (register)
  const void* first_operand = get_first_operand(data);
  TEST_ASSERT_NOT_NULL(first_operand, "First operand should be found");
  
  uint32_t reg_val;
  int result = decode_operand_u32(first_operand, &reg_val);
  TEST_ASSERT(result, "Should successfully decode register");
  TEST_ASSERT_EQUAL_INT(1, reg_val, "Register should be r1");
  
  // Get second operand (immediate)
  const void* second_operand = get_next_operand(first_operand);
  TEST_ASSERT_NOT_NULL(second_operand, "Second operand should be found");
  
  uint32_t imm_val;
  result = decode_operand_u32(second_operand, &imm_val);
  TEST_ASSERT(result, "Should successfully decode immediate");
  TEST_ASSERT_EQUAL_INT(42, imm_val, "Immediate value should be 42");
  
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
  
  if (g_test_verbose) {
    printf("After encoding ADD r3, r1, r2:\n");
    dump_instruction_data(arena, "ADD instruction");
  }
  
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
  
  if (g_test_verbose) {
    printf("After extreme cases:\n");
    dump_instruction_data(arena, "Extreme tests");
  }
  
  arena_destroy(arena);
}

// Test decoder functions directly
void test_decoder_functions(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena creation should succeed");
  
  // Create some test data
  uint8_t test_data[32] = {0};
  
  // Set up a simple instruction: MOV r1, #42
  test_data[0] = COIL_OP_MOV;
  test_data[1] = 2; // 2 operands
  
  // First operand: register r1
  test_data[2] = COIL_TYPEOP_REG;
  test_data[3] = COIL_VAL_REG;
  test_data[4] = COIL_MOD_NONE;
  test_data[5] = 1; // r1 (little-endian)
  test_data[6] = 0;
  test_data[7] = 0;
  test_data[8] = 0;
  
  // Second operand: immediate 42
  test_data[9] = COIL_TYPEOP_IMM;
  test_data[10] = COIL_VAL_U32;
  test_data[11] = COIL_MOD_CONST;
  test_data[12] = 42; // 42 (little-endian)
  test_data[13] = 0;
  test_data[14] = 0;
  test_data[15] = 0;
  
  // Test decoder functions
  coil_opcode_t op = decode_opcode(test_data);
  TEST_ASSERT_EQUAL_INT(COIL_OP_MOV, op, "decode_opcode should extract MOV");
  
  uint8_t count = decode_operand_count(test_data);
  TEST_ASSERT_EQUAL_INT(2, count, "decode_operand_count should extract 2");
  
  int has_count = has_operand_count(test_data, COIL_OP_MOV);
  TEST_ASSERT(has_count, "MOV should have operand count");
  
  size_t instr_size = get_instruction_size(test_data);
  TEST_ASSERT_EQUAL_SIZE(2, instr_size, "MOV instruction size should be 2");
  
  const void* first_op = get_first_operand(test_data);
  TEST_ASSERT_NOT_NULL(first_op, "First operand should be found");
  TEST_ASSERT_EQUAL_PTR(&test_data[2], first_op, "First operand should start at byte 2");
  
  coil_operand_header_t header;
  size_t header_size = decode_operand_header(first_op, &header);
  TEST_ASSERT_EQUAL_SIZE(3, header_size, "Operand header size should be 3");
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_REG, header.type, "Operand type should be REG");
  
  uint32_t reg_val;
  int result = decode_operand_u32(first_op, &reg_val);
  TEST_ASSERT(result, "Should decode register value");
  TEST_ASSERT_EQUAL_INT(1, reg_val, "Register should be r1");
  
  const void* second_op = get_next_operand(first_op);
  TEST_ASSERT_NOT_NULL(second_op, "Second operand should be found");
  TEST_ASSERT_EQUAL_PTR(&test_data[9], second_op, "Second operand should start at byte 9");
  
  uint32_t imm_val;
  result = decode_operand_u32(second_op, &imm_val);
  TEST_ASSERT(result, "Should decode immediate value");
  TEST_ASSERT_EQUAL_INT(42, imm_val, "Immediate should be 42");
  
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
  {"Instruction Extreme Cases", test_instruction_extreme},
  {"Decoder Functions", test_decoder_functions}
};

// Run instruction tests
void run_instr_tests(void) {
  run_tests(instr_tests, sizeof(instr_tests) / sizeof(instr_tests[0]));
}