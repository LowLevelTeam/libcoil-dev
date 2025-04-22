/**
* @file instr.h
* @brief Define the standard instruction interface for serialization and deserialization
*/

#ifndef __COIL_INCLUDE_GUARD_INSTR_H
#define __COIL_INCLUDE_GUARD_INSTR_H

#include <coil/section.h>
#include <coil/types.h>

// -------------------------------- Structures -------------------------------- //

/**
* @brief Instruction Header
*/
typedef struct {
  coil_u8_t opcode;
  coil_u8_t operand_count;
} coil_instr_t;

/**
* @brief Offset operand header structure
*/
typedef struct {
  coil_u64_t disp;
  coil_u64_t index;
  coil_u64_t scale;
  // (scale * index) + disp
} coil_offset_t;

/**
* @brief Operand header structure
*/
typedef struct {
  coil_u8_t type;       // Operand type
  coil_u8_t value_type; // Value type
  coil_u8_t modifier;   // Modifiers
} coil_operand_header_t;

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Encode an instruction header with operand count (should be used to encode 0 operands)
*
* @param arena Memory arena for allocation
* @param op Opcode to encode
* @param operand_count Number of operands for this instruction
*/
void encode_instr(coil_section_t *sect, coil_opcode_t op, coil_u8_t operand_count);

/**
* @brief Encode an instruction header without operand count 
*
* not for 0 operands but for instructions that NEVER take any operands
*
* @param arena Memory arena for allocation
* @param op Opcode to encode
*/
void encode_instr_void(coil_section_t *sect, coil_opcode_t op);

/**
* @brief Encode operand header with or without offset addition
* 
* @param arena Memory arena for allocation
* @param type operand type
* @param value_type the operand's value type
* @param modifier bitmap of any or no type modifiers
*/
void encode_operand(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, coil_u8_t modifier);

/**
* @brief Encode operand header with offset 
* 
* @param arena Memory arena for allocation
* @param type operand type
* @param value_type the operand's value type
* @param modifier bitmap of any or no type modifiers
* @param offset Offset data to encode
*/
void encode_operand_off(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, coil_u8_t modifier, coil_offset_t *offset);

/**
* @brief Encode operand data
* 
* @param arena Memory arena for allocation
* @param data Pointer to data to encode
* @param datasize Size of data to encode
*/
void encode_operand_data(coil_section_t *sect, void *data, coil_size_t datasize);

// -------------------------------- De-Serialization -------------------------------- //

/**
* @brief Decode an instruction header 
*
* @param buffer Pointer to the buffer containing encoded data
* @param pos Current position in the buffer
* @param op Pointer to store the decoded instruction
* @return coil_size_t Updated buffer position after decoding
*/
coil_size_t decode_instr(coil_section_t *sect, coil_size_t pos, coil_instr_t *op);

/**
* @brief Decode operand header with or without offset addition
*
* If operand does not have offset parameters the three offset values will be set to zero
*
* @param buffer Pointer to the buffer containing encoded data
* @param pos Current position in the buffer
* @param header Pointer to store the decoded operand header
* @param offset Pointer to store the decoded offset (if present)
* @return coil_size_t Updated buffer position after decoding
*/
coil_size_t decode_operand(coil_section_t *sect, coil_size_t pos, coil_operand_header_t *header, coil_offset_t *offset);

/**
* @brief Decode operand data
*
* Header must already be decoded
*
* @param buffer Pointer to the buffer containing encoded data
* @param pos Current position in the buffer
* @param data Pointer to store the decoded data
* @param datasize Size of the data buffer
* @param valsize Pointer to store the actual size of the value type
* @param header Operand header containing value type information
* @return coil_size_t Updated buffer position after decoding
*/
coil_size_t decode_operand_data(coil_section_t *sect, coil_size_t pos, void *data, coil_size_t datasize, coil_size_t *valsize, coil_operand_header_t *header);

#endif // __COIL_INCLUDE_GUARD_INSTR_H