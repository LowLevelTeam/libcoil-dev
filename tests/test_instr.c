/**
* @file test_instr.c
* @brief Test suite for instruction functionality
*
* @author Low Level Team
*/

#include <coil/instr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test macros
#define TEST_ASSERT(cond, msg) do { \
  if (!(cond)) { \
    printf("ASSERT FAILED: %s (line %d)\n", msg, __LINE__); \
    return 1; \
  } \
} while (0)

/**
* @brief Test instruction encoding
*/
static int test_instruction_encode() {
  printf("  Testing instruction encoding...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Encode a basic instruction (NOP)
  err = coil_instr_encode(&sect, COIL_OP_NOP);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding NOP should succeed");
  TEST_ASSERT(sect.size == sizeof(coil_u8_t), "Section size should match");
  TEST_ASSERT(sect.data[0] == COIL_OP_NOP, "Encoded opcode should match");
  
  // Encode an instruction with flag (conditional jump)
  err = coil_instrflag_encode(&sect, COIL_OP_BR, COIL_INSTRFLAG_EQ);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding BR should succeed");
  TEST_ASSERT(sect.data[1] == COIL_OP_BR, "Encoded opcode should match");
  TEST_ASSERT(sect.data[2] == COIL_INSTRFLAG_EQ, "Encoded flag should match");
  
  // Encode an instruction with value (DEF)
  coil_u64_t value = 0x1234567890ABCDEF;
  err = coil_instrval_encode(&sect, COIL_OP_DEF, value);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding DEF should succeed");
  
  // Check that we can get the instruction format
  coil_instrfmt_t fmt = coil_instrfmt(COIL_OP_ADD);
  TEST_ASSERT(fmt == COIL_INSTRFMT_FLAG_BINARY, "ADD should be FLAG_BINARY format");
  
  fmt = coil_instrfmt(COIL_OP_NOP);
  TEST_ASSERT(fmt == COIL_INSTRFMT_VOID, "NOP should be VOID format");
  
  fmt = coil_instrfmt(COIL_OP_DEF);
  TEST_ASSERT(fmt == COIL_INSTRFMT_VALUE, "DEF should be VALUE format");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test operand encoding
*/
static int test_operand_encode() {
  printf("  Testing operand encoding...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Encode a register operand
  err = coil_operand_encode(&sect, COIL_TYPEOP_REG, COIL_VAL_I32, COIL_MOD_NONE);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding register operand should succeed");
  TEST_ASSERT(sect.size == sizeof(coil_operand_header_t), "Section size should match");
  TEST_ASSERT(sect.data[0] == COIL_TYPEOP_REG, "Encoded type should match");
  TEST_ASSERT(sect.data[1] == COIL_VAL_I32, "Encoded value type should match");
  TEST_ASSERT(sect.data[2] == COIL_MOD_NONE, "Encoded modifier should match");
  
  // Encode an immediate operand with constant modifier
  err = coil_operand_encode(&sect, COIL_TYPEOP_IMM, COIL_VAL_U64, COIL_MOD_CONST);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding immediate operand should succeed");
  
  // Encode an operand with offset
  coil_offset_t offset;
  offset.disp = 0x1000;
  offset.index = 2;
  offset.scale = 4;
  
  err = coil_operand_encode_off(&sect, COIL_TYPEOP_OFF, COIL_VAL_PTR, COIL_MOD_NONE, &offset);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding offset operand should succeed");
  
  // Encode operand data
  coil_u32_t reg_value = 5;  // EAX or similar
  err = coil_operand_encode_data(&sect, &reg_value, sizeof(reg_value));
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding operand data should succeed");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test instruction decoding
*/
static int test_instruction_decode() {
  printf("  Testing instruction decoding...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Encode a sequence of instructions
  err = coil_instr_encode(&sect, COIL_OP_NOP);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding NOP should succeed");
  
  err = coil_instrflag_encode(&sect, COIL_OP_BR, COIL_INSTRFLAG_EQ);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding BR should succeed");
  
  coil_u64_t value = 0x1234567890ABCDEF;
  err = coil_instrval_encode(&sect, COIL_OP_DEF, value);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding DEF should succeed");
  
  // Decode the instructions
  coil_instrmem_t instr;
  coil_instrfmt_t fmt;
  coil_size_t pos = 0;
  
  // Decode NOP
  pos = coil_instr_decode(&sect, pos, &instr, &fmt);
  TEST_ASSERT(pos > 0, "Decoding should succeed");
  TEST_ASSERT(fmt == COIL_INSTRFMT_VOID, "Format should be VOID");
  TEST_ASSERT(instr.opcode == COIL_OP_NOP, "Opcode should be NOP");
  
  // Decode BR
  pos = coil_instr_decode(&sect, pos, &instr, &fmt);
  TEST_ASSERT(pos > 0, "Decoding should succeed");
  TEST_ASSERT(fmt == COIL_INSTRFMT_FLAG_UNARY, "Format should be FLAG_UNARY");
  TEST_ASSERT(instr.opcode == COIL_OP_BR, "Opcode should be BR");
  TEST_ASSERT(((coil_instrflag_t *)&instr)->flag == COIL_INSTRFLAG_EQ, "Flag should be EQ");
  
  // Decode DEF
  pos = coil_instr_decode(&sect, pos, &instr, &fmt);
  TEST_ASSERT(pos > 0, "Decoding should succeed");
  TEST_ASSERT(fmt == COIL_INSTRFMT_VALUE, "Format should be VALUE");
  TEST_ASSERT(instr.opcode == COIL_OP_DEF, "Opcode should be DEF");
  TEST_ASSERT(((coil_instrval_t *)&instr)->value == value, "Value should match");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Test operand decoding
*/
static int test_operand_decode() {
  printf("  Testing operand decoding...\n");
  
  // Initialize a section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, 1024);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Section initialization should succeed");
  
  // Encode operands
  
  // Register operand
  err = coil_operand_encode(&sect, COIL_TYPEOP_REG, COIL_VAL_I32, COIL_MOD_NONE);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding register operand should succeed");
  
  coil_u32_t reg_value = 5;  // EAX or similar
  err = coil_operand_encode_data(&sect, &reg_value, sizeof(reg_value));
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding operand data should succeed");
  
  // Offset operand
  coil_offset_t offset;
  offset.disp = 0x1000;
  offset.index = 2;
  offset.scale = 4;
  
  err = coil_operand_encode_off(&sect, 0, COIL_VAL_PTR, COIL_MOD_NONE, &offset);
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding offset operand should succeed");
  
  coil_u64_t ptr_value = 0xABCDEF0123456789;
  err = coil_operand_encode_data(&sect, &ptr_value, sizeof(ptr_value));
  TEST_ASSERT(err == COIL_ERR_GOOD, "Encoding operand data should succeed");
  
  // Decode the operands
  coil_size_t pos = 0;
  coil_operand_header_t header;
  coil_offset_t decoded_offset;
  
  // Decode register operand
  pos = coil_operand_decode(&sect, pos, &header, &decoded_offset);
  TEST_ASSERT(pos > 0, "Decoding operand should succeed");
  TEST_ASSERT(header.type == COIL_TYPEOP_REG, "Type should be REG");
  TEST_ASSERT(header.value_type == COIL_VAL_I32, "Value type should be I32");
  TEST_ASSERT(header.modifier == COIL_MOD_NONE, "Modifier should be NONE");
  
  // Decode register data
  coil_u32_t decoded_reg;
  coil_size_t valsize;
  pos = coil_operand_decode_data(&sect, pos, &decoded_reg, sizeof(decoded_reg), &valsize, &header);
  TEST_ASSERT(pos > 0, "Decoding operand data should succeed");
  TEST_ASSERT(valsize == sizeof(coil_u32_t), "Value size should match");
  TEST_ASSERT(decoded_reg == reg_value, "Register value should match");
  
  // Decode offset operand
  pos = coil_operand_decode(&sect, pos, &header, &decoded_offset);
  TEST_ASSERT(pos > 0, "Decoding operand should succeed");
  TEST_ASSERT(header.type == COIL_TYPEOP_OFF, "Type should be OFF");
  TEST_ASSERT(header.value_type == COIL_VAL_PTR, "Value type should be PTR");
  TEST_ASSERT(header.modifier == COIL_MOD_NONE, "Modifier should be NONE");
  TEST_ASSERT(decoded_offset.disp == offset.disp, "Offset displacement should match");
  TEST_ASSERT(decoded_offset.index == offset.index, "Offset index should match");
  TEST_ASSERT(decoded_offset.scale == offset.scale, "Offset scale should match");
  
  // Decode pointer data
  coil_u64_t decoded_ptr;
  pos = coil_operand_decode_data(&sect, pos, &decoded_ptr, sizeof(decoded_ptr), &valsize, &header);
  TEST_ASSERT(pos > 0, "Decoding operand data should succeed");
  TEST_ASSERT(valsize == sizeof(coil_u64_t), "Value size should match");
  TEST_ASSERT(decoded_ptr == ptr_value, "Pointer value should match");
  
  // Clean up
  coil_section_cleanup(&sect);
  
  return 0;
}

/**
* @brief Run all instruction tests
*/
int test_instr() {
  printf("\nRunning instruction tests...\n");
  
  int result = 0;
  
  // Run individual test functions
  result |= test_instruction_encode();
  result |= test_operand_encode();
  result |= test_instruction_decode();
  result |= test_operand_decode();
  
  if (result == 0) {
    printf("All instruction tests passed!\n");
  }
  
  return result;
}