/**
* @file types.h
* @brief Common enumerations, aliases and structures throughout the program
*/

#ifndef __COIL_INCLUDE_GUARD_TYPES_H
#define __COIL_INCLUDE_GUARD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// -------------------------------- Base -------------------------------- //

/**
* @brief Basic fixed width types
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

typedef char coil_bit_t;   ///< boolean value
typedef char coil_byte_t;  ///< byte value
typedef char coil_char_t;  ///< character
typedef size_t coil_size_t;  ///< size_t

// -------------------------------- Section -------------------------------- //

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
  COIL_SECTION_PROGBITS = 1,    ///< Program space with data (can store either COIL or native code with target metadata)
  COIL_SECTION_SYMTAB = 2,      ///< Symbol table
  COIL_SECTION_STRTAB = 3,      ///< String table
  COIL_SECTION_RELTAB = 4,      ///< Relocation entries
  COIL_SECTION_NOBITS = 5,      ///< Program space with no data (bss)
  COIL_SECTION_DEBUG = 6,       ///< Debug information
  COIL_SECTION_TARGET = 7       ///< Section with specific target architecture (explicitly for native machine code)
} coil_section_type_t;

/**
* @brief Section flags
*/
typedef enum coil_section_flag_e {
  COIL_SECTION_FLAG_NONE = 0,          ///< No flags
  COIL_SECTION_FLAG_WRITE = 1 << 0,    ///< Writable
  COIL_SECTION_FLAG_CODE = 1 << 1,     ///< Executable code section (used for both COIL and native code)
  COIL_SECTION_FLAG_MERGE = 1 << 2,    ///< Might be merged
  COIL_SECTION_FLAG_ALLOC = 1 << 3,    ///< Occupies memory during execution
  COIL_SECTION_FLAG_TLS = 1 << 4,      ///< Thread-local storage
  COIL_SECTION_FLAG_TARGET = 1 << 5,   ///< Contains native machine code for a specific target architecture
  COIL_SECTION_FLAG_NATIVE = 1 << 5    ///< Alias for TARGET flag (backwards compatibility)
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
  COIL_SECT_MODE_CREATE, // New object (RW)
  COIL_SECT_MODE_MODIFY, // Loaded object (RW)
  COIL_SECT_MODE_VIEW,   // Loaded object (R)
} coil_section_mode_t;

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
  // No Flag
  COIL_INSTRFLAG_NONE = 0x00,

  // Runtime Condition
  COIL_INSTRFLAG_EQ = 0x01,
  COIL_INSTRFLAG_NEQ = 0x02,
  COIL_INSTRFLAG_GT = 0x03,
  COIL_INSTRFLAG_GTE = 0x04,
  COIL_INSTRFLAG_LT = 0x05,
  COIL_INSTRFLAG_LTE = 0x06,
  // Reserved
};
typedef uint8_t coil_instrflags_t;

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
  COIL_OP_MOV    = 0x10,  ///< Copy value from source to destination
  COIL_OP_PUSH   = 0x11,  ///< Push onto stack
  COIL_OP_POP    = 0x12,  ///< Pop from stack
  COIL_OP_LEA    = 0x13,  ///< Load effective address
  COIL_OP_PUSHFD = 0x14,  ///< Push Flag Register
  COIL_OP_POPFD  = 0x15,  ///< Pop Flag Register
  COIL_OP_PUSHA  = 0x16,  ///< Push Flag Register
  COIL_OP_POPA   = 0x17,  ///< Pop Flag Register
  COIL_OP_VAR    = 0x18,  ///< Define a Variable
  COIL_OP_SCOPE  = 0x19,  ///< Enter a Scope
  COIL_OP_SCOPL  = 0x1A,  ///< Leave a Scope
  // Reserved: 1B-2F

  // Arithmetic (0x20-0x4F)
  COIL_OP_ADD  = 0x20,    ///< Addition
  COIL_OP_SUB  = 0x21,    ///< Subtraction
  COIL_OP_MUL  = 0x22,    ///< Multiplication
  COIL_OP_DIV  = 0x23,    ///< Division
  COIL_OP_MOD  = 0x24,    ///< Remainder
  COIL_OP_INC  = 0x25,    ///< Increment
  COIL_OP_DEC  = 0x26,    ///< Decrement
  COIL_OP_NEG  = 0x27,    ///< Negate value
  // COIL_OP_ABS  = 0x28,    ///< Absolute Value
  // COIL_OP_SIN  = 0x29,    ///< Sin of Value
  // COIL_OP_COS  = 0x2A,    ///< CoSin of Value
  // COIL_OP_TAN  = 0x2B,    ///< Tanget of Value
  // COIL_OP_POW  = 0x2C,    ///< Power
  // COIL_OP_SQRT = 0x2D,    ///< Square Root
  // COIL_OP_MAX  = 0x2E,    ///< Maximum Value
  // COIL_OP_MIN  = 0x2F,    ///< Minimum Value
  // Reserved: 30-4F

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
  // Reserved: A1-AF

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
  // Reserved: E2-EF
  COIL_OP_SPARAM = 0xF0,   ///< Set the parameter value utilizing the current ABI (used in the caller)
  COIL_OP_GPARAM = 0xF1,   ///< Get the parameter value utilizing the current ABI (used in the callee)
  COIL_OP_SRET   = 0xF2,   ///< Set the return value utilizing the current ABI (used in the callee)
  COIL_OP_GRET   = 0xF3,   ///< Get the return value utilizing the current ABI (used in the caller)
  // Reserved: F4-FE
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
  COIL_VAL_I128  = 0x04,  ///< 128-bit signed integer
  
  // Unsigned Integer (0x10-0x1F)
  COIL_VAL_U8   = 0x10,  ///< 8-bit unsigned integer
  COIL_VAL_U16  = 0x11,  ///< 16-bit unsigned integer
  COIL_VAL_U32  = 0x12,  ///< 32-bit unsigned integer
  COIL_VAL_U64  = 0x13,  ///< 64-bit unsigned integer
  COIL_VAL_U128  = 0x14,  ///< 128-bit unsigned integer
  
  // Floating Point (0x20-0x2F)
  // COIL_VAL_F32  = 0x20,  ///< 32-bit float (IEEE-754)
  // COIL_VAL_F64  = 0x21,  ///< 64-bit float (IEEE-754)
  
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

