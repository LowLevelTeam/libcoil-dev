/**
 * @file types.h
 * @brief Defines the standard enumerations and simple types used throughout the programs
 */

#ifndef __COIL_INCLUDE_GUARD_TYPES_H
#define __COIL_INCLUDE_GUARD_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Basic fixed width types 
 * If you don't have stdint in your system remove the above include and change the stdint types to native C types
 */
typedef unsigned char coil_u8_t;
typedef uint16_t coil_u16_t;
typedef uint32_t coil_u32_t;
typedef uint64_t coil_u64_t;

typedef char coil_i8_t;
typedef int16_t coil_i16_t;
typedef int32_t coil_i32_t;
typedef int64_t coil_i64_t;

typedef float coil_f32_t;
typedef double coil_f64_t;

typedef char coil_bit_t;     ///< boolean value
typedef char coil_byte_t;    ///< byte value
typedef char coil_char_t;    ///< character
typedef size_t coil_size_t;  ///< size_t

// -------------------------------- Object -------------------------------- //

/**
 * @brief Magic number for COIL object files
 * "COIL" in ASCII
 */
#define COIL_MAGIC_BYTES {'C', 'O', 'I', 'L'}

/**
 * @brief Section types
 */
typedef enum coil_section_type_e {
  COIL_SECTION_NULL = 0,        ///< Null section
  COIL_SECTION_PROGBITS = 1,    ///< Program space with data
  COIL_SECTION_SYMTAB = 2,      ///< Symbol table
  COIL_SECTION_STRTAB = 3,      ///< String table
  COIL_SECTION_RELTAB = 4,      ///< Relocation entries
  COIL_SECTION_NOBITS = 5,      ///< Program space with no data (bss)
  COIL_SECTION_DEBUG = 6        ///< Debug information
} coil_section_type_t;

/**
 * @brief Section flags
 */
typedef enum coil_section_flag_e {
  COIL_SECTION_FLAG_NONE = 0,          ///< No flags
  COIL_SECTION_FLAG_WRITE = 1 << 0,    ///< Writable
  COIL_SECTION_FLAG_CODE = 1 << 1,     ///< Compile this section as COIL
  COIL_SECTION_FLAG_MERGE = 1 << 2,    ///< Might be merged
  COIL_SECTION_FLAG_ALLOC = 1 << 3,    ///< Occupies memory during execution
  COIL_SECTION_FLAG_TLS = 1 << 4       ///< Thread-local storage
} coil_section_flag_t;

/**
 * @brief Symbol types
 */
typedef enum coil_symbol_type_e {
  COIL_SYMBOL_NOTYPE = 0,      ///< Type not specified
  COIL_SYMBOL_OBJECT = 1,      ///< Data object
  COIL_SYMBOL_FUNC = 2,        ///< Function
  COIL_SYMBOL_SECTION = 3,     ///< Section
  COIL_SYMBOL_FILE = 4         ///< File
} coil_symbol_type_t;

/**
 * @brief Symbol binding
 */
typedef enum coil_symbol_binding_e {
  COIL_SYMBOL_LOCAL = 0,       ///< Local symbol
  COIL_SYMBOL_GLOBAL = 1,      ///< Global symbol
  COIL_SYMBOL_WEAK = 2         ///< Weak symbol
} coil_symbol_binding_t;

/**
 * @brief Section Mode 
 */
typedef enum coil_section_mode_e {
  COIL_SECT_MODE_N = 0,         ///< No access
  COIL_SECT_MODE_R = (1 << 0),  ///< Read access
  COIL_SECT_MODE_W = (1 << 1),  ///< Write access
  COIL_SECT_MODE_O = (1 << 2),  ///< Owned - If not set then the object owns the memory, do not resize, do not free
} coil_section_mode_t;

/**
 * @brief Section memory ownership flags
 */
typedef enum coil_section_ownership_e {
  COIL_SECT_OWN_NONE = 0,    ///< Memory owned elsewhere (mapped or borrowed)
  COIL_SECT_OWN_SELF = 1     ///< Memory owned by this section (must be freed)
} coil_section_ownership_t;

// -------------------------------- Instructions -------------------------------- //

/**
* @brief Instruction Format for COIL instructions
*/
enum coil_instrfmt_e {
  COIL_INSTRFMT_UNKN,        // Invalid or Unsupported Instruction
  COIL_INSTRFMT_VOID,        // [opcode]
  COIL_INSTRFMT_VALUE,       // [opcode][id][operand]
  COIL_INSTRFMT_UNARY,       // [opcode][operand]
  COIL_INSTRFMT_BINARY,      // [opcode][operand][operand]
  COIL_INSTRFMT_TENARY,      // [opcode][operand][operand][operand]
  COIL_INSTRFMT_FLAG_UNARY,  // [opcode][flag][operand]
  COIL_INSTRFMT_FLAG_BINARY, // [opcode][flag][operand][operand]
  COIL_INSTRFMT_FLAG_TENARY, // [opcode][flag][operand][operand][operand]
};
typedef uint8_t coil_instrfmt_t;

