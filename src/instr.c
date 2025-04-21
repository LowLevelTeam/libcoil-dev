#include <coil/instr.h>
#include <string.h>
#include <stdio.h>  // For debugging

// -------------------------------- Serialization -------------------------------- //

void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count) {
  if (!arena) return;
  
  // Create a temporary buffer for the instruction
  uint8_t data[2];
  data[0] = op;
  data[1] = operand_count;
  
  // Use arena_push to ensure data is properly copied to the arena
  if (!arena_push_default(arena, data, sizeof(data))) {
    fprintf(stderr, "DEBUG: Failed to push instruction with opcode %d\n", op);
  }
}

void encode_instr_void(coil_arena_t *arena, coil_opcode_t op) {
  if (!arena) return;
  
  // For void instructions, just push the opcode
  if (!arena_push_default(arena, &op, sizeof(op))) {
    fprintf(stderr, "DEBUG: Failed to push void instruction with opcode %d\n", op);
  }
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
    case COIL_VAL_U8: {
      // Single byte values - directly copy the byte
      uint8_t byte_val = *(uint8_t*)data;
      arena_push_default(arena, &byte_val, 1);
      break;
    }

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
      fprintf(stderr, "DEBUG: Unknown value type: %d\n", type);
      break;
  }
}

void encode_operand_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, void *data) {
  if (!arena || !data) return;
  
  // Create a temporary buffer for the header
  uint8_t header[3];
  coil_operand_type_t optype = COIL_TYPEOP_IMM;
  
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push operand header\n");
    return;
  }
  
  // Then encode the immediate value
  __encode_imm_value(arena, type, data);
}

void encode_operand_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t ref) {
  if (!arena) return;
  
  // Create a temporary buffer for the header
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push u64 operand header\n");
    return;
  }
  
  // Create a temporary buffer for the value
  uint8_t bytes[8];
  bytes[0] = (uint8_t)(ref & 0xFF);
  bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  bytes[4] = (uint8_t)((ref >> 32) & 0xFF);
  bytes[5] = (uint8_t)((ref >> 40) & 0xFF);
  bytes[6] = (uint8_t)((ref >> 48) & 0xFF);
  bytes[7] = (uint8_t)((ref >> 56) & 0xFF);
  
  // Push the value
  arena_push_default(arena, bytes, sizeof(bytes));
}

void encode_operand_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint32_t ref) {
  if (!arena) return;
  
  // Create a temporary buffer for the header
  uint8_t header[3];
  header[0] = optype;
  header[1] = type;
  header[2] = mod;
  
  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push u32 operand header\n");
    return;
  }
  
  // Create a temporary buffer for the value
  uint8_t bytes[4];
  bytes[0] = (uint8_t)(ref & 0xFF);
  bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  
  // Push the value
  arena_push_default(arena, bytes, sizeof(bytes));
}

void encode_operand_off_imm(coil_arena_t *arena, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, void *data) {
  if (!arena || !data) return;
  
  // Create a temporary buffer for the header
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  coil_operand_type_t optype = COIL_TYPEOP_IMM;

  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push offset imm header\n");
    return;
  }
  
  // Create temporary buffers for the offset components
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  
  // Push the displacement
  arena_push_default(arena, disp_bytes, sizeof(disp_bytes));
  
  // Create temporary buffer for index
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  
  // Push the index
  arena_push_default(arena, index_bytes, sizeof(index_bytes));
  
  // Create temporary buffer for scale
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  
  // Push the scale
  arena_push_default(arena, scale_bytes, sizeof(scale_bytes));
  
  // Finally, encode the immediate value
  __encode_imm_value(arena, type, data);
}

