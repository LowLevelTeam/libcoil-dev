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

// -------------------------------- De-Serialization -------------------------------- //
void decode_instr(coil_arena_t *arena, coil_instr_t *op) {
  // TODO
}

void decode_operand(coil_arena_t *arena, coil_operand_header_t *header, coil_offset_t *offset) {
  offset->disp  = 0;
  offset->index = 0;
  offset->scale = 0;

  // TODO
  
  // Get operand type
  
  // if operand type is COIL_TYPEOP_OFF then get another type and put that in header then expect offset information

  // Get value type and type modifier

  // If offset information get offset information
}

void decode_operand_data(coil_arena_t *arena, void *data, size_t datasize, size_t *valsize, coil_operand_header_t *header) {
  // TODO
}