/**
* @brief 
*/
enum coil_instrflag_e {
  // Runtime Condition
  COIL_INSTRFLAG_EQ = 0x00,
  COIL_INSTRFLAG_NEQ = 0x01,
  COIL_INSTRFLAG_GT = 0x02,
  COIL_INSTRFLAG_GTE = 0x03,
  COIL_INSTRFLAG_LT = 0x04,
  COIL_INSTRFLAG_LTE = 0x05,
  // Reserved
};
typedef uint8_t coil_instrflag_t;

/**
 * @brief Opcode enumeration for COIL instructions
 */
enum coil_opcode_e {
  // Control Flow operations (0x00-0x0F)
  COIL_OP_NOP  = 0x00,    ///< No operation
  COIL_OP_BR   = 0x01,    ///< Branch (conditional jump)
  COIL_OP_JMP  = 0x02,    ///< Unconditional jump
  COIL_OP_CALL = 0x03,    ///< Call function
  COIL_OP_RET  = 0x04,    ///< Return from function
  COIL_OP_CMP  = 0x05,    ///< Compare (sets flags)
  COIL_OP_TEST = 0x06,    ///< Test (sets flags)
  // Reserved: 07-0F

  // Memory Operations (0x10-0x1F)
  COIL_OP_MOV  = 0x10,    ///< Copy value from source to destination
  COIL_OP_PUSH = 0x11,    ///< Push onto stack
  COIL_OP_POP  = 0x12,    ///< Pop from stack
  COIL_OP_LEA  = 0x13,    ///< Load effective address
  // Reserved: 14-2F

  // Arithmetic (0x20-0x4F)
  COIL_OP_ADD = 0x20,     ///< Addition
  COIL_OP_SUB = 0x21,     ///< Subtraction
  COIL_OP_MUL = 0x22,     ///< Multiplication
  COIL_OP_DIV = 0x23,     ///< Division
  COIL_OP_MOD = 0x24,     ///< Remainder
  COIL_OP_INC = 0x25,     ///< Increment
  COIL_OP_DEC = 0x26,     ///< Decrement
  COIL_OP_NEG = 0x27,     ///< Negate a value
  // Reserved: 28-4F

  // Bitwise (0x50-5F)
  COIL_OP_AND    = 0x50,  ///< Bitwise AND
  COIL_OP_OR     = 0x51,  ///< Bitwise OR
  COIL_OP_XOR    = 0x52,  ///< Bitwise XOR
  COIL_OP_NOT    = 0x53,  ///< Bitwise NOT
  COIL_OP_SHL    = 0x54,  ///< Shift left
  COIL_OP_SHR    = 0x55,  ///< Shift right (logical)
  COIL_OP_SAL    = 0x56,  ///< Shift arithmetic left
  COIL_OP_SAR    = 0x57,  ///< Shift arithmetic right
  // Reserved: 58-5F

  // Multi-Dimensional (0x60-0x6F)
  // Reserved: 60-6F
  // COIL_OP_GETE  = 0x60,   ///< Get element
  // COIL_OP_SETE  = 0x61,   ///< Set element
  // COIL_OP_DOT   = 0x62,   ///< Dot product
  // COIL_OP_CROSS = 0x63,   ///< Cross product
  // COIL_OP_NORM  = 0x64,   ///< Normalize
  // COIL_OP_LEN   = 0x65,   ///< Length/magnitude
  // COIL_OP_TRANS = 0x66,   ///< Transpose
  // COIL_OP_INV   = 0x67,   ///< Invert/inverse

  // Crpytography and Random Numbers (0x70-0x7F)
  // Reserved: 70-7F

  // (Future Proof reservation) Reserved: 80-9F

  // Type (0xA0-0xAF)
  COIL_OP_CVT   = 0xA0,   ///< Type Cast
  // Reserved: A3-AF

  // PU (0xB0-0xCF)
    // CPU
      COIL_OP_CPU_INT     = 0xB0, ///< Interrupt
      COIL_OP_CPU_IRET    = 0xB1, ///< Interrupt return
      COIL_OP_CPU_CLI     = 0xB2, ///< Stop interrupts
      COIL_OP_CPU_STI     = 0xB3, ///< Start interrupts
      COIL_OP_CPU_SYSCALL = 0xB4, ///< Interrupt to supervisor from user
      COIL_OP_CPU_SYSRET  = 0xB5, ///< Return from supervisor interrupt
      COIL_OP_CPU_RDTSC   = 0xB6, ///< Read time-tamp counter
    // GPU
      // TODO...