void encode_operand_off_u64(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint64_t ref) {
  if (!arena) return;
  
  // Create a temporary buffer for the header
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push offset u64 header\n");
    return;
  }
  
  // Create temporary buffers for the offset components
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  
  // Push the displacement
  arena_push_default(arena, disp_bytes, sizeof(disp_bytes));
  
  // Create temporary buffer for index
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  
  // Push the index
  arena_push_default(arena, index_bytes, sizeof(index_bytes));
  
  // Create temporary buffer for scale
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  
  // Push the scale
  arena_push_default(arena, scale_bytes, sizeof(scale_bytes));
  
  // Create a buffer for the value
  uint8_t ref_bytes[8];
  ref_bytes[0] = (uint8_t)(ref & 0xFF);
  ref_bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  ref_bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  ref_bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  ref_bytes[4] = (uint8_t)((ref >> 32) & 0xFF);
  ref_bytes[5] = (uint8_t)((ref >> 40) & 0xFF);
  ref_bytes[6] = (uint8_t)((ref >> 48) & 0xFF);
  ref_bytes[7] = (uint8_t)((ref >> 56) & 0xFF);
  
  // Push the reference
  arena_push_default(arena, ref_bytes, sizeof(ref_bytes));
}

void encode_operand_off_u32(coil_arena_t *arena, coil_operand_type_t optype, coil_value_type_t type, coil_modifier_t mod, uint64_t disp, uint64_t index, uint64_t scale, uint32_t ref) {
  if (!arena) return;
  
  // Create a temporary buffer for the header
  uint8_t header[4];
  coil_operand_type_t offtype = COIL_TYPEOP_OFF;
  
  header[0] = offtype;
  header[1] = optype;
  header[2] = type;
  header[3] = mod;

  // Push the header first
  if (!arena_push_default(arena, header, sizeof(header))) {
    fprintf(stderr, "DEBUG: Failed to push offset u32 header\n");
    return;
  }
  
  // Create temporary buffers for the offset components
  uint8_t disp_bytes[8];
  disp_bytes[0] = (uint8_t)(disp & 0xFF);
  disp_bytes[1] = (uint8_t)((disp >> 8) & 0xFF);
  disp_bytes[2] = (uint8_t)((disp >> 16) & 0xFF);
  disp_bytes[3] = (uint8_t)((disp >> 24) & 0xFF);
  disp_bytes[4] = (uint8_t)((disp >> 32) & 0xFF);
  disp_bytes[5] = (uint8_t)((disp >> 40) & 0xFF);
  disp_bytes[6] = (uint8_t)((disp >> 48) & 0xFF);
  disp_bytes[7] = (uint8_t)((disp >> 56) & 0xFF);
  
  // Push the displacement
  arena_push_default(arena, disp_bytes, sizeof(disp_bytes));
  
  // Create temporary buffer for index
  uint8_t index_bytes[8];
  index_bytes[0] = (uint8_t)(index & 0xFF);
  index_bytes[1] = (uint8_t)((index >> 8) & 0xFF);
  index_bytes[2] = (uint8_t)((index >> 16) & 0xFF);
  index_bytes[3] = (uint8_t)((index >> 24) & 0xFF);
  index_bytes[4] = (uint8_t)((index >> 32) & 0xFF);
  index_bytes[5] = (uint8_t)((index >> 40) & 0xFF);
  index_bytes[6] = (uint8_t)((index >> 48) & 0xFF);
  index_bytes[7] = (uint8_t)((index >> 56) & 0xFF);
  
  // Push the index
  arena_push_default(arena, index_bytes, sizeof(index_bytes));
  
  // Create temporary buffer for scale
  uint8_t scale_bytes[8];
  scale_bytes[0] = (uint8_t)(scale & 0xFF);
  scale_bytes[1] = (uint8_t)((scale >> 8) & 0xFF);
  scale_bytes[2] = (uint8_t)((scale >> 16) & 0xFF);
  scale_bytes[3] = (uint8_t)((scale >> 24) & 0xFF);
  scale_bytes[4] = (uint8_t)((scale >> 32) & 0xFF);
  scale_bytes[5] = (uint8_t)((scale >> 40) & 0xFF);
  scale_bytes[6] = (uint8_t)((scale >> 48) & 0xFF);
  scale_bytes[7] = (uint8_t)((scale >> 56) & 0xFF);
  
  // Push the scale
  arena_push_default(arena, scale_bytes, sizeof(scale_bytes));
  
  // Create a buffer for the value
  uint8_t ref_bytes[4];
  ref_bytes[0] = (uint8_t)(ref & 0xFF);
  ref_bytes[1] = (uint8_t)((ref >> 8) & 0xFF);
  ref_bytes[2] = (uint8_t)((ref >> 16) & 0xFF);
  ref_bytes[3] = (uint8_t)((ref >> 24) & 0xFF);
  
  // Push the reference
  arena_push_default(arena, ref_bytes, sizeof(ref_bytes));
}

