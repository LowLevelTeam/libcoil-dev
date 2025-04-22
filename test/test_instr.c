/**
 * @file test_instr.c
 * @brief Tests for the instruction encoding and decoding functionality
 */

#include "test_framework.h"
#include <coil/instr.h>
#include <coil/arena.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Utility to dump encoded instruction for debugging

// static void dump_encoded_data(const void* data, size_t size, const char* label) {
//   if (g_test_verbose) {
//     hexdump(data, size, label);
//   }
// }

// Test simple instruction encoding
void test_encode_instr(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Calculate initial size
  size_t initial_size = arena_used(arena);
  (void)initial_size;

  // Create a pointer to track the encoded data
  uint8_t* encoded_start = (uint8_t*)arena_alloc_default(arena, 2);
  TEST_ASSERT_NOT_NULL(encoded_start, "Memory allocation should succeed");
  
  // Record new size after allocation
  size_t after_alloc_size = arena_used(arena);
  
  // Encode a NOP instruction with 0 operands
  encode_instr(arena, COIL_OP_NOP, 0);
  
  // Calculate new size
  size_t after_encode_size = arena_used(arena);
  
  // Check that encode_instr added 2 bytes

  TEST_ASSERT_EQUAL_SIZE(after_alloc_size + 2, after_encode_size, "encode_instr should add 2 bytes");  

  // Verify the encoded data
  // uint8_t* encoded_data = (uint8_t*)arena_alloc_default(arena, 0) - 2;
  // TEST_ASSERT_EQUAL_INT(COIL_OP_NOP, encoded_data[0], "First byte should be NOP opcode");
  // TEST_ASSERT_EQUAL_INT(0, encoded_data[1], "Second byte should be operand count (0)");
  
  // dump_encoded_data(encoded_data, 2, "NOP instruction");
  
  // Free resources
  arena_destroy(arena);
}

// Test instruction without operand count
void test_encode_instr_void(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Record initial size
  size_t initial_size = arena_used(arena);
  
  // Encode a RET instruction (no operands)
  encode_instr_void(arena, COIL_OP_RET);
  
  // Check size increase
  size_t new_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_size + 1, new_size, "Void instruction should add 1 byte");
  
  // Verify the encoded data
  // uint8_t* encoded_data = (uint8_t*)arena_alloc_default(arena, 0) - 1;
  // TEST_ASSERT_EQUAL_INT(COIL_OP_RET, encoded_data[0], "Byte should be RET opcode");
  
  // dump_encoded_data(encoded_data, 1, "RET instruction");
  
  // Free resources
  arena_destroy(arena);
}

// Test operand encoding
void test_encode_operand(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Record initial size
  size_t initial_size = arena_used(arena);
  
  // Create operand header
  coil_operand_header_t header;
  header.type = COIL_TYPEOP_REG;
  header.value_type = COIL_VAL_U32;
  header.modifier = COIL_MOD_NONE;
  
  // Encode the operand
  encode_operand(arena, &header);
  
  // Check size increase
  size_t new_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_size + 3, new_size, "Operand header should add 3 bytes");
  
  // Verify the encoded data
  // uint8_t* encoded_data = (uint8_t*)arena_alloc_default(arena, 0) - 3;
  // TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_REG, encoded_data[0], "First byte should be REG type");
  // TEST_ASSERT_EQUAL_INT(COIL_VAL_U32, encoded_data[1], "Second byte should be U32 value type");
  // TEST_ASSERT_EQUAL_INT(COIL_MOD_NONE, encoded_data[2], "Third byte should be NONE modifier");

  // dump_encoded_data(encoded_data, 3, "Register operand header");
  
  // Free resources
  arena_destroy(arena);
}

