#include <coil/instr.h>
#include <string.h>

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Encode an instruction header with operand count
*/
void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count) {
  uint8_t data[2];
  data[0] = op;
  data[1] = operand_count;
  arena_push_default(arena, data, sizeof(data));
}

/**
* @brief Encode an instruction header without operand count
*/
void encode_instr_void(coil_arena_t *arena, coil_opcode_t op) {
  arena_push_default(arena, &op, sizeof(op));
}

/**
* @brief Helper function to encode immediate values of different sizes
*/
static void __encode_imm_value(coil_arena_t *arena, coil_value_type_t type, void *data) {
  if (!data) return;
  
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
      
    default:
      // For unknown types, do nothing
      break;
  }
}

/**
* @brief Encode an instruction operand to an immediate value
*/
void encode_operand_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, void *data) {
  if (!arena || !data) return;
  
  uint8_t header[3];
  coil_operand_type_t optype = COIL_TYPEOP_IMM;
  
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  arena_push_default(arena, header, sizeof(header));
  __encode_imm_value(arena, type, data);
}

/**
* @brief Encode an instruction operand to a u64 reference
*/
void encode_operand_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t ref) {
  if (!arena) return;
  
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  arena_push_default(arena, header, sizeof(header));
  arena_push_default(arena, &ref, sizeof(ref));
}

/**
* @brief Encode an instruction operand to a u32 reference
*/
void encode_operand_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint32_t ref) {
  if (!arena) return;
  
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  arena_push_default(arena, header, sizeof(header));
  arena_push_default(arena, &ref, sizeof(ref));
}

/**
* @brief Encode an instruction operand to an immediate value with offset
*/
void encode_operand_off_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, void *data) {
  if (!arena || !data) return;
  
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  coil_operand_type_t optype = COIL_TYPEOP_IMM;

  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  arena_push_default(arena, header, sizeof(header));
  arena_push_default(arena, &disp, sizeof(disp));
  arena_push_default(arena, &index, sizeof(index));
  arena_push_default(arena, &scale, sizeof(scale));
  
  __encode_imm_value(arena, type, data);
}

/**
* @brief Encode an instruction operand to a u64 reference with offset
*/
void encode_operand_off_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint64_t ref) {
  if (!arena) return;
  
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  arena_push_default(arena, header, sizeof(header));
  arena_push_default(arena, &disp, sizeof(disp));
  arena_push_default(arena, &index, sizeof(index));
  arena_push_default(arena, &scale, sizeof(scale));
  arena_push_default(arena, &ref, sizeof(ref));
}

/**
* @brief Encode an instruction operand to a u32 reference with offset
*/
void encode_operand_off_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint32_t ref) {
  if (!arena) return;
  
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  arena_push_default(arena, header, sizeof(header));
  arena_push_default(arena, &disp, sizeof(disp));
  arena_push_default(arena, &index, sizeof(index));
  arena_push_default(arena, &scale, sizeof(scale));
  arena_push_default(arena, &ref, sizeof(ref));
}

// -------------------------------- De-Serialization -------------------------------- //