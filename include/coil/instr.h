/**
* @file instr.h
* @brief Define the standard instruction interface for serialization and deserialization
*/

#ifndef __COIL_INCLUDE_GUARD_INSTR_H
#define __COIL_INCLUDE_GUARD_INSTR_H

#include <coil/arena.h>
#include <stdint.h>

// -------------------------------- Definitions -------------------------------- //

/**
* @brief Encode an instruction header with operand count
*/
enum coil_opcode_e {
  // Control Flow operations (0x00-0x0F)
  COIL_OP_NOP  = 0x00,    // No operation
  COIL_OP_BR   = 0x01,     // Branch (conditional jump)
  COIL_OP_JMP  = 0x02,    // Unconditional jump
  COIL_OP_CALL = 0x03,   // Call function
  COIL_OP_RET  = 0x04,    // Return from function
  COIL_OP_CMP  = 0x05,    // Compare (sets flags)
  COIL_OP_TEST = 0x06,   // Test (sets flags)
  // Reserved: 07-0F

  // Memory Operations (0x10-0x2F)
  COIL_OP_MOV  = 0x10,    // Copy value from source to destination (MOV has been used for copying since the creation of assembly languages)
  COIL_OP_PUSH = 0x11,   // Push onto stack
  COIL_OP_POP  = 0x12,    // Pop from stack
  COIL_OP_LEA  = 0x13,    // Load effective address
  COIL_OP_LOAD = 0x14,   // Load from memory
  COIL_OP_STOR = 0x15,   // Store from memory
  // Reserved: 15-2F

  // Arithmetic (0x30-0x4F)
  COIL_OP_ADD = 0x30,    // Addition
  COIL_OP_SUB = 0x31,    // Subtraction
  COIL_OP_MUL = 0x32,    // Multiplication
  COIL_OP_DIV = 0x33,    // Division
  COIL_OP_MOD = 0x34,    // Remainder
  COIL_OP_INC = 0x35,    // Increment
  COIL_OP_DEC = 0x36,    // Decrement
  COIL_OP_NEG = 0x37,    // Negate a value
  // Reserved: 38-4F

  // Bitwise (0x50-5F)
  COIL_OP_AND    = 0x50,  // Bitwise AND
  COIL_OP_OR     = 0x51,  // Bitwise OR
  COIL_OP_XOR    = 0x52,  // Bitwise XOR
  COIL_OP_NOT    = 0x53,  // Bitwise NOT
  COIL_OP_SHL    = 0x54,  // Shift left
  COIL_OP_SHR    = 0x55,  // Shift right (logical)
  COIL_OP_SAL    = 0x56,  // Shift arithmetic left
  COIL_OP_SAR    = 0x57,  // Shift arithmetic right
  COIL_OP_POPCNT = 0x58,  // Population count  
  // Reserved: 59-5F

  // Multi-Dimensional (0x60-0x6F)
  COIL_OP_GETE  = 0x60,   // Get element
  COIL_OP_SETE  = 0x61,   // Set element
  COIL_OP_DOT   = 0x62,   // Dot product
  COIL_OP_CROSS = 0x63,   // Cross product
  COIL_OP_NORM  = 0x64,   // Normalize
  COIL_OP_LEN   = 0x65,   // Length/magnitude
  COIL_OP_TRANS = 0x66,   // Transpose
  COIL_OP_INV   = 0x67,   // Invert/inverse

  // Crpytography and Random Numbers (0x70-0x7F)
  // Reserved: 70-7F

  // (Future Proof reservation) Reserved: 80-9F

  // Type (0xA0-0xAF)
  COIL_OP_CVT   = 0xA0,   // Type Cast
  COIL_OP_SIZE  = 0xA1,   // Sizeof Type
  COIL_OP_ALIGN = 0xA2,   // Allign Types
  // Reserved: A3-AF

  // PU (0xB0-0xCF)
    // CPU
      COIL_OP_CPU_INT     = 0xB0, // Interrupt
      COIL_OP_CPU_IRET    = 0xB1, // Interrupt return
      COIL_OP_CPU_CLI     = 0xB2, // Stop interrupts
      COIL_OP_CPU_STI     = 0xB3, // Start interrupts
      COIL_OP_CPU_SYSCALL = 0xB4, // Interrupt to supervisor from user
      COIL_OP_CPU_SYSRET  = 0xB5, // Return from supervisor interrupt
      COIL_OP_CPU_RDTSC   = 0xB6, // Read time-tamp counter
    // GPU
      // TODO...