// Test operand with offset encoding
void test_encode_operand_offset(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Record initial size
  size_t initial_size = arena_used(arena);
  
  // Create operand header with offset
  coil_operand_header_t header;
  header.type = COIL_TYPEOP_OFF;
  header.value_type = COIL_VAL_U64;
  header.modifier = COIL_MOD_NONE;
  
  // Create offset
  coil_offset_t offset;
  offset.disp = 0x1234567890ABCDEF;
  offset.index = 0x23;
  offset.scale = 0x8;
  
  // Encode the operand with offset
  encode_operand_off(arena, &header, &offset);
  
  // Check size increase
  size_t new_size = arena_used(arena);
  TEST_ASSERT_EQUAL_SIZE(initial_size + 3 + 24, new_size, "Operand with offset should add 27 bytes");

/*
  // Verify the encoded data
  uint8_t* encoded_data = (uint8_t*)arena_alloc_default(arena, 0) - 27;
  
  // Check header
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_OFF, encoded_data[0], "First byte should be OFFSET type");
  TEST_ASSERT_EQUAL_INT(COIL_VAL_U64, encoded_data[1], "Second byte should be U64 value type");
  TEST_ASSERT_EQUAL_INT(COIL_MOD_NONE, encoded_data[2], "Third byte should be NONE modifier");
  
  // Check offset values (assuming little-endian)
  uint64_t disp, index, scale;
  memcpy(&disp, encoded_data + 3, 8);
  memcpy(&index, encoded_data + 11, 8);
  memcpy(&scale, encoded_data + 19, 8);
  
  TEST_ASSERT_EQUAL_UINT64(offset.disp, disp, "Displacement value should match");
  TEST_ASSERT_EQUAL_UINT64(offset.index, index, "Index value should match");
  TEST_ASSERT_EQUAL_UINT64(offset.scale, scale, "Scale value should match");
  
  dump_encoded_data(encoded_data, 27, "Offset operand");
*/

  // Free resources
  arena_destroy(arena);
}

// Test operand data encoding
void test_encode_operand_data(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Test different data types
  struct {
    const char* name;
    void* data;
    size_t size;
  } test_cases[] = {
    {"8-bit value", &(uint8_t){0xAB}, sizeof(uint8_t)},
    {"16-bit value", &(uint16_t){0xABCD}, sizeof(uint16_t)},
    {"32-bit value", &(uint32_t){0xABCDEF12}, sizeof(uint32_t)},
    {"64-bit value", &(uint64_t){0xABCDEF1234567890}, sizeof(uint64_t)},
    {"Float value", &(float){3.14159f}, sizeof(float)},
    {"Double value", &(double){2.71828}, sizeof(double)}
  };
  
  // Test each case
  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
    // Record size before encoding
    size_t before_size = arena_used(arena);
    
    // Encode the data
    encode_operand_data(arena, test_cases[i].data, test_cases[i].size);
    
    // Check size increase
    size_t after_size = arena_used(arena);
    TEST_ASSERT_EQUAL_SIZE(
      before_size + test_cases[i].size, after_size,
      "Data encoding should add the correct number of bytes"
    );
    
/*
    // Verify the encoded data
    uint8_t* encoded_data = (uint8_t*)arena_alloc_default(arena, 0) - test_cases[i].size;
    TEST_ASSERT_EQUAL_INT(0, memcmp(test_cases[i].data, encoded_data, test_cases[i].size),
                         "Encoded data should match original");
    
    char label[64];
    snprintf(label, sizeof(label), "Encoded %s", test_cases[i].name);
    dump_encoded_data(encoded_data, test_cases[i].size, label);
*/
  }

  // Free resources
  arena_destroy(arena);
}