// -------------------------------- De-Serialization -------------------------------- //

/**
 * Helper function to determine if an opcode uses operand count
 */
static int is_void_instruction(coil_opcode_t code) {
  // Instructions that don't have operands
  switch (code) {
    case COIL_OP_NOP:
    case COIL_OP_RET:
    // Add any other void instructions here
      return 1;
    default:
      return 0;
  }
}

coil_opcode_t decode_opcode(const void* data) {
  if (!data) return 0;
  
  // First byte is always the opcode
  const uint8_t* bytes = (const uint8_t*)data;
  return bytes[0];
}

uint8_t decode_operand_count(const void* data) {
  if (!data) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_opcode_t op = bytes[0];
  
  // Void instructions don't have operand count
  if (is_void_instruction(op)) {
    return 0;
  }
  
  // Return the operand count (second byte)
  return bytes[1];
}

int has_operand_count(const void* data, coil_opcode_t code) {
  if (!data) return 0;
  
  // If code not provided, extract it
  if (code == 0) {
    code = decode_opcode(data);
  }
  
  // Check if it's a void instruction
  return !is_void_instruction(code);
}

size_t get_instruction_size(const void* data) {
  if (!data) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_opcode_t op = bytes[0];
  
  // Void instructions are 1 byte
  if (is_void_instruction(op)) {
    return 1;
  }
  
  // Instructions with operand count are at least 2 bytes
  return 2;
}

size_t decode_operand_header(const void* data, coil_operand_header_t* header) {
  if (!data || !header) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  
  // Extract header components
  header->type = bytes[0];
  header->value_type = bytes[1];
  header->modifier = bytes[2];
  
  // Return header size
  return 3;
}

size_t decode_offset_header(const void* data, coil_offset_header_t* header) {
  if (!data || !header) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  
  // Check that this is actually an offset operand
  if (bytes[0] != COIL_TYPEOP_OFF) {
    return 0;
  }
  
  // Extract header components
  header->offset_type = bytes[0];
  header->op_type = bytes[1];
  header->value_type = bytes[2];
  header->modifier = bytes[3];
  
  // Return header size
  return 4;
}