// -------------------------------- CBC Instructions -------------------------------- //
// Some instructions can have immediates as first operand where as normal operations rely on the first operand to be the destination
// ONEI is one operand with immediates
// ONE Is one operand without immediates
#define __COIL_HELPER_COP_INSTR_ONE_IMPL(name) \
  COIL_COP_##name##I, \
  COIL_COP_##name##S, \
  COIL_COP_##name##R, \
  COIL_COP_##name##OS, \
  COIL_COP_##name##OR,
#define __COIL_HELPER_COP_INSTR_ONEI_IMPL(name) \
  COIL_COP_##name##S, \
  COIL_COP_##name##R, \
  COIL_COP_##name##OS, \
  COIL_COP_##name##OR,
#define __COIL_HELPER_COP_INSTR_TWO_IMPL(name) \
  COIL_COP_##name##SI, \
  COIL_COP_##name##SS, \
  COIL_COP_##name##SR, \
  COIL_COP_##name##SOS, \
  COIL_COP_##name##SOR, \
  COIL_COP_##name##RI, \
  COIL_COP_##name##RS, \
  COIL_COP_##name##RR, \
  COIL_COP_##name##ROS, \
  COIL_COP_##name##ROR, \
  COIL_COP_##name##OSI, \
  COIL_COP_##name##OSS, \
  COIL_COP_##name##OSR, \
  COIL_COP_##name##OSOS, \
  COIL_COP_##name##OSOR, \
  COIL_COP_##name##ORI, \
  COIL_COP_##name##ORS, \
  COIL_COP_##name##ORR, \
  COIL_COP_##name##OROS, \
  COIL_COP_##name##OROR,