// Test complete instruction encoding and decoding
/*
void test_complete_instruction(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Point to start of buffer for encoded instruction
  uint8_t* buffer_start = (uint8_t*)arena_alloc_default(arena, 0);
  size_t initial_size = arena_used(arena);
  
  // Encode a MOV instruction: MOV r1, #42
  encode_instr(arena, COIL_OP_MOV, 2);
  
  // First operand: destination register (r1)
  coil_operand_header_t reg_header;
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg_id = 1; // Register r1
  encode_operand_data(arena, &reg_id, sizeof(reg_id));
  
  // Second operand: immediate value (42)
  coil_operand_header_t imm_header;
  imm_header.type = COIL_TYPEOP_IMM;
  imm_header.value_type = COIL_VAL_U32;
  imm_header.modifier = COIL_MOD_CONST;
  encode_operand(arena, &imm_header);
  
  uint32_t imm_value = 42;
  encode_operand_data(arena, &imm_value, sizeof(imm_value));
  
  // Calculate final size
  size_t final_size = arena_used(arena);
  size_t instruction_size = final_size - initial_size;
  
  // Expected size: 2 (instr) + 3 (reg header) + 4 (reg data) + 3 (imm header) + 4 (imm data) = 16
  TEST_ASSERT_EQUAL_SIZE(16, instruction_size, "Complete instruction size should be 16 bytes");
  
  // Dump the complete instruction
  dump_encoded_data(buffer_start, instruction_size, "MOV r1, #42 instruction");
  
  // Now test decoding
  size_t pos = 0;
  
  // Decode the instruction header
  coil_instr_t decoded_instr;
  pos = decode_instr(buffer_start, pos, &decoded_instr);
  
  // Verify instruction
  TEST_ASSERT_EQUAL_INT(COIL_OP_MOV, decoded_instr.opcode, "Decoded opcode should be MOV");
  TEST_ASSERT_EQUAL_INT(2, decoded_instr.operand_count, "Decoded operand count should be 2");
  
  // Decode first operand (register)
  coil_operand_header_t decoded_reg_header;
  coil_offset_t decoded_reg_offset;
  pos = decode_operand(buffer_start, pos, &decoded_reg_header, &decoded_reg_offset);
  
  // Verify register operand header
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_REG, decoded_reg_header.type, "Decoded operand type should be REG");
  TEST_ASSERT_EQUAL_INT(COIL_VAL_U32, decoded_reg_header.value_type, "Decoded value type should be U32");
  TEST_ASSERT_EQUAL_INT(COIL_MOD_NONE, decoded_reg_header.modifier, "Decoded modifier should be NONE");
  
  // Decode register value
  uint32_t decoded_reg_id = 0;
  size_t decoded_reg_size = 0;
  pos = decode_operand_data(buffer_start, pos, &decoded_reg_id, sizeof(decoded_reg_id), 
                          &decoded_reg_size, &decoded_reg_header);
  
  // Verify register value
  TEST_ASSERT_EQUAL_SIZE(sizeof(reg_id), decoded_reg_size, "Decoded register size should match");
  TEST_ASSERT_EQUAL_INT(reg_id, decoded_reg_id, "Decoded register ID should be 1");
  
  // Decode second operand (immediate)
  coil_operand_header_t decoded_imm_header;
  coil_offset_t decoded_imm_offset;
  pos = decode_operand(buffer_start, pos, &decoded_imm_header, &decoded_imm_offset);
  
  // Verify immediate operand header
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_IMM, decoded_imm_header.type, "Decoded operand type should be IMM");
  TEST_ASSERT_EQUAL_INT(COIL_VAL_U32, decoded_imm_header.value_type, "Decoded value type should be U32");
  TEST_ASSERT_EQUAL_INT(COIL_MOD_CONST, decoded_imm_header.modifier, "Decoded modifier should be CONST");
  
  // Decode immediate value
  uint32_t decoded_imm_value = 0;
  size_t decoded_imm_size = 0;
  pos = decode_operand_data(buffer_start, pos, &decoded_imm_value, sizeof(decoded_imm_value), 
                          &decoded_imm_size, &decoded_imm_header);
  
  // Verify immediate value
  TEST_ASSERT_EQUAL_SIZE(sizeof(imm_value), decoded_imm_size, "Decoded immediate size should match");
  TEST_ASSERT_EQUAL_INT(imm_value, decoded_imm_value, "Decoded immediate value should be 42");
  
  // Verify decode position
  TEST_ASSERT_EQUAL_SIZE(instruction_size, pos, "Decode position should match instruction size");
  
  // Free resources
  arena_destroy(arena);
}
*/

// Test decoding of various value types
void test_value_type_sizes(void) {
  // Define test cases for different value types
  struct {
    coil_value_type_t type;
    const char* name;
    size_t expected_size;
  } test_cases[] = {
    {COIL_VAL_I8, "I8", 1},
    {COIL_VAL_U8, "U8", 1},
    {COIL_VAL_I16, "I16", 2},
    {COIL_VAL_U16, "U16", 2},
    {COIL_VAL_I32, "I32", 4},
    {COIL_VAL_U32, "U32", 4},
    {COIL_VAL_F32, "F32", 4},
    {COIL_VAL_I64, "I64", 8},
    {COIL_VAL_U64, "U64", 8},
    {COIL_VAL_F64, "F64", 8},
    {COIL_VAL_PTR, "PTR", 8},
    {COIL_VAL_REG, "REG", 4},
    {COIL_VAL_VOID, "VOID", 0}
  };
  
  // Test each value type
  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
    // Create a dummy operand header with the current value type
    coil_operand_header_t header;
    header.type = COIL_TYPEOP_IMM;
    header.value_type = test_cases[i].type;
    header.modifier = COIL_MOD_NONE;
    
    // Create a dummy buffer for the value and for decoding
    uint8_t dummy_buffer[16] = {0};
    uint8_t decode_buffer[16] = {0};
    size_t actual_size = 0;
    
    // Call decode_operand_data to determine the value size
    decode_operand_data(dummy_buffer, 0, decode_buffer, sizeof(decode_buffer), 
                       &actual_size, &header);
    
    // Verify the size
    char message[100];
    snprintf(message, sizeof(message), "Value type %s should have size %zu", 
             test_cases[i].name, test_cases[i].expected_size);
    TEST_ASSERT_EQUAL_SIZE(test_cases[i].expected_size, actual_size, message);
  }
}

