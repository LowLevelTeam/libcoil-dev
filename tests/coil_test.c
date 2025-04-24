#include <coil/err.h>
#include <coil/memory.h>
#include <coil/types.h>
#include <coil/section.h>
#include <coil/instr.h>
#include <coil/obj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Custom error callback function
void error_callback(coil_error_level_t level, const char* message, const coil_error_position_t* position, void* user_data) {
    (void)user_data;
    const char* level_str = "UNKNOWN";
    
    switch (level) {
        case COIL_LEVEL_INFO:    level_str = "INFO"; break;
        case COIL_LEVEL_WARNING: level_str = "WARNING"; break;
        case COIL_LEVEL_ERROR:   level_str = "ERROR"; break;
        case COIL_LEVEL_FATAL:   level_str = "FATAL"; break;
    }
    
    if (position && position->file) {
        printf("[%s] %s (%s:%zu)\n", level_str, message, position->file, position->line);
    } else {
        printf("[%s] %s\n", level_str, message);
    }
}

// Test creating and saving a simple COIL object
int test_create_object(const char* filename) {
    coil_object_t obj;
    coil_err_t err;

    printf("\n--- Testing Object Creation ---\n");
    
    // Initialize object
    err = coil_obj_init(&obj, COIL_OBJ_INIT_EMPTY);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to initialize object: %s\n", coil_error_string(err));
        return 1;
    }
    
    // Create a code section
    coil_section_t code_section;
    err = coil_section_init(&code_section, 256, COIL_SECT_MODE_R | COIL_SECT_MODE_W);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to create code section: %s\n", coil_error_string(err));
        coil_obj_cleanup(&obj);
        return 1;
    }

    // Add some simple instructions with new format
    
    // NOP instruction (VOID format)
    err = coil_instr_encode(&code_section, COIL_OP_NOP);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode NOP: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // MOV instruction (FLAG_BINARY format)
    err = coil_instrflag_encode(&code_section, COIL_OP_MOV, COIL_INSTRFLAG_NEQ); // Using some flag
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode MOV: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // Destination register (r1)
    err = coil_operand_encode(&code_section, COIL_TYPEOP_REG, COIL_VAL_REG, COIL_MOD_NONE);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode destination operand: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    coil_u32_t reg_id = 1; // r1
    err = coil_operand_encode_data(&code_section, &reg_id, sizeof(reg_id));
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode register ID: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // Source immediate value (42)
    err = coil_operand_encode(&code_section, COIL_TYPEOP_IMM, COIL_VAL_I32, COIL_MOD_CONST);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode source operand: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    coil_i32_t imm_value = 42;
    err = coil_operand_encode_data(&code_section, &imm_value, sizeof(imm_value));
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode immediate value: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // DEF instruction (VALUE format)
    coil_u64_t expr_id = 123;
    err = coil_instrval_encode(&code_section, COIL_OP_DEF, expr_id);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode DEF: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // RET instruction (VOID format)
    err = coil_instr_encode(&code_section, COIL_OP_RET);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to encode RET: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }

    // Create a string section for symbol names
    coil_section_t strtab_section;
    err = coil_section_init(&strtab_section, 128, COIL_SECT_MODE_R | COIL_SECT_MODE_W);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to create string table section: %s\n", coil_error_string(err));
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    // Add some strings
    err = coil_section_putstr(&strtab_section, "main");
    if (err != COIL_ERR_GOOD) {
        printf("Failed to add string: %s\n", coil_error_string(err));
        coil_section_cleanup(&strtab_section);
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    // Add sections to object
    coil_u16_t code_index, strtab_index;
    
    err = coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".text", 
                                COIL_SECTION_FLAG_CODE, &code_section, &code_index);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to add code section to object: %s\n", coil_error_string(err));
        coil_section_cleanup(&strtab_section);
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    err = coil_obj_create_section(&obj, COIL_SECTION_STRTAB, ".strtab", 
                                COIL_SECTION_FLAG_NONE, &strtab_section, &strtab_index);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to add string table section to object: %s\n", coil_error_string(err));
        coil_section_cleanup(&strtab_section);
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    // Save the object to file
    err = coil_obj_save_file(&obj, filename);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to save object: %s\n", coil_error_string(err));
        coil_section_cleanup(&strtab_section);
        coil_section_cleanup(&code_section);
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    printf("Successfully created COIL object with %d sections\n", obj.header.section_count);
    
    // Clean up
    coil_obj_cleanup(&obj);

    return 0;
}

