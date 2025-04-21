#include <coil/instr.h>

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Encode an instruction header with operand count
*/
void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count) {
  arena_push_default(arena, &op, sizeof(op));
  arena_push_default(arena, &operand_count, sizeof(operand_count));
}

/**
* @brief Encode an instruction header without operand count
*/
void encode_instr_void(coil_arena_t *arena, coil_opcode_t op) {
  arena_push_default(arena, &op, sizeof(op));
}

void __encode_imm_value(coil_arena_t *arena, coil_value_type_t type, void *data) {
  switch (type) {
    case COIL_VAL_FLAG0:
    case COIL_VAL_FLAG1:
    case COIL_VAL_FLAG2:
    case COIL_VAL_FLAG3:
    case COIL_VAL_BIT:
    case COIL_VAL_I8:
    case COIL_VAL_U8:
      arena_push_default(arena, data, 1);
      break;

    case COIL_VAL_I16:
    case COIL_VAL_U16:
      arena_push_default(arena, data, 2);
      break;

    case COIL_VAL_REG:
    case COIL_VAL_I32:
    case COIL_VAL_U32:
    case COIL_VAL_F32:
      arena_push_default(arena, data, 4);
      break;

    case COIL_VAL_PTR:
    case COIL_VAL_SIZE:
    case COIL_VAL_SSIZE:
    case COIL_VAL_VAR:
    case COIL_VAL_SYM:
    case COIL_VAL_EXP:
    case COIL_VAL_STR:
    case COIL_VAL_I64:
    case COIL_VAL_U64:
    case COIL_VAL_F64:
      arena_push_default(arena, data, 8);
      break;
  }
}

void encode_operand_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, void *data) {
  coil_operand_type_t optype = COIL_TYPEOP_IMM;
  
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));

  __encode_imm_value(arena, type, data);
}

void encode_operand_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t ref) {
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));
  arena_push_default(arena, &ref, 8);
}

void encode_operand_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint32_t ref) {
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));
  arena_push_default(arena, &ref, 4);
}

void encode_operand_off_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, void *data) {
  coil_operand_off_type_t offtype = COIL_TYPEOP_OFF;
  coil_operand_type_t optype = COIL_TYPEOP_IMM;

  arena_push_default(arena, &offtype, sizeof(offtype));
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));

  arena_push_default(arena, &disp, 8);
  arena_push_default(arena, &index, 8);
  arena_push_default(arena, &scale, 8);

  __encode_imm_value(arena, type, data);
}

void encode_operand_off_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint64_t ref) {
  coil_operand_off_type_t offtype = COIL_TYPEOP_OFF;
  arena_push_default(arena, &offtype, sizeof(offtype));
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));
  arena_push_default(arena, &ref, 8);
}

void encode_operand_off_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint32_t ref) {
  coil_operand_off_type_t offtype = COIL_TYPEOP_OFF;
  arena_push_default(arena, &offtype, sizeof(offtype));
  arena_push_default(arena, &optype, sizeof(optype));
  arena_push_default(arena, &type, sizeof(type));
  arena_push_default(arena, &mod, sizeof(mod));
  arena_push_default(arena, &ref, 4);
}

// -------------------------------- De-Serialization -------------------------------- //