size_t get_operand_size(const void* data) {
  if (!data) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_type_t type = bytes[0];
  
  // Base size is the header (3 bytes for normal operands, 4 for offset)
  size_t size = (type == COIL_TYPEOP_OFF) ? 4 : 3;
  
  // Add size based on operand type
  if (type == COIL_TYPEOP_OFF) {
    // Offset operands have 3 uint64 components (disp, index, scale)
    size += 3 * 8;
    
    // Then add the size of the value
    coil_value_type_t value_type = bytes[2];
    coil_operand_type_t op_type = bytes[1];
    
    // For immediate values, determine size based on value type
    if (op_type == COIL_TYPEOP_IMM) {
      switch (value_type) {
        case COIL_VAL_I8:
        case COIL_VAL_U8:
        case COIL_VAL_BIT:
        case COIL_VAL_FLAG0:
        case COIL_VAL_FLAG1:
        case COIL_VAL_FLAG2:
        case COIL_VAL_FLAG3:
          size += 1;
          break;
        case COIL_VAL_I16:
        case COIL_VAL_U16:
          size += 2;
          break;
        case COIL_VAL_I32:
        case COIL_VAL_U32:
        case COIL_VAL_F32:
        case COIL_VAL_REG:
          size += 4;
          break;
        case COIL_VAL_I64:
        case COIL_VAL_U64:
        case COIL_VAL_F64:
        case COIL_VAL_PTR:
        case COIL_VAL_SIZE:
        case COIL_VAL_SSIZE:
        case COIL_VAL_VAR:
        case COIL_VAL_SYM:
        case COIL_VAL_EXP:
        case COIL_VAL_STR:
          size += 8;
          break;
        default:
          // Unknown value type, no additional size
          break;
      }
    } else if (op_type == COIL_TYPEOP_REG) {
      // Register operands are always 4 bytes (u32)
      size += 4;
    } else {
      // Other references are 8 bytes (u64)
      size += 8;
    }
  } else {
    // For normal operands, determine size based on type and value type
    coil_value_type_t value_type = bytes[1];
    
    if (type == COIL_TYPEOP_IMM) {
      // For immediate values, determine size based on value type
      switch (value_type) {
        case COIL_VAL_I8:
        case COIL_VAL_U8:
        case COIL_VAL_BIT:
        case COIL_VAL_FLAG0:
        case COIL_VAL_FLAG1:
        case COIL_VAL_FLAG2:
        case COIL_VAL_FLAG3:
          size += 1;
          break;
        case COIL_VAL_I16:
        case COIL_VAL_U16:
          size += 2;
          break;
        case COIL_VAL_I32:
        case COIL_VAL_U32:
        case COIL_VAL_F32:
        case COIL_VAL_REG:
          size += 4;
          break;
        case COIL_VAL_I64:
        case COIL_VAL_U64:
        case COIL_VAL_F64:
        case COIL_VAL_PTR:
        case COIL_VAL_SIZE:
        case COIL_VAL_SSIZE:
        case COIL_VAL_VAR:
        case COIL_VAL_SYM:
        case COIL_VAL_EXP:
        case COIL_VAL_STR:
          size += 8;
          break;
        default:
          // Unknown value type, no additional size
          break;
      }
    } else if (type == COIL_TYPEOP_REG) {
      // Register operands are always 4 bytes (u32)
      size += 4;
    } else {
      // Other references are 8 bytes (u64)
      size += 8;
    }
  }
  
  return size;
}

const void* get_operand_value_ptr(const void* data) {
  if (!data) return NULL;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_type_t type = bytes[0];
  
  // For normal operands, the value is after the 3-byte header
  if (type != COIL_TYPEOP_OFF) {
    return bytes + 3;
  }
  
  // For offset operands, the value is after the header and offset components
  // Header (4) + disp (8) + index (8) + scale (8) = 28 bytes
  return bytes + 28;
}

int decode_operand_u8(const void* data, uint8_t* value) {
  if (!data || !value) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_header_t header;
  
  // Get header size
  size_t header_size = decode_operand_header(data, &header);
  if (header_size == 0) return 0;
  
  // Check if this is a compatible type
  if (header.type != COIL_TYPEOP_IMM) return 0;
  if (header.value_type != COIL_VAL_U8 && header.value_type != COIL_VAL_I8) return 0;
  
  // Value is immediately after the header
  *value = bytes[header_size];
  return 1;
}

int decode_operand_u16(const void* data, uint16_t* value) {
  if (!data || !value) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_header_t header;
  
  // Get header size
  size_t header_size = decode_operand_header(data, &header);
  if (header_size == 0) return 0;
  
  // Check if this is a compatible type
  if (header.type != COIL_TYPEOP_IMM) return 0;
  if (header.value_type != COIL_VAL_U16 && header.value_type != COIL_VAL_I16) return 0;
  
  // Value is immediately after the header, in little-endian
  *value = (uint16_t)bytes[header_size] | ((uint16_t)bytes[header_size + 1] << 8);
  return 1;
}

