/**
* @file instr.h
* @brief Define the standard instruction interface for serialization and deserialization
* 
* This file provides functions for encoding and decoding COIL instructions and operands,
* supporting the binary format of the COIL object file instruction sections.
*/

#ifndef __COIL_INCLUDE_GUARD_INSTR_H
#define __COIL_INCLUDE_GUARD_INSTR_H

#include <coil/section.h>
#include <coil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------- Structures -------------------------------- //

/**
* @brief Instruction Header
* 
* Basic structure for COIL instructions, consisting of opcode and operand count
*/
typedef struct coil_instr_s {
  coil_u8_t opcode;       ///< Operation code
  coil_u8_t condition; ///< Number of operands
} coil_instr_t;

/**
* @brief Offset operand header structure
* 
* Used for complex addressing with displacement and scaling
*/
typedef struct coil_offset_s {
  coil_u64_t disp;   ///< Displacement value
  coil_u64_t index;  ///< Index register or value
  coil_u64_t scale;  ///< Scale factor
  // Effective address = (scale * index) + disp
} coil_offset_t;

/**
* @brief Operand header structure
* 
* Contains type information about an instruction operand
*/
typedef struct coil_operand_header_s {
  coil_u8_t type;       ///< Operand type (COIL_TYPEOP_*)
  coil_u8_t value_type; ///< Value type (COIL_VAL_*)
  coil_u8_t modifier;   ///< Modifiers (COIL_MOD_*)
} coil_operand_header_t;

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Encode an instruction header with operand count
*
* @param sect Section to write the encoded instruction to
* @param op Opcode to encode
* @param condition Number of operands for this instruction
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if section is NULL
* @return coil_err_t COIL_ERR_BADSTATE if section doesn't support writing
* @return coil_err_t COIL_ERR_NOMEM if write fails due to memory allocation
*/
coil_err_t coil_instr_encode(coil_section_t *sect, coil_opcode_t op, coil_u8_t condition);

/**
* @brief Encode operand header without offset
* 
* @param sect Section to write the encoded operand to
* @param type Operand type (COIL_TYPEOP_*)
* @param value_type The operand's value type (COIL_VAL_*)
* @param modifier Bitmap of any or no type modifiers (COIL_MOD_*)
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if section is NULL
* @return coil_err_t COIL_ERR_BADSTATE if section doesn't support writing
* @return coil_err_t COIL_ERR_NOMEM if write fails due to memory allocation
*/
coil_err_t coil_operand_encode(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, coil_u8_t modifier);

/**
* @brief Encode operand header with offset 
* 
* @param sect Section to write the encoded operand to
* @param type Operand type (COIL_TYPEOP_*)
* @param value_type The operand's value type (COIL_VAL_*)
* @param modifier Bitmap of any or no type modifiers (COIL_MOD_*)
* @param offset Offset data to encode
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if section or offset is NULL
* @return coil_err_t COIL_ERR_BADSTATE if section doesn't support writing
* @return coil_err_t COIL_ERR_NOMEM if write fails due to memory allocation
*/
coil_err_t coil_operand_encode_off(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, 
                                   coil_u8_t modifier, coil_offset_t *offset);

/**
* @brief Encode operand data
* 
* @param sect Section to write the encoded data to
* @param data Pointer to data to encode
* @param datasize Size of data to encode
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if section or data is NULL
* @return coil_err_t COIL_ERR_BADSTATE if section doesn't support writing
* @return coil_err_t COIL_ERR_NOMEM if write fails due to memory allocation
*/
coil_err_t coil_operand_encode_data(coil_section_t *sect, void *data, coil_size_t datasize);

// -------------------------------- De-Serialization -------------------------------- //

/**
* @brief Decode an instruction header 
*
* @param sect Section containing the encoded instruction
* @param pos Current position in the section
* @param op Pointer to store the decoded instruction
* 
* @return coil_size_t Updated position after decoding
* @return 0 on error (check coil_error_get_last() for details)
*/
coil_size_t coil_instr_decode(coil_section_t *sect, coil_size_t pos, coil_instr_t *op);

/**
* @brief Decode operand header with or without offset addition
*
* If operand does not have offset parameters, the three offset values will be set to zero
*
* @param sect Section containing the encoded operand
* @param pos Current position in the section
* @param header Pointer to store the decoded operand header
* @param offset Pointer to store the decoded offset (if present)
* 
* @return coil_size_t Updated position after decoding
* @return 0 on error (check coil_error_get_last() for details)
*/
coil_size_t coil_operand_decode(coil_section_t *sect, coil_size_t pos, 
                               coil_operand_header_t *header, coil_offset_t *offset);

/**
* @brief Decode operand data
*
* Header must already be decoded to determine the value type
*
* @param sect Section containing the encoded data
* @param pos Current position in the section
* @param data Pointer to buffer to store the decoded data
* @param datasize Size of the data buffer
* @param valsize Pointer to store the actual size of the value type
* @param header Operand header containing value type information
* 
* @return coil_size_t Updated position after decoding
* @return 0 on error (check coil_error_get_last() for details)
*/
coil_size_t coil_operand_decode_data(coil_section_t *sect, coil_size_t pos, 
                                    void *data, coil_size_t datasize, 
                                    coil_size_t *valsize, coil_operand_header_t *header);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_INSTR_H