// Test loading a COIL object
int test_load_object(const char* filename) {
    coil_object_t obj;
    coil_err_t err;
    
    printf("\n--- Testing Object Loading ---\n");
    
    // Load object file using memory mapping
    err = coil_obj_mmap(&obj, filename);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to load object: %s\n", coil_error_string(err));
        return 1;
    }
    
    printf("Loaded COIL object with %d sections\n", obj.header.section_count);
    
    // Find code section
    coil_u16_t code_index;
    err = coil_obj_find_section(&obj, ".text", &code_index);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to find code section: %s\n", coil_error_string(err));
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    // Load code section
    coil_section_t code_section;
    err = coil_obj_load_section(&obj, code_index, &code_section, 
                              COIL_SECT_MODE_R | COIL_SLOAD_VIEW);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to load code section: %s\n", coil_error_string(err));
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    printf("Loaded code section with %zu bytes\n", code_section.size);
    
    // Decode instructions with new format
    coil_size_t pos = 0;
    while (pos < code_section.size) {
        coil_instrmem_t instr_mem;
        coil_instrfmt_t fmt;
        
        coil_size_t new_pos = coil_instr_decode(&code_section, pos, &instr_mem, &fmt);
        if (new_pos == 0) {
            printf("Failed to decode instruction at position %zu\n", pos);
            break;
        }
        pos = new_pos;
        
        // Print instruction information based on format
        printf("Instruction: opcode=0x%02X, format=%d\n", instr_mem.opcode, fmt);
        
        // Handle different instruction formats
        switch (fmt) {
            case COIL_INSTRFMT_VOID:
                printf("  VOID instruction\n");
                break;
                
            case COIL_INSTRFMT_VALUE: {
                coil_instrval_t *instr = (coil_instrval_t*)&instr_mem;
                printf("  VALUE instruction: value=%llu\n", (unsigned long long)instr->value);
                break;
            }
            
            case COIL_INSTRFMT_UNARY:
                printf("  UNARY instruction (1 operand follows)\n");
                break;
                
            case COIL_INSTRFMT_BINARY:
                printf("  BINARY instruction (2 operands follow)\n");
                break;
                
            case COIL_INSTRFMT_TENARY:
                printf("  TENARY instruction (3 operands follow)\n");
                break;
                
            case COIL_INSTRFMT_FLAG_UNARY: {
                coil_instrflag_t *instr = (coil_instrflag_t*)&instr_mem;
                printf("  FLAG_UNARY instruction: flag=%d (1 operand follows)\n", instr->flag);
                break;
            }
            
            case COIL_INSTRFMT_FLAG_BINARY: {
                coil_instrflag_t *instr = (coil_instrflag_t*)&instr_mem;
                printf("  FLAG_BINARY instruction: flag=%d (2 operands follow)\n", instr->flag);
                break;
            }
            
            case COIL_INSTRFMT_FLAG_TENARY: {
                coil_instrflag_t *instr = (coil_instrflag_t*)&instr_mem;
                printf("  FLAG_TENARY instruction: flag=%d (3 operands follow)\n", instr->flag);
                break;
            }
            
            default:
                printf("  Unknown instruction format\n");
                break;
        }
        
        // Decode operands based on format
        int operand_count = 0;
        switch (fmt) {
            case COIL_INSTRFMT_VOID:
            case COIL_INSTRFMT_VALUE:
                operand_count = 0;
                break;
            case COIL_INSTRFMT_UNARY:
            case COIL_INSTRFMT_FLAG_UNARY:
                operand_count = 1;
                break;
            case COIL_INSTRFMT_BINARY:
            case COIL_INSTRFMT_FLAG_BINARY:
                operand_count = 2;
                break;
            case COIL_INSTRFMT_TENARY:
            case COIL_INSTRFMT_FLAG_TENARY:
                operand_count = 3;
                break;
            default:
                operand_count = 0;
                break;
        }
        
        // Decode operands
        for (int i = 0; i < operand_count; i++) {
            coil_operand_header_t header;
            coil_offset_t offset;
            
            new_pos = coil_operand_decode(&code_section, pos, &header, &offset);
            if (new_pos == 0) {
                printf("Failed to decode operand header\n");
                break;
            }
            pos = new_pos;
            
            printf("  Operand %d: type=%d, value_type=%d, modifier=%d\n", 
                   i, header.type, header.value_type, header.modifier);
            
            // Decode operand data based on type
            if (header.type == COIL_TYPEOP_REG) {
                coil_u32_t reg_id;
                coil_size_t valsize;
                
                new_pos = coil_operand_decode_data(&code_section, pos, &reg_id, 
                                              sizeof(reg_id), &valsize, &header);
                if (new_pos == 0) {
                    printf("Failed to decode register ID\n");
                    break;
                }
                pos = new_pos;
                
                printf("    Register: r%u\n", reg_id);
            } else if (header.type == COIL_TYPEOP_IMM) {
                if (header.value_type == COIL_VAL_I32) {
                    coil_i32_t value;
                    coil_size_t valsize;
                    
                    new_pos = coil_operand_decode_data(&code_section, pos, &value, 
                                                  sizeof(value), &valsize, &header);
                    if (new_pos == 0) {
                        printf("Failed to decode immediate value\n");
                        break;
                    }
                    pos = new_pos;
                    
                    printf("    Immediate: %d\n", value);
                } else {
                    // Skip unknown type
                    coil_size_t type_size = 0;
                    coil_u8_t dummy[16]; // Large enough for most types
                    
                    new_pos = coil_operand_decode_data(&code_section, pos, dummy, 
                                                  sizeof(dummy), &type_size, &header);
                    if (new_pos == 0) {
                        printf("Failed to decode operand data\n");
                        break;
                    }
                    pos = new_pos;
                    
                    printf("    Unknown type data (%zu bytes)\n", type_size);
                }
            } else if (header.type == COIL_TYPEOP_OFF) {
                printf("    Offset: disp=%llu, index=%llu, scale=%llu\n",
                       (unsigned long long)offset.disp,
                       (unsigned long long)offset.index,
                       (unsigned long long)offset.scale);
            }
        }
    }
    
    // Find string table section
    coil_u16_t strtab_index;
    err = coil_obj_find_section(&obj, ".strtab", &strtab_index);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to find string table section: %s\n", coil_error_string(err));
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    // Load string table section
    coil_section_t strtab_section;
    err = coil_obj_load_section(&obj, strtab_index, &strtab_section, 
                              COIL_SECT_MODE_R | COIL_SLOAD_VIEW);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to load string table section: %s\n", coil_error_string(err));
        coil_obj_cleanup(&obj);
        return 1;
    }
    
    printf("Loaded string table section with %zu bytes\n", strtab_section.size);
    
    // Extract strings
    const char *str;
    if (strtab_section.size > 0) {
        err = coil_section_getstr(&strtab_section, 0, &str);
        if (err == COIL_ERR_GOOD) {
            printf("First string in table: '%s'\n", str);
        }
    }
    
    // Clean up
    coil_obj_cleanup(&obj);
    
    return 0;
}