int decode_operand_u32(const void* data, uint32_t* value) {
  if (!data || !value) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_header_t header;
  
  // Get header size
  size_t header_size = decode_operand_header(data, &header);
  if (header_size == 0) return 0;
  
  // For register operands
  if (header.type == COIL_TYPEOP_REG && header.value_type == COIL_VAL_REG) {
    // Extract register reference from the 4 bytes after the header
    *value = (uint32_t)bytes[header_size] | 
             ((uint32_t)bytes[header_size + 1] << 8) | 
             ((uint32_t)bytes[header_size + 2] << 16) | 
             ((uint32_t)bytes[header_size + 3] << 24);
    return 1;
  }
  
  // For immediate values
  if (header.type == COIL_TYPEOP_IMM) {
    if (header.value_type == COIL_VAL_U32 || header.value_type == COIL_VAL_I32) {
      // Extract 32-bit value from the 4 bytes after the header
      *value = (uint32_t)bytes[header_size] | 
               ((uint32_t)bytes[header_size + 1] << 8) | 
               ((uint32_t)bytes[header_size + 2] << 16) | 
               ((uint32_t)bytes[header_size + 3] << 24);
      return 1;
    }
    // Handle smaller integer types too
    else if (header.value_type == COIL_VAL_U8 || header.value_type == COIL_VAL_I8) {
      *value = (uint32_t)bytes[header_size];
      return 1;
    }
    else if (header.value_type == COIL_VAL_U16 || header.value_type == COIL_VAL_I16) {
      *value = (uint32_t)bytes[header_size] | ((uint32_t)bytes[header_size + 1] << 8);
      return 1;
    }
  }
  
  return 0;
}

int decode_operand_u64(const void* data, uint64_t* value) {
  if (!data || !value) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_operand_header_t header;
  
  // Get header size
  size_t header_size = decode_operand_header(data, &header);
  if (header_size == 0) return 0;
  
  // For reference types (variable, symbol, expression)
  if ((header.type == COIL_TYPEOP_VAR && header.value_type == COIL_VAL_VAR) ||
      (header.type == COIL_TYPEOP_SYM && header.value_type == COIL_VAL_SYM) ||
      (header.type == COIL_TYPEOP_EXP && header.value_type == COIL_VAL_EXP)) {
    // Extract the 64-bit reference
    *value = (uint64_t)bytes[header_size] | 
             ((uint64_t)bytes[header_size + 1] << 8) | 
             ((uint64_t)bytes[header_size + 2] << 16) | 
             ((uint64_t)bytes[header_size + 3] << 24) |
             ((uint64_t)bytes[header_size + 4] << 32) |
             ((uint64_t)bytes[header_size + 5] << 40) |
             ((uint64_t)bytes[header_size + 6] << 48) |
             ((uint64_t)bytes[header_size + 7] << 56);
    return 1;
  }
  
  // For immediate values
  if (header.type == COIL_TYPEOP_IMM) {
    if (header.value_type == COIL_VAL_U64 || header.value_type == COIL_VAL_I64 ||
        header.value_type == COIL_VAL_PTR || header.value_type == COIL_VAL_SIZE ||
        header.value_type == COIL_VAL_SSIZE || header.value_type == COIL_VAL_STR) {
      // Extract 64-bit value
      *value = (uint64_t)bytes[header_size] | 
               ((uint64_t)bytes[header_size + 1] << 8) | 
               ((uint64_t)bytes[header_size + 2] << 16) | 
               ((uint64_t)bytes[header_size + 3] << 24) |
               ((uint64_t)bytes[header_size + 4] << 32) |
               ((uint64_t)bytes[header_size + 5] << 40) |
               ((uint64_t)bytes[header_size + 6] << 48) |
               ((uint64_t)bytes[header_size + 7] << 56);
      return 1;
    }
    // Handle smaller integer types too
    else if (header.value_type == COIL_VAL_U8 || header.value_type == COIL_VAL_I8) {
      *value = (uint64_t)bytes[header_size];
      return 1;
    }
    else if (header.value_type == COIL_VAL_U16 || header.value_type == COIL_VAL_I16) {
      *value = (uint64_t)bytes[header_size] | ((uint64_t)bytes[header_size + 1] << 8);
      return 1;
    }
    else if (header.value_type == COIL_VAL_U32 || header.value_type == COIL_VAL_I32) {
      *value = (uint64_t)bytes[header_size] | 
               ((uint64_t)bytes[header_size + 1] << 8) | 
               ((uint64_t)bytes[header_size + 2] << 16) | 
               ((uint64_t)bytes[header_size + 3] << 24);
      return 1;
    }
  }
  
  return 0;
}