// Test decoding with offsets
/*
void test_decode_with_offset(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Point to start of buffer for encoded instruction
  uint8_t* buffer_start = (uint8_t*)arena_alloc_default(arena, 0);
  size_t initial_size = arena_used(arena);
  
  // Create an instruction with an offset operand
  // LOAD r1, [r2 + 4*r3 + 0x100]
  encode_instr(arena, COIL_OP_LOAD, 2);
  
  // First operand: destination register (r1)
  coil_operand_header_t reg_header;
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg_id = 1; // Register r1
  encode_operand_data(arena, &reg_id, sizeof(reg_id));
  
  // Second operand: memory address with offset
  coil_operand_header_t addr_header;
  addr_header.type = COIL_TYPEOP_OFF;
  addr_header.value_type = COIL_VAL_U32;
  addr_header.modifier = COIL_MOD_NONE;
  
  coil_offset_t addr_offset;
  addr_offset.disp = 0x100;     // Displacement: 0x100
  addr_offset.index = 3;        // Index register: r3
  addr_offset.scale = 4;        // Scale: 4
  
  encode_operand_off(arena, &addr_header, &addr_offset);
  
  uint32_t base_reg = 2; // Base register: r2
  encode_operand_data(arena, &base_reg, sizeof(base_reg));
  
  // Calculate final size
  size_t final_size = arena_used(arena);
  size_t instruction_size = final_size - initial_size;
  
  // Expected size: 2 (instr) + 3 (reg header) + 4 (reg data) + 
  //                3 (addr header) + 24 (offset) + 4 (base reg) = 40
  TEST_ASSERT_EQUAL_SIZE(40, instruction_size, "Instruction with offset should be 40 bytes");
  
  // Dump the complete instruction
  dump_encoded_data(buffer_start, instruction_size, "LOAD r1, [r2 + 4*r3 + 0x100] instruction");
  
  // Now test decoding
  size_t pos = 0;
  
  // Decode the instruction header
  coil_instr_t decoded_instr;
  pos = decode_instr(buffer_start, pos, &decoded_instr);
  
  // Verify instruction
  TEST_ASSERT_EQUAL_INT(COIL_OP_LOAD, decoded_instr.opcode, "Decoded opcode should be LOAD");
  TEST_ASSERT_EQUAL_INT(2, decoded_instr.operand_count, "Decoded operand count should be 2");
  
  // Decode first operand (register)
  coil_operand_header_t decoded_reg_header;
  coil_offset_t decoded_reg_offset;
  pos = decode_operand(buffer_start, pos, &decoded_reg_header, &decoded_reg_offset);
  
  // Verify register operand header
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_REG, decoded_reg_header.type, "Decoded operand type should be REG");
  
  // Decode register value
  uint32_t decoded_reg_id = 0;
  size_t decoded_reg_size = 0;
  pos = decode_operand_data(buffer_start, pos, &decoded_reg_id, sizeof(decoded_reg_id), 
                          &decoded_reg_size, &decoded_reg_header);
  
  // Verify register value
  TEST_ASSERT_EQUAL_INT(reg_id, decoded_reg_id, "Decoded register ID should be 1");
  
  // Decode second operand (memory address with offset)
  coil_operand_header_t decoded_addr_header;
  coil_offset_t decoded_addr_offset;
  pos = decode_operand(buffer_start, pos, &decoded_addr_header, &decoded_addr_offset);
  
  // Verify address operand header
  TEST_ASSERT_EQUAL_INT(COIL_TYPEOP_OFF, decoded_addr_header.type, "Decoded operand type should be OFFSET");
  TEST_ASSERT_EQUAL_INT(COIL_VAL_U32, decoded_addr_header.value_type, "Decoded value type should be U32");
  
  // Verify offset values
  TEST_ASSERT_EQUAL_UINT64(addr_offset.disp, decoded_addr_offset.disp, "Decoded disp should match");
  TEST_ASSERT_EQUAL_UINT64(addr_offset.index, decoded_addr_offset.index, "Decoded index should match");
  TEST_ASSERT_EQUAL_UINT64(addr_offset.scale, decoded_addr_offset.scale, "Decoded scale should match");
  
  // Decode base register
  uint32_t decoded_base_reg = 0;
  size_t decoded_base_size = 0;
  pos = decode_operand_data(buffer_start, pos, &decoded_base_reg, sizeof(decoded_base_reg), 
                          &decoded_base_size, &decoded_addr_header);
  
  // Verify base register
  TEST_ASSERT_EQUAL_INT(base_reg, decoded_base_reg, "Decoded base register should be 2");
  
  // Free resources
  arena_destroy(arena);
}
*/

