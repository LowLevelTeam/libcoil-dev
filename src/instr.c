#include <coil/instr.h>
#include <coil/err.h>
#include <coil/types.h>
#include <string.h>

/**
* @brief Encode an instruction header with operand count
*
* @param sect Section to write the encoded instruction to
* @param op Opcode to encode
* @param operand_count Number of operands for this instruction
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_instr_encode(coil_section_t *sect, coil_opcode_t op, coil_u8_t operand_count) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    // Prepare instruction header
    coil_instr_t instr;
    instr.opcode = op;
    instr.operand_count = operand_count;
    
    // Write instruction header to section
    coil_size_t bytes_written;
    return coil_section_write(sect, (coil_byte_t*)&instr, sizeof(instr), &bytes_written);
}

/**
* @brief Encode an instruction header without operand count 
*
* @param sect Section to write the encoded instruction to
* @param op Opcode to encode
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_instr_encode_void(coil_section_t *sect, coil_opcode_t op) {
    // Void instructions always have 0 operands
    return coil_instr_encode(sect, op, 0);
}

/**
* @brief Encode operand header without offset
* 
* @param sect Section to write the encoded operand to
* @param type Operand type
* @param value_type The operand's value type
* @param modifier Bitmap of any or no type modifiers
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_operand_encode(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, coil_u8_t modifier) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    // Prepare operand header
    coil_operand_header_t header;
    header.type = type;
    header.value_type = value_type;
    header.modifier = modifier;
    
    // Write operand header to section
    coil_size_t bytes_written;
    return coil_section_write(sect, (coil_byte_t*)&header, sizeof(header), &bytes_written);
}

/**
* @brief Encode operand header with offset 
* 
* @param sect Section to write the encoded operand to
* @param type Operand type
* @param value_type The operand's value type
* @param modifier Bitmap of any or no type modifiers
* @param offset Offset data to encode
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_operand_encode_off(coil_section_t *sect, coil_u8_t type, coil_u8_t value_type, coil_u8_t modifier, coil_offset_t *offset) {
    (void)type;
    
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (offset == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Offset pointer is NULL");
    }
    
    // Encode the operand header with type set to OFFSET
    coil_err_t err = coil_operand_encode(sect, COIL_TYPEOP_OFF, value_type, modifier);
    if (err != COIL_ERR_GOOD) {
        return err;
    }
    
    // Write offset data to section
    coil_size_t bytes_written;
    return coil_section_write(sect, (coil_byte_t*)offset, sizeof(coil_offset_t), &bytes_written);
}

/**
* @brief Encode operand data
* 
* @param sect Section to write the encoded data to
* @param data Pointer to data to encode
* @param datasize Size of data to encode
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_operand_encode_data(coil_section_t *sect, void *data, coil_size_t datasize) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (data == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Data pointer is NULL");
    }
    
    if (datasize == 0) {
        return COIL_ERROR(COIL_ERR_INVAL, "Data size is zero");
    }
    
    // Write data to section
    coil_size_t bytes_written;
    return coil_section_write(sect, (coil_byte_t*)data, datasize, &bytes_written);
}

/**
* @brief Decode an instruction header 
*
* @param sect Section containing the encoded instruction
* @param pos Current position in the section
* @param op Pointer to store the decoded instruction
* @return coil_size_t Updated position after decoding
*/
coil_size_t coil_instr_decode(coil_section_t *sect, coil_size_t pos, coil_instr_t *op) {
    // Validate parameters
    if (sect == NULL || op == NULL) {
        COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
        return 0;
    }
    
    // Check if we have enough data
    if (pos + sizeof(coil_instr_t) > sect->size) {
        COIL_ERROR(COIL_ERR_FORMAT, "Instruction goes beyond section boundary");
        return 0;
    }
    
    // Copy instruction header
    memcpy(op, sect->data + pos, sizeof(coil_instr_t));
    
    // Return updated position
    return pos + sizeof(coil_instr_t);
}

/**
* @brief Decode operand header with or without offset addition
*
* @param sect Section containing the encoded operand
* @param pos Current position in the section
* @param header Pointer to store the decoded operand header
* @param offset Pointer to store the decoded offset (if present)
* @return coil_size_t Updated position after decoding
*/
coil_size_t coil_operand_decode(coil_section_t *sect, coil_size_t pos, coil_operand_header_t *header, coil_offset_t *offset) {
    // Validate parameters
    if (sect == NULL || header == NULL || offset == NULL) {
        COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
        return 0;
    }
    
    // Check if we have enough data for the header
    if (pos + sizeof(coil_operand_header_t) > sect->size) {
        COIL_ERROR(COIL_ERR_FORMAT, "Operand header goes beyond section boundary");
        return 0;
    }
    
    // Copy operand header
    memcpy(header, sect->data + pos, sizeof(coil_operand_header_t));
    pos += sizeof(coil_operand_header_t);
    
    // If this is an offset operand, decode the offset
    if (header->type == COIL_TYPEOP_OFF) {
        // Check if we have enough data for the offset
        if (pos + sizeof(coil_offset_t) > sect->size) {
            COIL_ERROR(COIL_ERR_FORMAT, "Operand offset goes beyond section boundary");
            return 0;
        }
        
        // Copy offset data
        memcpy(offset, sect->data + pos, sizeof(coil_offset_t));
        pos += sizeof(coil_offset_t);
    } else {
        // Clear offset
        memset(offset, 0, sizeof(coil_offset_t));
    }
    
    // Return updated position
    return pos;
}

/**
* @brief Get the size of a value type
*
* @param value_type Value type
* @return coil_size_t Size in bytes
*/
static coil_size_t coil_value_type_size(coil_u8_t value_type) {
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
            return 8;
            
        case COIL_VAL_PTR:
        case COIL_VAL_SIZE:
        case COIL_VAL_SSIZE:
            return sizeof(void*);
            
        case COIL_VAL_VAR:
        case COIL_VAL_SYM:
        case COIL_VAL_EXP:
        case COIL_VAL_STR:
            return 8;  // These are 64-bit identifiers
            
        case COIL_VAL_REG:
            return 4;  // 32-bit register ID
            
        case COIL_VAL_VOID:
        default:
            return 0;
    }
}

/**
* @brief Decode operand data
*
* @param sect Section containing the encoded data
* @param pos Current position in the section
* @param data Pointer to buffer to store the decoded data
* @param datasize Size of the data buffer
* @param valsize Pointer to store the actual size of the value type
* @param header Operand header containing value type information
* @return coil_size_t Updated position after decoding
*/
coil_size_t coil_operand_decode_data(coil_section_t *sect, coil_size_t pos, void *data, coil_size_t datasize, coil_size_t *valsize, coil_operand_header_t *header) {
    // Validate parameters
    if (sect == NULL || data == NULL || valsize == NULL || header == NULL) {
        COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
        return 0;
    }
    
    // Get the size of the value type
    coil_size_t type_size = coil_value_type_size(header->value_type);
    
    // Check if the buffer is large enough
    if (datasize < type_size) {
        COIL_ERROR(COIL_ERR_INVAL, "Buffer too small for the value type");
        return 0;
    }
    
    // Check if we have enough data
    if (pos + type_size > sect->size) {
        COIL_ERROR(COIL_ERR_FORMAT, "Operand data goes beyond section boundary");
        return 0;
    }
    
    // Copy the data
    if (type_size > 0) {
        memcpy(data, sect->data + pos, type_size);
    }
    
    // Set the actual size
    *valsize = type_size;
    
    // Return updated position
    return pos + type_size;
}