  // Arch (0xD0-0xDF)
    // CPU
      // x86
        COIL_OP_CPU_X86_CPUID = 0xD0, ///< Get CPU information
        COIL_OP_CPU_X86_RDMSR = 0xD1, ///< Read model-specific register
        COIL_OP_CPU_X86_WRMSR = 0xD2, ///< Write model-specific register
        COIL_OP_CPU_X86_LGDT  = 0xD3, ///< Load global descriptor table
        COIL_OP_CPU_X86_SGDT  = 0xD4, ///< Store global descriptor table
        COIL_OP_CPU_X86_LIDT  = 0xD5, ///< Load interrupt descriptor table
        COIL_OP_CPU_X86_SIDT  = 0xD6, ///< Store interrupt descriptor table
        COIL_OP_CPU_X86_RDPMC = 0xD7, ///< Read performance monitoring counter
      // arm
        COIL_OP_CPU_ARM_SEV = 0xD0, ///< Send event
        COIL_OP_CPU_ARM_WFE = 0xD1, ///< Wait for event
        COIL_OP_CPU_ARM_MRS = 0xD2, ///< Move to register from special
        COIL_OP_CPU_ARM_MSR = 0xD3, ///< Move to special from register
    // GPU
      // AMD
        // TODO...
      // NVIDIA
        // TODO...
      // INTEL

  // Directive (0xE0-0xFF)
  COIL_OP_DEF    = 0xE0,   ///< Define an expression
  COIL_OP_UDEF   = 0xE1,   ///< Undefine an expression
  // Reserved: E3-EF
  COIL_OP_SPARAM = 0xF1,   ///< Set the parameter value utilizing the current ABI (used in the caller)
  COIL_OP_GPARAM = 0xF2,   ///< Get the parameter value utilizing the current ABI (used in the callee)
  COIL_OP_SRET   = 0xF3,   ///< Set the return value utilizing the current ABI (used in the callee)
  COIL_OP_GRET   = 0xF4,   ///< Get the return value utilizing the current ABI (used in the caller)
  // Reserved: F5-FE
};
typedef uint8_t coil_opcode_t;

/**
 * @brief Operand type encoding
 */
enum coil_operand_type_e {
  COIL_TYPEOP_NONE = 0x00, ///< No operand
  COIL_TYPEOP_REG = 0x01,  ///< u32 Register
  COIL_TYPEOP_VAR = 0x02,  ///< u64 Variable reference
  COIL_TYPEOP_EXP = 0x03,  ///< u64 Expression reference
  COIL_TYPEOP_IMM = 0x04,  ///< void* Immediate value
  COIL_TYPEOP_SYM = 0x05,  ///< u64 Symbol reference
  COIL_TYPEOP_OFF = 0x06,  ///< { u64 u64 u64 void* }  The instruction includes offsets then another 4 bits follows this for the actual operand type
  // Reserved 0x06-0x0F
};
typedef uint8_t coil_operand_type_t;

/**
 * @brief Value type encoding
 */
enum coil_value_type_e {
  // Signed Integer (0x00-0x0F)
  COIL_VAL_I8   = 0x00,  ///< 8-bit signed integer
  COIL_VAL_I16  = 0x01,  ///< 16-bit signed integer
  COIL_VAL_I32  = 0x02,  ///< 32-bit signed integer
  COIL_VAL_I64  = 0x03,  ///< 64-bit signed integer
  
  // Unsigned Integer (0x10-0x1F)
  COIL_VAL_U8   = 0x10,  ///< 8-bit unsigned integer
  COIL_VAL_U16  = 0x11,  ///< 16-bit unsigned integer
  COIL_VAL_U32  = 0x12,  ///< 32-bit unsigned integer
  COIL_VAL_U64  = 0x13,  ///< 64-bit unsigned integer
  
  // Floating Point (0x20-0x2F)
  COIL_VAL_F32  = 0x20,  ///< 32-bit float (IEEE-754)
  COIL_VAL_F64  = 0x21,  ///< 64-bit float (IEEE-754)
  
  // Reserved (0x30-BF)

  // Composite (C0-CF)
  // Not implemented yet

  // Complex (D0-DF)
  // Not implemented yet

  // Platform Types
  COIL_VAL_PTR   = 0xE0,  ///< Pointer type (platform width)
  COIL_VAL_SIZE  = 0xE1,  ///< Unsigned type (platform width)
  COIL_VAL_SSIZE = 0xE2,  ///< Signed type (platform width)

  // COIL Types
  COIL_VAL_VAR   = 0xF0,  ///< Value is u64 Variable ID
  COIL_VAL_SYM   = 0xF1,  ///< Value is u64 symbol reference
  COIL_VAL_EXP   = 0xF2,  ///< Value is u64 Expression ID
  COIL_VAL_REG   = 0xF3,  ///< Value is u32 register reference
  COIL_VAL_STR   = 0xF4,  ///< Value is u64 string table offset

  // Bit Type
  COIL_VAL_BIT  = 0xFE,   ///< Bit type

  // Void Type
  COIL_VAL_VOID = 0xFF,   ///< Void type
};
typedef uint8_t coil_value_type_t;

/**
 * @brief Operand modifier flags
 */
enum coil_modifier_e {
  COIL_MOD_NONE   = 0,      ///< No modifiers
  COIL_MOD_CONST  = 1 << 0, ///< Constant value (value should not be changed)
  COIL_MOD_VOL    = 1 << 1, ///< Volatile value (value could change unexpectedly)
  COIL_MOD_ATOMIC = 1 << 2, ///< Atomic access (value requires atomic access)
  COIL_MOD_MUT    = 1 << 3, ///< Value can change (finds use cases in composite types)
};
typedef uint8_t coil_modifier_t;

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_TYPES_H */