#include <coil/instr.h>
#include <string.h>

// -------------------------------- Serialization -------------------------------- //

void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count) {
  if (!arena) return;
  
  uint8_t data[2];
  data[0] = op;
  data[1] = operand_count;
  arena_push_default(arena, data, sizeof(data));
}

void encode_instr_void(coil_arena_t *arena, coil_opcode_t op) {
  if (!arena) return;
  
  arena_push_default(arena, &op, sizeof(op));
}

static void __encode_imm_value(coil_arena_t *arena, coil_value_type_t type, void *data) {
  if (!arena || !data) return;
  
  // Encode values consistently in little-endian format
  switch (type) {
    case COIL_VAL_FLAG0:
    case COIL_VAL_FLAG1:
    case COIL_VAL_FLAG2:
    case COIL_VAL_FLAG3:
    case COIL_VAL_BIT:
    case COIL_VAL_I8:
    case COIL_VAL_U8:
      // Single byte values
      arena_push_default(arena, data, 1);
      break;

    case COIL_VAL_I16:
    case COIL_VAL_U16: {
      // 16-bit values in little-endian
      uint16_t value = *(uint16_t*)data;
      uint8_t bytes[2];
      bytes[0] = (uint8_t)(value & 0xFF);
      bytes[1] = (uint8_t)((value >> 8) & 0xFF);
      arena_push_default(arena, bytes, 2);
      break;
    }

    case COIL_VAL_I32:
    case COIL_VAL_U32:
    case COIL_VAL_REG: {
      // 32-bit values in little-endian
      uint32_t value = *(uint32_t*)data;
      uint8_t bytes[4];
      bytes[0] = (uint8_t)(value & 0xFF);
      bytes[1] = (uint8_t)((value >> 8) & 0xFF);
      bytes[2] = (uint8_t)((value >> 16) & 0xFF);
      bytes[3] = (uint8_t)((value >> 24) & 0xFF);
      arena_push_default(arena, bytes, 4);
      break;
    }
      
    case COIL_VAL_F32: {
      // 32-bit float - preserve bit pattern
      float value = *(float*)data;
      uint32_t bits;
      memcpy(&bits, &value, sizeof(bits));
      uint8_t bytes[4];
      bytes[0] = (uint8_t)(bits & 0xFF);
      bytes[1] = (uint8_t)((bits >> 8) & 0xFF);
      bytes[2] = (uint8_t)((bits >> 16) & 0xFF);
      bytes[3] = (uint8_t)((bits >> 24) & 0xFF);
      arena_push_default(arena, bytes, 4);
      break;
    }

    case COIL_VAL_I64:
    case COIL_VAL_U64:
    case COIL_VAL_PTR:
    case COIL_VAL_SIZE:
    case COIL_VAL_SSIZE:
    case COIL_VAL_VAR:
    case COIL_VAL_SYM:
    case COIL_VAL_EXP:
    case COIL_VAL_STR: {
      // 64-bit values in little-endian
      uint64_t value = *(uint64_t*)data;
      uint8_t bytes[8];
      bytes[0] = (uint8_t)(value & 0xFF);
      bytes[1] = (uint8_t)((value >> 8) & 0xFF);
      bytes[2] = (uint8_t)((value >> 16) & 0xFF);
      bytes[3] = (uint8_t)((value >> 24) & 0xFF);
      bytes[4] = (uint8_t)((value >> 32) & 0xFF);
      bytes[5] = (uint8_t)((value >> 40) & 0xFF);
      bytes[6] = (uint8_t)((value >> 48) & 0xFF);
      bytes[7] = (uint8_t)((value >> 56) & 0xFF);
      arena_push_default(arena, bytes, 8);
      break;
    }
      
    case COIL_VAL_F64: {
      // 64-bit double - preserve bit pattern
      double value = *(double*)data;
      uint64_t bits;
      memcpy(&bits, &value, sizeof(bits));
      uint8_t bytes[8];
      bytes[0] = (uint8_t)(bits & 0xFF);
      bytes[1] = (uint8_t)((bits >> 8) & 0xFF);
      bytes[2] = (uint8_t)((bits >> 16) & 0xFF);
      bytes[3] = (uint8_t)((bits >> 24) & 0xFF);
      bytes[4] = (uint8_t)((bits >> 32) & 0xFF);
      bytes[5] = (uint8_t)((bits >> 40) & 0xFF);
      bytes[6] = (uint8_t)((bits >> 48) & 0xFF);
      bytes[7] = (uint8_t)((bits >> 56) & 0xFF);
      arena_push_default(arena, bytes, 8);
      break;
    }
      
    default:
      // For unknown types, do nothing
      break;
  }
}

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