const void* get_next_operand(const void* data) {
  if (!data) return NULL;
  
  // Get the size of the current operand
  size_t size = get_operand_size(data);
  if (size == 0) return NULL;
  
  // Return pointer to the next operand
  return (const uint8_t*)data + size;
}

const void* get_first_operand(const void* data) {
  if (!data) return NULL;
  
  const uint8_t* bytes = (const uint8_t*)data;
  coil_opcode_t op = bytes[0];
  
  // Void instructions don't have operands
  if (is_void_instruction(op)) {
    return NULL;
  }
  
  // For instructions with operands, they start after the header
  return bytes + 2;
}

int decode_offset_displacement(const void* data, uint64_t* disp) {
  if (!data || !disp) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  
  // Check if this is an offset operand
  if (bytes[0] != COIL_TYPEOP_OFF) return 0;
  
  // Displacement is the 8 bytes after the 4-byte header
  *disp = (uint64_t)bytes[4] | 
          ((uint64_t)bytes[5] << 8) | 
          ((uint64_t)bytes[6] << 16) | 
          ((uint64_t)bytes[7] << 24) |
          ((uint64_t)bytes[8] << 32) |
          ((uint64_t)bytes[9] << 40) |
          ((uint64_t)bytes[10] << 48) |
          ((uint64_t)bytes[11] << 56);
  
  return 1;
}

int decode_offset_index(const void* data, uint64_t* index) {
  if (!data || !index) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  
  // Check if this is an offset operand
  if (bytes[0] != COIL_TYPEOP_OFF) return 0;
  
  // Index is the 8 bytes after the 4-byte header and 8-byte displacement
  *index = (uint64_t)bytes[12] | 
           ((uint64_t)bytes[13] << 8) | 
           ((uint64_t)bytes[14] << 16) | 
           ((uint64_t)bytes[15] << 24) |
           ((uint64_t)bytes[16] << 32) |
           ((uint64_t)bytes[17] << 40) |
           ((uint64_t)bytes[18] << 48) |
           ((uint64_t)bytes[19] << 56);
  
  return 1;
}

int decode_offset_scale(const void* data, uint64_t* scale) {
  if (!data || !scale) return 0;
  
  const uint8_t* bytes = (const uint8_t*)data;
  
  // Check if this is an offset operand
  if (bytes[0] != COIL_TYPEOP_OFF) return 0;
  
  // Scale is the 8 bytes after the 4-byte header, 8-byte displacement, and 8-byte index
  *scale = (uint64_t)bytes[20] | 
           ((uint64_t)bytes[21] << 8) | 
           ((uint64_t)bytes[22] << 16) | 
           ((uint64_t)bytes[23] << 24) |
           ((uint64_t)bytes[24] << 32) |
           ((uint64_t)bytes[25] << 40) |
           ((uint64_t)bytes[26] << 48) |
           ((uint64_t)bytes[27] << 56);
  
  return 1;
}