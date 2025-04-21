/**
 * @file test_instr.c
 * @brief Tests for the instruction serialization system
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include <coil/instr.h>
#include <coil/arena.h>

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
  assert_int_equal(arena_used(arena), 2);
  
  /* Get the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
  
  /* Check that the data matches expectations */
  assert_int_equal(data[0], COIL_OP_NOP);
  assert_int_equal(data[1], 0);
  
  /* Encode an ADD instruction with 3 operands */
  arena_reset(arena);
  encode_instr(arena, COIL_OP_ADD, 3);
  
  data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
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
  assert_int_equal(arena_used(arena), 1);
  
  /* Get the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
  
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
  assert_int_equal(arena_used(arena), 7);
  
  /* Get the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
  
  /* Check that the data matches expectations */
  assert_int_equal(data[0], COIL_TYPEOP_IMM);
  assert_int_equal(data[1], COIL_VAL_U32);
  assert_int_equal(data[2], COIL_MOD_NONE);
  
  /* Check the value (considering potential endianness issues) */
  uint32_t decoded;
  memcpy(&decoded, &data[3], sizeof(uint32_t));
  assert_int_equal(decoded, value);
  
  /* Test with a modifier */
  arena_reset(arena);
  encode_operand_imm(arena, COIL_VAL_U32, COIL_MOD_CONST, &value);
  
  data = arena_alloc(arena, 0, 1);
  assert_int_equal(data[2], COIL_MOD_CONST);
  
  /* Test with 8-bit value */
  arena_reset(arena);
  uint8_t small_value = 42;
  encode_operand_imm(arena, COIL_VAL_U8, COIL_MOD_NONE, &small_value);
  
  /* Size: 1 (optype) + 1 (valtype) + 1 (mod) + 1 (value) */
  assert_int_equal(arena_used(arena), 4);
  
  data = arena_alloc(arena, 0, 1);
  assert_int_equal(data[3], small_value);
  
  /* Test with 64-bit value */
  arena_reset(arena);
  uint64_t large_value = 0x1234567890ABCDEF;
  encode_operand_imm(arena, COIL_VAL_U64, COIL_MOD_NONE, &large_value);
  
  /* Size: 1 (optype) + 1 (valtype) + 1 (mod) + 8 (value) */
  assert_int_equal(arena_used(arena), 11);
  
  data = arena_alloc(arena, 0, 1);
  uint64_t decoded_large;
  memcpy(&decoded_large, &data[3], sizeof(uint64_t));
  assert_int_equal(decoded_large, large_value);
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
  assert_int_equal(arena_used(arena), 7);
  
  /* Get the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
  
  /* Check that the data matches expectations */
  assert_int_equal(data[0], COIL_TYPEOP_REG);
  assert_int_equal(data[1], COIL_VAL_REG);
  assert_int_equal(data[2], COIL_MOD_NONE);
  
  /* Check the value */
  uint32_t decoded;
  memcpy(&decoded, &data[3], sizeof(uint32_t));
  assert_int_equal(decoded, reg);
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
  assert_int_equal(arena_used(arena), 11);
  
  /* Get the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1); /* Get pointer to start of arena */
  
  /* Check that the data matches expectations */
  assert_int_equal(data[0], COIL_TYPEOP_SYM);
  assert_int_equal(data[1], COIL_VAL_SYM);
  assert_int_equal(data[2], COIL_MOD_NONE);
  
  /* Check the value */
  uint64_t decoded;
  memcpy(&decoded, &data[3], sizeof(uint64_t));
  assert_int_equal(decoded, sym_ref);
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
  assert_int_equal(arena_used(arena), 32);
  
  /* Check key parts of the encoded data */
  uint8_t *data = arena_alloc(arena, 0, 1);
  assert_int_equal(data[0], COIL_TYPEOP_OFF);
  assert_int_equal(data[1], COIL_TYPEOP_IMM);
  
  /* Test with offset + u64 */
  arena_reset(arena);
  uint64_t ref = 0x1234567890ABCDEF;
  encode_operand_off_u64(arena, COIL_TYPEOP_SYM, COIL_VAL_SYM, COIL_MOD_NONE, 10, 2, 4, ref);
  
  /* Size: 1 (offtype) + 1 (optype) + 1 (valtype) + 1 (mod) + 
           8 (disp) + 8 (index) + 8 (scale) + 8 (value) */
  assert_int_equal(arena_used(arena), 36);
  
  /* Test with offset + u32 */
  arena_reset(arena);
  uint32_t reg = 5;
  encode_operand_off_u32(arena, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE, 10, 2, 4, reg);
  
  /* Size: 1 (offtype) + 1 (optype) + 1 (valtype) + 1 (mod) + 
           8 (disp) + 8 (index) + 8 (scale) + 4 (value) */
  assert_int_equal(arena_used(arena), 32);
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
  
  /* Verify the size is as expected:
   * - MOV r1, #42:  2 (instr) + 7 (reg) + 7 (imm) = 16 bytes
   * - MOV r2, #13:  2 (instr) + 7 (reg) + 7 (imm) = 16 bytes
   * - ADD r3, r1, r2: 2 (instr) + 7 (reg) + 7 (reg) + 7 (reg) = 23 bytes
   * Total: 55 bytes
   */
  assert_int_equal(arena_used(arena), 55);
}

/* Main function running all tests */
int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(test_encode_instr, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_encode_instr_void, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_encode_operand_imm, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_encode_operand_u32, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_encode_operand_u64, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_encode_operand_off, setup_arena, teardown_arena),
    cmocka_unit_test_setup_teardown(test_full_instruction_sequence, setup_arena, teardown_arena),
  };
  
  return cmocka_run_group_tests(tests, NULL, NULL);
}