  // Arch (0xD0-0xDF)
    // CPU
      // x86
        COIL_OP_CPU_X86_CPUID = 0xD0,
        COIL_OP_CPU_X86_RDMSR = 0xD1,
        COIL_OP_CPU_X86_WRMSR = 0xD2,
        COIL_OP_CPU_X86_LGDT  = 0xD3,
        COIL_OP_CPU_X86_SGDT  = 0xD4,
        COIL_OP_CPU_X86_LIDT  = 0xD5,
        COIL_OP_CPU_X86_SIDT  = 0xD6,
        COIL_OP_CPU_X86_RDPMC = 0xD7,
      // arm
        COIL_OP_CPU_ARM_SEV = 0xD0,
        COIL_OP_CPU_ARM_WFE = 0xD1,
        COIL_OP_CPU_ARM_MRS = 0xD2,
        COIL_OP_CPU_ARM_MSR = 0xD3,
    // GPU
      // AMD
        // TODO...
      // NVIDIA
        // TODO...
      // INTEL

  // Directive (0xE0-0xFE)
  COIL_OP_DEF    = 0xE0,   // Define an expression
  COIL_OP_UDEF   = 0xE1,   // Undefine an expression
  COIL_OP_ISDEF  = 0xE2,   // Check if an expression exists
  COIL_OP_DATA   = 0xE3,   // Insert raw bytes into the program
  COIL_OP_PADD   = 0xE4,   // Padd program to byte offset (byte offset represents the native binary offset not the COIL file, this is to provide compatibiltiy with x86 bios bootloaders)
  // Reserved: E5-EF
  COIL_OP_ABI    = 0xF0,   // Define an ABI
  COIL_OP_SPARAM = 0xF1,  // Set the parameter value utilizing the current ABI (used in the caller)
  COIL_OP_GPARAM = 0xF2,  // Get the parameter value utilizing the current ABI (used in the callee)
  COIL_OP_SRET   = 0xF3,  // Set the return value utilizing the current ABI (used in the callee)
  COIL_OP_GRET   = 0xF4,  // Get the return value utilizing the current ABI (used in the caller)
  // Reserved: F5-FE
  
  // Attribute (0xFF)
  COIL_OP_EXT    = 0xFF   ///< Extension (similar to GNU __attribute__) (currently reserved)
};
typedef uint8_t coil_opcode_t;

/**
* @brief Encode the data encoding
*/
enum coil_operand_type_e {
  COIL_TYPEOP_NONE = 0x00, // No operand
  COIL_TYPEOP_REG = 0x01,  // u32 Register
  COIL_TYPEOP_VAR = 0x02,  // u64 Variable reference
  COIL_TYPEOP_EXP = 0x03,  // u64 Expression reference
  COIL_TYPEOP_IMM = 0x04,  // void* Immediate value
  COIL_TYPEOP_SYM = 0x05,  // u64 Symbol reference
  COIL_TYPEOP_OFF = 0x06,  // { u64 u64 u64 void* }  The instruction includes offsets then another 4 bits follows this for the actual operand type
  // Reserved 0x06-0x0F
};
typedef uint8_t coil_operand_type_t;

/**
* @brief Encode the value type
*/
enum coil_value_type_e {
  // Signed Integer (0x00-0x0F)
  COIL_VAL_I8   = 0x00,  // 8-bit signed integer
  COIL_VAL_I16  = 0x01, // 16-bit signed integer
  COIL_VAL_I32  = 0x02, // 32-bit signed integer
  COIL_VAL_I64  = 0x03, // 64-bit signed integer
  
  // Unsigned Integer (0x10-0x1F)
  COIL_VAL_U8   = 0x10,  // 8-bit unsigned integer
  COIL_VAL_U16  = 0x11, // 16-bit unsigned integer
  COIL_VAL_U32  = 0x12, // 32-bit unsigned integer
  COIL_VAL_U64  = 0x13, // 64-bit unsigned integer
  
  // Floating Point (0x20-0x2F)
  COIL_VAL_F32  = 0x20, // 32-bit float (IEEE-754)
  COIL_VAL_F64  = 0x21, // 64-bit float (IEEE-754)
  
  // Reserved (0x30-BF)

  // Composite (C0-CF)

  // Not Implemented Yet (requires a way to encode type info into the operand)
  // COIL_VAL_STRUCT  = 0xC0, // Structure of types
  // COIL_VAL_ASTRUCT = 0xC1, // Anonymous/inline structure of types
  // COIL_VAL_UNION   = 0xC2, // Union of types
  // COIL_VAL_AUNION  = 0xC3, // Anonymous/inline union of types
  // COIL_VAL_PACK    = 0xC4, // Pack of types (structure with no padding)
  // COIL_VAL_APACK   = 0xC5, // Anonymous/inline pack of types
  // COIL_VAL_ALIAS   = 0xC6, // Alias type (real type is stored at type id)

  // Complex (D0-DF)

  // Not Implemented Yet (requires a way to encode type info into the operand)
  // COIL_VAL_ARR     = 0xD0, // A variable size array
  // COIL_VAL_INT     = 0xD1, // A variable bit width signed integer
  // COIL_VAL_UNT     = 0xD2, // A variable bit width unsigned integer
  // COIL_VAL_FP      = 0xD3, // A variable bit width floating point
  // COIL_VAL_DINT    = 0xD4, // A dynamic bit width signed integer
  // COIL_VAL_DUNT    = 0xD5, // A dynamic bit width unsigned integer
  // COIL_VAL_DFP     = 0xD6, // A dynamic bit width floating point