// Test memory management
int test_memory() {
    printf("\n--- Testing Memory Management ---\n");
    
    // Get system page size
    coil_size_t page_size = coil_get_page_size();
    printf("System page size: %zu bytes\n", page_size);
    
    // Allocate aligned memory
    void *ptr = coil_mmap_alloc(1024, page_size);
    if (ptr == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    printf("Allocated 1024 bytes at %p\n", ptr);
    
    // Free memory
    coil_err_t err = coil_mmap_free(ptr, 1024);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to free memory: %s\n", coil_error_string(err));
        return 1;
    }
    
    printf("Successfully freed memory\n");
    
    return 0;
}

// Test section operations
int test_sections() {
    coil_section_t sect;
    coil_err_t err;
    
    printf("\n--- Testing Section Operations ---\n");
    
    // Initialize section
    err = coil_section_init(&sect, 128, COIL_SECT_MODE_R | COIL_SECT_MODE_W);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to initialize section: %s\n", coil_error_string(err));
        return 1;
    }
    
    printf("Initialized section with capacity: %zu\n", sect.capacity);
    
    // Write data
    const char *data = "Hello, COIL!";
    coil_size_t bytes_written;
    
    err = coil_section_write(&sect, (coil_byte_t*)data, strlen(data) + 1, &bytes_written);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to write data: %s\n", coil_error_string(err));
        coil_section_cleanup(&sect);
        return 1;
    }
    
    printf("Wrote %zu bytes to section\n", bytes_written);
    
    // Reset read index
    coil_section_seek_read(&sect, 0);
    
    // Read data
    char buffer[32] = {0};
    coil_size_t bytes_read;
    
    err = coil_section_read(&sect, (coil_byte_t*)buffer, sizeof(buffer), &bytes_read);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to read data: %s\n", coil_error_string(err));
        coil_section_cleanup(&sect);
        return 1;
    }
    
    printf("Read %zu bytes from section: '%s'\n", bytes_read, buffer);
    
    // Ensure capacity
    err = coil_section_ensure_capacity(&sect, 256);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to ensure capacity: %s\n", coil_error_string(err));
        coil_section_cleanup(&sect);
        return 1;
    }
    
    printf("Expanded section capacity to %zu bytes\n", sect.capacity);
    
    // Compact section
    err = coil_section_compact(&sect);
    if (err != COIL_ERR_GOOD) {
        printf("Failed to compact section: %s\n", coil_error_string(err));
        coil_section_cleanup(&sect);
        return 1;
    }
    
    printf("Compacted section to %zu bytes\n", sect.capacity);
    
    // Clean up
    coil_section_cleanup(&sect);
    
    return 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *filename = "test.coil";
    
    // Initialize error system
    coil_error_init();
    
    // Set custom error callback
    coil_error_set_callback(error_callback, NULL);
    
    printf("COIL Library Test Program\n");
    printf("========================\n");
    
    // Run tests
    if (test_memory() != 0) {
        return 1;
    }
    
    if (test_sections() != 0) {
        return 1;
    }
    
    if (test_create_object(filename) != 0) {
        return 1;
    }
    
    if (test_load_object(filename) != 0) {
        return 1;
    }
    
    printf("\nAll tests completed successfully!\n");
    
    // Shutdown error system
    coil_error_shutdown();
    
    return 0;
}