// Test encoding and decoding multiple instructions
/*
void test_multiple_instructions(void) {
  coil_arena_t* arena = arena_init(4096, 0);
  TEST_ASSERT_NOT_NULL(arena, "Arena should be created");
  
  // Point to start of buffer for encoded instructions
  uint8_t* buffer_start = (uint8_t*)arena_alloc_default(arena, 0);
  
  // Instructions to encode:
  // 1. MOV r1, #10
  // 2. MOV r2, #20
  // 3. ADD r3, r1, r2
  
  // Encode MOV r1, #10
  encode_instr(arena, COIL_OP_MOV, 2);
  
  coil_operand_header_t reg_header;
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg1 = 1;
  encode_operand_data(arena, &reg1, sizeof(reg1));
  
  coil_operand_header_t imm_header;
  imm_header.type = COIL_TYPEOP_IMM;
  imm_header.value_type = COIL_VAL_U32;
  imm_header.modifier = COIL_MOD_CONST;
  encode_operand(arena, &imm_header);
  
  uint32_t val1 = 10;
  encode_operand_data(arena, &val1, sizeof(val1));
  
  // Encode MOV r2, #20
  encode_instr(arena, COIL_OP_MOV, 2);
  
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg2 = 2;
  encode_operand_data(arena, &reg2, sizeof(reg2));
  
  imm_header.type = COIL_TYPEOP_IMM;
  imm_header.value_type = COIL_VAL_U32;
  imm_header.modifier = COIL_MOD_CONST;
  encode_operand(arena, &imm_header);
  
  uint32_t val2 = 20;
  encode_operand_data(arena, &val2, sizeof(val2));
  
  // Encode ADD r3, r1, r2
  encode_instr(arena, COIL_OP_ADD, 3);
  
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg3 = 3;
  encode_operand_data(arena, &reg3, sizeof(reg3));
  
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg1_arg = 1;
  encode_operand_data(arena, &reg1_arg, sizeof(reg1_arg));
  
  reg_header.type = COIL_TYPEOP_REG;
  reg_header.value_type = COIL_VAL_U32;
  reg_header.modifier = COIL_MOD_NONE;
  encode_operand(arena, &reg_header);
  
  uint32_t reg2_arg = 2;
  encode_operand_data(arena, &reg2_arg, sizeof(reg2_arg));
  
  // Calculate size of instructions
  size_t buffer_size = (uint8_t*)arena_alloc_default(arena, 0) - buffer_start;
  
  // Dump the encoded instructions
  dump_encoded_data(buffer_start, buffer_size, "Multiple instructions");
  
  // Now decode the instructions
  size_t pos = 0;
  
  // Expected values for each instruction
  struct {
    uint8_t opcode;
    uint8_t operand_count;
    uint32_t operands[3];
  } expected[3] = {
    {COIL_OP_MOV, 2, {1, 10, 0}},
    {COIL_OP_MOV, 2, {2, 20, 0}},
    {COIL_OP_ADD, 3, {3, 1, 2}}
  };
  
  // Decode and verify each instruction
  for (int i = 0; i < 3; i++) {
    // Decode instruction header
    coil_instr_t instr;
    pos = decode_instr(buffer_start, pos, &instr);
    
    char message[100];
    snprintf(message, sizeof(message), "Instruction %d: opcode should match", i+1);
    TEST_ASSERT_EQUAL_INT(expected[i].opcode, instr.opcode, message);
    
    snprintf(message, sizeof(message), "Instruction %d: operand count should match", i+1);
    TEST_ASSERT_EQUAL_INT(expected[i].operand_count, instr.operand_count, message);
    
    // Decode each operand
    for (int j = 0; j < instr.operand_count; j++) {
      coil_operand_header_t header;
      coil_offset_t offset;
      pos = decode_operand(buffer_start, pos, &header, &offset);
      
      uint32_t value = 0;
      size_t size = 0;
      pos = decode_operand_data(buffer_start, pos, &value, sizeof(value), &size, &header);
      
      char message[100];
      snprintf(message, sizeof(message), "Instruction %d: operand %d value should match", i+1, j+1);
      TEST_ASSERT_EQUAL_INT(expected[i].operands[j], value, message);
    }
  }
  
  // Free resources
  arena_destroy(arena);
}
*/

