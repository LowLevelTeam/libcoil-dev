#include <coil/instr.h>
#include <string.h>
#include <stdio.h>  // For debugging

// -------------------------------- Serialization -------------------------------- //

void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count) {
  arena_push_default(arena, &op, 1);
  arena_push_default(arena, &operand_count, 1);
}

void encode_instr_void(coil_arena_t *arena, coil_opcode_t op) {
  arena_push_default(arena, &op, 1);
}

void encode_operand(coil_arena_t *arena, coil_operand_header_t *header) {
  arena_push_default(arena, header, 3); // 3 * u8
}

void encode_operand_off(coil_arena_t *arena, coil_operand_header_t *header, coil_offset_t *offset) {
  arena_push_default(arena, header, 3); // 3 * u8
  arena_push_default(arena, offset, 24); // 3 * u64
}

void encode_operand_data(coil_arena_t *arena, void *data, size_t datasize) {
  arena_push_default(arena, data, datasize);
}

// -------------------------------- Helper Functions -------------------------------- //

/**
 * @brief Get the size of a value based on its type
 * 
 * @param value_type Value type
 * @return size_t Size of the value in bytes
 */
static size_t get_value_type_size(coil_value_type_t value_type) {
  switch (value_type) {
    case COIL_VAL_I8:
    case COIL_VAL_U8:
    case COIL_VAL_BIT:
      return 1;
    case COIL_VAL_I16:
    case COIL_VAL_U16:
      return 2;
    case COIL_VAL_I32:
    case COIL_VAL_U32:
    case COIL_VAL_F32:
      return 4;
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
      return 8;
    case COIL_VAL_REG:
      return 4;
    default:
      return 0;
  }
}

// -------------------------------- De-Serialization -------------------------------- //

size_t decode_instr(const uint8_t *buffer, size_t pos, coil_instr_t *op) {
  if (!buffer || !op) {
    return pos;
  }
  
  // Read the opcode
  op->opcode = buffer[pos++];
  
  // Read the operand count
  op->operand_count = buffer[pos++];
  
  return pos;
}

size_t decode_operand(const uint8_t *buffer, size_t pos, coil_operand_header_t *header, coil_offset_t *offset) {
  if (!buffer || !header || !offset) {
    return pos;
  }
  
  // Initialize offset to zeros
  offset->disp = 0;
  offset->index = 0;
  offset->scale = 0;
  
  // Read operand type
  header->type = buffer[pos++];
  
  // Read value type
  header->value_type = buffer[pos++];
  
  // Read modifier
  header->modifier = buffer[pos++];
  
  // If operand type is OFFSET, read the offset data
  if (header->type == COIL_TYPEOP_OFF) {
    // Read displacement (8 bytes)
    memcpy(&offset->disp, &buffer[pos], 8);
    pos += 8;
    
    // Read index (8 bytes)
    memcpy(&offset->index, &buffer[pos], 8);
    pos += 8;
    
    // Read scale (8 bytes)
    memcpy(&offset->scale, &buffer[pos], 8);
    pos += 8;
  }
  
  return pos;
}

size_t decode_operand_data(const uint8_t *buffer, size_t pos, void *data, size_t datasize, size_t *valsize, coil_operand_header_t *header) {
  if (!buffer || !data || !valsize || !header || datasize == 0) {
    return pos;
  }
  
  // Determine data size based on value type
  *valsize = get_value_type_size(header->value_type);
  
  if (*valsize == 0) {
    return pos;
  }
  
  // Read only as much as we can fit in the provided buffer
  size_t bytes_to_read = (*valsize <= datasize) ? *valsize : datasize;
  
  // Copy the data from the buffer
  memcpy(data, &buffer[pos], bytes_to_read);
  pos += bytes_to_read;
  
  return pos;
}