void encode_operand_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t ref) {
  if (!arena) return;
  
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  arena_push_default(arena, header, sizeof(header));
  
  // Encode in little-endian format
  uint8_t bytes[8];
  bytes[0] = (uint8_t)(ref & 0xFF);
  bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  bytes[4] = (uint8_t)((ref >> 32) & 0xFF);
  bytes[5] = (uint8_t)((ref >> 40) & 0xFF);
  bytes[6] = (uint8_t)((ref >> 48) & 0xFF);
  bytes[7] = (uint8_t)((ref >> 56) & 0xFF);
  arena_push_default(arena, bytes, 8);
}

void encode_operand_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint32_t ref) {
  if (!arena) return;
  
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  arena_push_default(arena, header, sizeof(header));
  
  // Encode in little-endian format
  uint8_t bytes[4];
  bytes[0] = (uint8_t)(ref & 0xFF);
  bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  arena_push_default(arena, bytes, 4);
}

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
  
  // Encode displacement in little-endian
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  arena_push_default(arena, disp_bytes, 8);
  
  // Encode index in little-endian
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  arena_push_default(arena, index_bytes, 8);
  
  // Encode scale in little-endian
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  arena_push_default(arena, scale_bytes, 8);
  
  // Encode the immediate value
  __encode_imm_value(arena, type, data);
}

void encode_operand_off_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint64_t ref) {
  if (!arena) return;
  
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  arena_push_default(arena, header, sizeof(header));
  
  // Encode displacement in little-endian
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  arena_push_default(arena, disp_bytes, 8);
  
  // Encode index in little-endian
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  arena_push_default(arena, index_bytes, 8);
  
  // Encode scale in little-endian
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  arena_push_default(arena, scale_bytes, 8);
  
  // Encode reference in little-endian
  uint8_t ref_bytes[8];
  ref_bytes[0] = (uint8_t)(ref & 0xFF);
  ref_bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  ref_bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  ref_bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  ref_bytes[4] = (uint8_t)((ref >> 32) & 0xFF);
  ref_bytes[5] = (uint8_t)((ref >> 40) & 0xFF);
  ref_bytes[6] = (uint8_t)((ref >> 48) & 0xFF);
  ref_bytes[7] = (uint8_t)((ref >> 56) & 0xFF);
  arena_push_default(arena, ref_bytes, 8);
}

void encode_operand_off_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint32_t ref) {
  if (!arena) return;
  
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  arena_push_default(arena, header, sizeof(header));
  
  // Encode displacement in little-endian
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  arena_push_default(arena, disp_bytes, 8);
  
  // Encode index in little-endian
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  arena_push_default(arena, index_bytes, 8);
  
  // Encode scale in little-endian
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  arena_push_default(arena, scale_bytes, 8);
  
  // Encode reference in little-endian
  uint8_t ref_bytes[4];
  ref_bytes[0] = (uint8_t)(ref & 0xFF);
  ref_bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  ref_bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  ref_bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  arena_push_default(arena, ref_bytes, 4);
}

// -------------------------------- De-Serialization -------------------------------- //