  // Platform Types
  COIL_VAL_PTR   = 0xE0,   // Pointer type (platform width)
  COIL_VAL_SIZE  = 0xE1,  // Unsigned type (platform width)
  COIL_VAL_SSIZE = 0xE2, // Signed type (platform width)

  // COIL Types
  COIL_VAL_VAR   = 0xF0, // Value is u64 Variable ID
  COIL_VAL_SYM   = 0xF1, // Value is u64 symbol reference
  COIL_VAL_EXP   = 0xF2, // Value is u64 Expression ID
  COIL_VAL_REG   = 0xF3, // Value is u32 register reference
  COIL_VAL_STR   = 0xF4, // Value is u64 string table offset
  
  // Instruction Flags
  COIL_VAL_FLAG0 = 0xFA,
  COIL_VAL_FLAG1 = 0xFB,
  COIL_VAL_FLAG2 = 0xFC,
  COIL_VAL_FLAG3 = 0xFD,

  // Bit Type
  COIL_VAL_BIT  = 0xFE,

  // Void Type
  COIL_VAL_VOID = 0xFF, // Void type
};
typedef uint8_t coil_value_type_t;

/**
* @brief Encode the operand modifier
*/
enum coil_modifier_e {
  COIL_MOD_NONE   = 0,      // No modifiers
  COIL_MOD_CONST  = 1 << 0, // Constant value (value should not be changed)
  COIL_MOD_VOL    = 1 << 1, // Volatile value (value could change unexpectedly)
  COIL_MOD_ATOMIC = 1 << 2, // Atomic access (value requires atomic access)
  COIL_MOD_MUT    = 1 << 3, // Value can change (finds use cases in composite types and possible uses in the future)
};
typedef uint8_t coil_modifier_t;

/**
* @brief Instruction Header
*/
typedef struct {
  uint8_t opcode;
  uint8_t operand_count;
} coil_instr_t;

/**
* @brief Offset operand header structure
*/
typedef struct {
  uint64_t disp;
  uint64_t index;
  uint64_t scale;
  // (scale * index) + disp
} coil_offset_t;

/**
* @brief Operand header structure
*/
typedef struct {
  uint8_t type;       // Operand type
  uint8_t value_type; // Value type
  uint8_t modifier;   // Modifiers
} coil_operand_header_t;

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Encode an instruction header with operand count (should be used to encode 0 operands)
*
* @param arena Memory arena for allocation
* @param op Opcode to encode
* @param operand_count Number of operands for this instruction
*/
void encode_instr(coil_arena_t *arena, coil_opcode_t op, uint8_t operand_count);

/**
* @brief Encode an instruction header without operand count 
*
* not for 0 operands but for instructions that NEVER take any operands
*
* @param arena Memory arena for allocation
* @param op Opcode to encode
*/
void encode_instr_void(coil_arena_t *arena, coil_opcode_t op);

/**
* @brief Encode operand header with or without offset addition
* 
* @param arena Memory arena for allocation
* @param header Operand header to encode
*/
void encode_operand(coil_arena_t *arena, coil_operand_header_t *header);

/**
* @brief Encode operand header with offset 
* 
* @param arena Memory arena for allocation
* @param header Operand header to encode
* @param offset Offset data to encode
*/
void encode_operand_off(coil_arena_t *arena, coil_operand_header_t *header, coil_offset_t *offset);

/**
* @brief Encode operand data
* 
* @param arena Memory arena for allocation
* @param data Pointer to data to encode
* @param datasize Size of data to encode
*/
void encode_operand_data(coil_arena_t *arena, void *data, size_t datasize);

// -------------------------------- De-Serialization -------------------------------- //

/**
* @brief Decode an instruction header 
*
* @param buffer Pointer to the buffer containing encoded data
* @param pos Current position in the buffer
* @param op Pointer to store the decoded instruction
* @return size_t Updated buffer position after decoding
*/
size_t decode_instr(const uint8_t *buffer, size_t pos, coil_instr_t *op);

/**
* @brief Decode operand header with or without offset addition
*
* If operand does not have offset parameters the three offset values will be set to zero
*
* @param buffer Pointer to the buffer containing encoded data
* @param pos Current position in the buffer
* @param header Pointer to store the decoded operand header
* @param offset Pointer to store the decoded offset (if present)
* @return size_t Updated buffer position after decoding
*/
size_t decode_operand(const uint8_t *buffer, size_t pos, coil_operand_header_t *header, coil_offset_t *offset);

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
* @return size_t Updated buffer position after decoding
*/
size_t decode_operand_data(const uint8_t *buffer, size_t pos, void *data, size_t datasize, size_t *valsize, coil_operand_header_t *header);

#endif // __COIL_INCLUDE_GUARD_INSTR_H