#define __COIL_HELPER_COP_INSTR_TWOI_IMPL(name) \
  COIL_COP_##name##II, \
  COIL_COP_##name##IS, \
  COIL_COP_##name##IR, \
  COIL_COP_##name##IOS, \
  COIL_COP_##name##IOR, \
  COIL_COP_##name##SI, \
  COIL_COP_##name##SS, \
  COIL_COP_##name##SR, \
  COIL_COP_##name##SOS, \
  COIL_COP_##name##SOR, \
  COIL_COP_##name##RI, \
  COIL_COP_##name##RS, \
  COIL_COP_##name##RR, \
  COIL_COP_##name##ROS, \
  COIL_COP_##name##ROR, \
  COIL_COP_##name##OSI, \
  COIL_COP_##name##OSS, \
  COIL_COP_##name##OSR, \
  COIL_COP_##name##OSOS, \
  COIL_COP_##name##OSOR, \
  COIL_COP_##name##ORI, \
  COIL_COP_##name##ORS, \
  COIL_COP_##name##ORR, \
  COIL_COP_##name##OROS, \
  COIL_COP_##name##OROR,

// b = Byte 8 bit
// w = Word 16 bit
// l = Long 32 bit
// q = Quad Word 64 bit
// o = Octa Word 128 bit
#define __COIL_HELPER_COP_INSTR_ONE(name) \
  __COIL_HELPER_COP_INSTR_ONE_IMPL(name##b) \
  __COIL_HELPER_COP_INSTR_ONE_IMPL(name##w) \
  __COIL_HELPER_COP_INSTR_ONE_IMPL(name##l) \
  __COIL_HELPER_COP_INSTR_ONE_IMPL(name##q) \
  __COIL_HELPER_COP_INSTR_ONE_IMPL(name##o)
#define __COIL_HELPER_COP_INSTR_ONEI(name) \
  __COIL_HELPER_COP_INSTR_ONEI_IMPL(name##b) \
  __COIL_HELPER_COP_INSTR_ONEI_IMPL(name##w) \
  __COIL_HELPER_COP_INSTR_ONEI_IMPL(name##l) \
  __COIL_HELPER_COP_INSTR_ONEI_IMPL(name##q) \
  __COIL_HELPER_COP_INSTR_ONEI_IMPL(name##o)
#define __COIL_HELPER_COP_INSTR_TWO(name) \
  __COIL_HELPER_COP_INSTR_TWO_IMPL(name##b) \
  __COIL_HELPER_COP_INSTR_TWO_IMPL(name##w) \
  __COIL_HELPER_COP_INSTR_TWO_IMPL(name##l) \
  __COIL_HELPER_COP_INSTR_TWO_IMPL(name##q) \
  __COIL_HELPER_COP_INSTR_TWO_IMPL(name##o)
#define __COIL_HELPER_COP_INSTR_TWOI(name) \
  __COIL_HELPER_COP_INSTR_TWOI_IMPL(name##b) \
  __COIL_HELPER_COP_INSTR_TWOI_IMPL(name##w) \
  __COIL_HELPER_COP_INSTR_TWOI_IMPL(name##l) \
  __COIL_HELPER_COP_INSTR_TWOI_IMPL(name##q) \
  __COIL_HELPER_COP_INSTR_TWOI_IMPL(name##o)

typedef enum coil_cbc_cpu_opcode_e {
  // Control Flow
  COIL_COP_NOP = 0x00,
  __COIL_HELPER_COP_INSTR_ONEI(JMP)
  __COIL_HELPER_COP_INSTR_ONEI(BR)
  __COIL_HELPER_COP_INSTR_ONEI(CALL)
  COIL_COP_RET,
  __COIL_HELPER_COP_INSTR_TWOI(CMP)
  __COIL_HELPER_COP_INSTR_TWOI(TEST)
  
  // Memory Operations
  __COIL_HELPER_COP_INSTR_TWO(MOV)
  __COIL_HELPER_COP_INSTR_ONE(PUSH)
  __COIL_HELPER_COP_INSTR_ONEI(POP)
  __COIL_HELPER_COP_INSTR_TWO(LEA)
  COIL_COP_PUSHFD,
  COIL_COP_POPFD,
  COIL_COP_PUSHA,
  COIL_COP_POPA,
  COIL_COP_SCOPE,
  COIL_COP_SCOPL,

  // Arithmetic
  __COIL_HELPER_COP_INSTR_TWO(ADD)
  __COIL_HELPER_COP_INSTR_TWO(SUB)
  __COIL_HELPER_COP_INSTR_TWO(MUL)
  __COIL_HELPER_COP_INSTR_TWO(DIV)
  __COIL_HELPER_COP_INSTR_TWO(MOD)
  __COIL_HELPER_COP_INSTR_TWO(INC)
  __COIL_HELPER_COP_INSTR_TWO(DEC)
  __COIL_HELPER_COP_INSTR_TWO(NEG)

  // Bitwise
  __COIL_HELPER_COP_INSTR_TWO(AND)
  __COIL_HELPER_COP_INSTR_TWO(OR)
  __COIL_HELPER_COP_INSTR_TWO(XOR)
  __COIL_HELPER_COP_INSTR_ONE(NOT)
  __COIL_HELPER_COP_INSTR_TWO(SHL)
  __COIL_HELPER_COP_INSTR_TWO(SHR)
  __COIL_HELPER_COP_INSTR_TWO(SAL)
  __COIL_HELPER_COP_INSTR_TWO(SAR)

  // Type 
  __COIL_HELPER_COP_INSTR_TWO(CVT)

  // PU
    // CPU
      COIL_COP_INTlI, // 32 bits however CPUs will not always be able to use 32 bit values (i.e. x86 8bit, ARM 24 bit)
      COIL_COP_CPU_IRET,
      COIL_COP_CPU_CLI,
      COIL_COP_CPU_STI,
      COIL_COP_CPU_SYSCALL,
      COIL_COP_CPU_SYSRET,
      __COIL_HELPER_COP_INSTR_ONE(RDTSC)
    // GPU
      // TODO...

  // Arch
    // CPU
      // TODO.
    // GPU
      // TODO.
};
typedef coil_u16_t coil_cbc_opcode_t;



// -------------------------------- Target Configuration -------------------------------- //

/**
* @brief Processing unit types
*/
typedef enum coil_pu_e {
  COIL_PU_NONE = 0x00, ///< None/Unknown
  COIL_PU_CPU = 0x01,  ///< Central Processing Unit
  COIL_PU_GPU = 0x02,  ///< Graphics Processing Unit
  COIL_PU_TPU = 0x03,  ///< Tensor Processing Unit
  COIL_PU_NPU = 0x04,  ///< Neural Processing Unit
  COIL_PU_DSP = 0x05,  ///< Digital Signal Processor
  COIL_PU_FPGA = 0x06, ///< Field-Programmable Gate Array
  // RESERVED
} coil_pu_t;

/**
* @brief CPU architecture types
*/
typedef enum coil_cpu_e {
  COIL_CPU_NONE = 0x00,     ///< None/Unknown
  COIL_CPU_x86 = 0x01,      ///< x86 16 bit
  COIL_CPU_x86_32 = 0x02,   ///< x86 32-bit
  COIL_CPU_x86_64 = 0x03,   ///< x86 64-bit (AMD64/Intel64)
  COIL_CPU_ARMT = 0x04,     ///< Generic Thumb
  COIL_CPU_ARM32 = 0x05,    ///< ARM 32-bit
  COIL_CPU_ARM64 = 0x06,    ///< ARM 64-bit (AArch64)
  COIL_CPU_RISCV32 = 0x08,  ///< RISC-V 32-bit
  COIL_CPU_RISCV64 = 0x09,  ///< RISC-V 64-bit
  COIL_CPU_RISCV128 = 0x09, ///< RISC-V 128-bit
  COIL_CPU_PPC32 = 0x0A,    ///< PowerPC 32-bit
  COIL_CPU_PPC64 = 0x0B,    ///< PowerPC 64-bit
  COIL_CPU_MIPS32 = 0x0C,   ///< MIPS 32-bit
  COIL_CPU_MIPS64 = 0x0D,   ///< MIPS 64-bit
  COIL_CPU_WASM32 = 0x0E,   ///< WebAssembly 32
  COIL_CPU_WASM64 = 0x0F,   ///< WebAssembly 64
  // RESERVED
} coil_cpu_t;

/**
* @brief GPU architecture types
*/
typedef enum coil_gpu_e {
  COIL_GPU_NONE = 0x00,      ///< None/Unknown
  COIL_GPU_NV_CU = 0x01,     ///< NVIDIA CUDA Architecture (SASS)
  COIL_GPU_AMD_GCN = 0x05,   ///< AMD GCN Architecture
  COIL_GPU_AMD_RDNA = 0x06,  ///< AMD RDNA Architecture
  COIL_GPU_INTL_GEN9 = 0x08, ///< Intel Gen9 Graphics
  COIL_GPU_INTL_XE = 0x09,   ///< Intel Xe Graphics
  // RESERVED
} coil_gpu_t;

/**
* @brief Feature flags for x86 architecture
*/
typedef enum coil_cpu_x86_feature_e {
  COIL_CPU_X86_MMX = 1 << 0,      ///< MMX Instructions
  COIL_CPU_X86_SSE = 1 << 1,      ///< SSE Instructions
  COIL_CPU_X86_SSE2 = 1 << 2,     ///< SSE2 Instructions
  COIL_CPU_X86_SSE3 = 1 << 3,     ///< SSE3 Instructions
  COIL_CPU_X86_SSSE3 = 1 << 4,    ///< SSSE3 Instructions
  COIL_CPU_X86_SSE4_1 = 1 << 5,   ///< SSE4.1 Instructions
  COIL_CPU_X86_SSE4_2 = 1 << 6,   ///< SSE4.2 Instructions
  COIL_CPU_X86_AVX = 1 << 7,      ///< AVX Instructions
  COIL_CPU_X86_AVX2 = 1 << 8,     ///< AVX2 Instructions
  COIL_CPU_X86_AVX512F = 1 << 9,  ///< AVX-512 Foundation
  COIL_CPU_X86_FMA = 1 << 10,     ///< FMA Instructions
  COIL_CPU_X86_AES = 1 << 11,     ///< AES Instructions
  COIL_CPU_X86_PCLMUL = 1 << 12,  ///< PCLMULQDQ Instructions
  COIL_CPU_X86_SHA = 1 << 13,     ///< SHA Instructions
  // RESERVED
} coil_cpu_x86_feature_t;

/**
* @brief Feature flags for ARM architecture
*/
typedef enum coil_cpu_arm_feature_e {
  COIL_CPU_ARM_NEON = 1 << 0,     ///< NEON SIMD Instructions
  COIL_CPU_ARM_VFP = 1 << 1,      ///< Vector Floating Point
  COIL_CPU_ARM_CRYPTO = 1 << 2,   ///< Cryptography Extensions
  COIL_CPU_ARM_CRC = 1 << 3,      ///< CRC32 Instructions
  COIL_CPU_ARM_SVE = 1 << 4,      ///< Scalable Vector Extension
  COIL_CPU_ARM_SVE2 = 1 << 5,     ///< Scalable Vector Extension 2
  COIL_CPU_ARM_DOTPROD = 1 << 6,  ///< Dot Product Instructions
  COIL_CPU_ARM_MATMUL = 1 << 7,   ///< Matrix Multiplication Instructions
  // RESERVED
} coil_cpu_arm_feature_t;



#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_TYPES_H