// Test error handling for decode functions
void test_decode_error_handling(void) {
  // Test with NULL parameters
  coil_instr_t instr;
  size_t pos;
  
  // Test decode_instr with NULL buffer
  pos = decode_instr(NULL, 0, &instr);
  TEST_ASSERT_EQUAL_SIZE(0, pos, "decode_instr with NULL buffer should return unchanged position");
  
  // Test decode_instr with NULL output parameter
  uint8_t buffer[10] = {0x10, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  pos = decode_instr(buffer, 5, NULL);
  TEST_ASSERT_EQUAL_SIZE(5, pos, "decode_instr with NULL output should return unchanged position");
  
  // Test decode_operand with NULL buffer
  coil_operand_header_t header;
  coil_offset_t offset;
  pos = decode_operand(NULL, 3, &header, &offset);
  TEST_ASSERT_EQUAL_SIZE(3, pos, "decode_operand with NULL buffer should return unchanged position");
  
  // Test decode_operand with NULL output parameters
  pos = decode_operand(buffer, 2, NULL, &offset);
  TEST_ASSERT_EQUAL_SIZE(2, pos, "decode_operand with NULL header should return unchanged position");
  
  pos = decode_operand(buffer, 4, &header, NULL);
  TEST_ASSERT_EQUAL_SIZE(4, pos, "decode_operand with NULL offset should return unchanged position");
  
  // Test decode_operand_data with NULL parameters
  uint32_t value;
  size_t valsize;
  
  header.value_type = COIL_VAL_U32; // Set for testing
  
  pos = decode_operand_data(NULL, 1, &value, sizeof(value), &valsize, &header);
  TEST_ASSERT_EQUAL_SIZE(1, pos, "decode_operand_data with NULL buffer should return unchanged position");
  
  pos = decode_operand_data(buffer, 2, NULL, sizeof(value), &valsize, &header);
  TEST_ASSERT_EQUAL_SIZE(2, pos, "decode_operand_data with NULL data should return unchanged position");
  
  pos = decode_operand_data(buffer, 3, &value, 0, &valsize, &header);
  TEST_ASSERT_EQUAL_SIZE(3, pos, "decode_operand_data with zero size should return unchanged position");
  
  pos = decode_operand_data(buffer, 4, &value, sizeof(value), NULL, &header);
  TEST_ASSERT_EQUAL_SIZE(4, pos, "decode_operand_data with NULL valsize should return unchanged position");
  
  pos = decode_operand_data(buffer, 5, &value, sizeof(value), &valsize, NULL);
  TEST_ASSERT_EQUAL_SIZE(5, pos, "decode_operand_data with NULL header should return unchanged position");
}

// Define test array
test_t instr_tests[] = {
  {"Encode Instruction", test_encode_instr},
  {"Encode Void Instruction", test_encode_instr_void},
  {"Encode Operand", test_encode_operand},
  {"Encode Operand with Offset", test_encode_operand_offset},
  {"Encode Operand Data", test_encode_operand_data},
  {"Value Type Sizes", test_value_type_sizes},
  // {"Complete Instruction", test_complete_instruction},
  // {"Decode with Offset", test_decode_with_offset},
  // {"Multiple Instructions", test_multiple_instructions},
  {"Decode Error Handling", test_decode_error_handling}
};

// Run instruction tests
void run_instr_tests(void) {
  run_tests(instr_tests, sizeof(instr_tests) / sizeof(instr_tests[0]));
}