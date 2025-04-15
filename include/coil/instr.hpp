#pragma once

#include "coil/op.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace coil {

/**
* @brief Type opcode definitions for the COIL instruction set
* 
* These opcodes represent the various data types that can be used in COIL instructions.
* They are categorized into several ranges:
* - Fixed Width: Basic integer and floating point types
* - Fixed Width Vector: Vector types with fixed sizes
* - Composite: Structures, unions, and other composite types
* - Platform: Platform-specific types like pointers
* - Optimized: Special optimized types
* - COIL specific: Types specific to the COIL system
* - Parameter: Parameter type placeholders
* - Special: Special types like void
*/
enum class TypeOpcode : uint8_t {
  // Fixed Width types
  FW_RANGE_START = 0x00,
  FW_RANGE_END   = 0x9F,

  // Integer types
  I8    = 0x00,  ///< 8-bit signed integer
  I16   = 0x01,  ///< 16-bit signed integer
  I32   = 0x02,  ///< 32-bit signed integer
  I64   = 0x03,  ///< 64-bit signed integer
  I128  = 0x04,  ///< 128-bit signed integer

  U8    = 0x10,  ///< 8-bit unsigned integer
  U16   = 0x11,  ///< 16-bit unsigned integer
  U32   = 0x12,  ///< 32-bit unsigned integer
  U64   = 0x13,  ///< 64-bit unsigned integer
  U128  = 0x14,  ///< 128-bit unsigned integer

  // Floating point types
  F8E5M2 = 0x20, ///< 8-bit float (5-bit exponent, 2-bit mantissa)
  F8E4M3 = 0x21, ///< 8-bit float (4-bit exponent, 3-bit mantissa)
  F16    = 0x22, ///< 16-bit float
  FB16   = 0x23, ///< 16-bit brain float
  F32    = 0x24, ///< 32-bit float
  FT32   = 0x25, ///< 32-bit tensor float
  F64    = 0x26, ///< 64-bit float
  F80    = 0x27, ///< 80-bit extended precision float
  F128   = 0x28, ///< 128-bit float

  // Fixed Width Vector types
  V128  = 0x50,  ///< 128-bit vector
  V256  = 0x51,  ///< 256-bit vector
  V512  = 0x52,  ///< 512-bit vector

  // Composite types
  COMP_RANGE_START = 0xC0,
  COMP_RANGE_END   = 0xCF,

  // Not Implemented Yet
  // STRUCT  = 0xC0, ///< Structure of types
  // ASTRUCT = 0xC1, ///< Anonymous/inline structure of types
  // UNION   = 0xC2, ///< Union of types
  // AUNION  = 0xC3, ///< Anonymous/inline union of types
  // PACK    = 0xC4, ///< Pack of types (structure with no padding)
  // APACK   = 0xC5, ///< Anonymous/inline pack of types
  // ALIAS   = 0xC6, ///< Alias type (real type is stored at type id)

  // Platform types
  PLT_RANGE_START = 0xD0,
  PLT_RANGE_END   = 0xDF,

  PTR   = 0xD0,   ///< Platform pointer
  SIZE  = 0xD1,   ///< Platform size_t (largest unsigned integer)
  SSIZE = 0xD2,   ///< Platform ssize_t (largest signed integer)

  // Optimized types
  OPT_RANGE_START = 0xE0,
  OPT_RANGE_END   = 0xEF,

  BIT   = 0xE0,   ///< Bit/Bool (to allow processors to create bit maps rather then allocating a byte for each bit)

  // COIL specific types
  COIL_RANGE_START = 0xF0,
  COIL_RANGE_END   = 0xF9,

  VAR   = 0xF0,   ///< Variable (Value is Variable ID)
  SYM   = 0xF1,   ///< Symbol (Value is symbol reference)
  EXP   = 0xF2,   ///< Expression (Value is Expression ID)
  REG   = 0xF3,   ///< Register (Value is register reference)

  // Parameter types
  // When passing parameter types it is important to note that they have a different encoding
  // Parameters utilize the following uint8_t control as the parameter value instead and have no data after that
  PARAM_RANGE_START = 0xFA,
  PARAM_RANGE_END   = 0xFE,

  PARAM3 = 0xFA,   ///< Parameter placeholder 3
  PARAM2 = 0xFB,   ///< Parameter placeholder 2
  PARAM1 = 0xFC,   ///< Parameter placeholder 1
  PARAMC = 0xFD,   ///< Parameter Conditional

  // Special types
  VOID  = 0xFF     ///< Void type (no data)
};

/**
* @brief Type control flags for the COIL instruction set
* 
* These flags control type modifiers and specify how the type data should be interpreted.
*/
enum TypeControl : uint8_t {
  CONST  = (1 << 0),  ///< Constant value
  VOL    = (1 << 1),  ///< Volatile value
  ATOMIC = (1 << 2),  ///< Atomic value
  REG    = (1 << 3),  ///< Register value
  IMM    = (1 << 4),  ///< Immediate value
  VAR    = (1 << 5),  ///< Variable reference
  SYM    = (1 << 6),  ///< Symbol reference
  EXP    = (1 << 7)   ///< Expression reference
};

/**
* @brief Conditional Parameter
*
* Runtime parameter to check flags at execution to control instructions execution
*/
enum TypeParamCond : uint16_t {
  TPCOND_EQ  = 0x00,
  TPCOND_NEQ = 0x01,
  TPCOND_LT  = 0x02,
  TPCOND_LTE = 0x03,
  TPCOND_GT  = 0x04,
  TPCOND_GTE = 0x05,
};


/**
* @brief Instruction opcodes for the COIL instruction set
* 
* These opcodes represent the various operations that can be performed in COIL.
* They are categorized into several ranges:
* - Control Flow: Branch, call, return, compare operations
* - Memory: Memory operations like move, push, pop
* - Arithmetic: Basic arithmetic operations
* - Bit Manipulation: Bitwise operations
* - Mathematical: Vector and matrix operations
* - Random Number: Random number generation
* - Type Operations: Type conversions and manipulations
* - Extended: Architecture-specific and processing unit-specific operations
* - Directives: Preprocessor-like operations
*/
enum class Opcode : uint8_t {
  // Control Flow operations
  CF_RANGE_START = 0x00,
  CF_RANGE_END   = 0x1F,

  NOP  = 0x00,  ///< No operation
  BR   = 0x01,  ///< Branch
  CALL = 0x02,  ///< Call function
  RET  = 0x03,  ///< Return from function
  CMP  = 0x04,  ///< Compare values
  TEST = 0x05,  ///< Test value (logical AND with flags only)

  // Memory operations
  MEM_RANGE_START = 0x20,
  MEM_RANGE_END   = 0x3F,

  MOV   = 0x20,  ///< Move data
  PUSH  = 0x21,  ///< Push onto stack
  POP   = 0x22,  ///< Pop from stack
  LEA   = 0x23,  ///< Load effective address
  SCOPE = 0x24,  ///< Begin scope
  SCOPL = 0x25,  ///< End scope
  VAR   = 0x26,  ///< Define variable
  XCHG  = 0x27,  ///< Exchange values
  CAS   = 0x28,  ///< Compare and swap

  // Arithmetic operations
  ARITH_RANGE_START = 0x40,
  ARITH_RANGE_END   = 0x5F,

  ADD = 0x40,  ///< Addition
  SUB = 0x41,  ///< Subtraction
  MUL = 0x42,  ///< Multiplication
  DIV = 0x43,  ///< Division
  MOD = 0x44,  ///< Modulo
  INC = 0x45,  ///< Increment
  DEC = 0x46,  ///< Decrement
  
  // Bitwise operations
  BIT_RANGE_START = 0x60,
  BIT_RANGE_END   = 0x6F,

  AND    = 0x60,  ///< Bitwise AND
  OR     = 0x61,  ///< Bitwise OR
  XOR    = 0x62,  ///< Bitwise XOR
  NOT    = 0x63,  ///< Bitwise NOT
  SHL    = 0x64,  ///< Shift left
  SHR    = 0x65,  ///< Shift right (logical)
  SAL    = 0x66,  ///< Shift arithmetic left
  SAR    = 0x67,  ///< Shift arithmetic right
  POPCNT = 0x68,  ///< Population count

  // Math/Matrix/Vector operations
  MD_RANGE_START = 0x70,
  MD_RANGE_END   = 0x7F,

  GETE  = 0x70,  ///< Get element
  SETE  = 0x71,  ///< Set element
  DOT   = 0x72,  ///< Dot product
  CROSS = 0x73,  ///< Cross product
  NORM  = 0x74,  ///< Normalize
  LEN   = 0x75,  ///< Length/magnitude
  TRANS = 0x76,  ///< Transpose
  INV   = 0x77,  ///< Invert/inverse

  // Random number operations
  RAND_RANGE_START = 0x80,
  RAND_RANGE_END   = 0x8F,

  // Reserved for future use
  RESV_RANGE_START = 0x90,
  RESV_RANGE_END   = 0xAF,

  // Type operations
  TYPE_RANGE_START = 0xB0,
  TYPE_RANGE_END   = 0xBF,

  // Extended operations
  EXT_RANGE_START = 0xC0,
  EXT_RANGE_END   = 0xEF,

  // Processing Unit specific operations
  PU_RANGE_START = 0xC0,
  PU_RANGE_END   = 0xCF,

  // Architecture specific operations
  ARCH_RANGE_START = 0xD0,
  ARCH_RANGE_END   = 0xDF,

  // Directive operations
  DIR_RANGE_START = 0xE0,
  DIR_RANGE_END   = 0xFF,

  PPDEF  = 0xE0,  ///< Define preprocessor macro
  PPUDEF = 0xE1,  ///< Undefine preprocessor macro
  PPIF   = 0xE2,  ///< Preprocessor if
  PPELIF = 0xE3,  ///< Preprocessor else if
  PPELSE = 0xE4,  ///< Preprocessor else
  PPEIF  = 0xE5,  ///< Preprocessor end if
  PPINC  = 0xE6,  ///< Include file
  PPSEC  = 0xE7,  ///< Begin section
  PPDATA = 0xE8,  ///< Raw data insertion
  PPPADD = 0xE9,  ///< Pad program to byte offset
  
  PPTARG = 0xFC,  ///< Set target platform
  PPABI  = 0xFD,  ///< Set ABI
  
  PPEXT  = 0xFF   ///< Extension (similar to GNU __attribute__)
};

/**
* @brief CPU-specific operation codes
*/
enum class CpuOp : uint8_t {
  INT     = 0xC0,  ///< Software interrupt
  IRET    = 0xC1,  ///< Return from interrupt
  CLI     = 0xC2,  ///< Clear interrupt flag
  STI     = 0xC3,  ///< Set interrupt flag
  SYSCALL = 0xC4,  ///< System call
  SYSRET  = 0xC5,  ///< Return from system call
  RDTSC   = 0xC6   ///< Read time-stamp counter
};

/**
* @brief x86-specific CPU operations
*/
enum class CpuX86Op : uint8_t {
  CPUID = 0xD0,  ///< CPU identification
  RDMSR = 0xD1,  ///< Read model-specific register
  WRMSR = 0xD2,  ///< Write model-specific register
  LGDT  = 0xD3,  ///< Load global descriptor table
  SGDT  = 0xD4,  ///< Store global descriptor table
  LIDT  = 0xD5,  ///< Load interrupt descriptor table
  SIDT  = 0xD6,  ///< Store interrupt descriptor table
  RDPMC = 0xD7   ///< Read performance monitoring counter
};

/**
* @brief ARM-specific CPU operations
*/
enum class CpuArmOp : uint8_t {
  SEV = 0xD0,  ///< Send event
  WFE = 0xD1,  ///< Wait for event
  MRS = 0xD2,  ///< Move from system register
  MSR = 0xD3   ///< Move to system register
};

/**
* @brief GPU-specific operation codes
*/
enum class GpuOp : uint8_t {
  // Base GPU operations
  BARRIER   = 0xC0,  ///< Memory barrier
  WORKGROUP = 0xC1,  ///< Workgroup synchronization
  SHUFFLE   = 0xC2   ///< Shuffle values between lanes
};

/**
* @brief NVIDIA-specific GPU operations
*/
enum class GpuNvOp : uint8_t {
  WARP_SYNC = 0xD0,   ///< Warp synchronization
  VOTE_ALL  = 0xD1,   ///< Warp vote all
  VOTE_ANY  = 0xD2,   ///< Warp vote any
  SHFL      = 0xD3    ///< Shuffle within warp
};

/**
* @brief AMD-specific GPU operations
*/
enum class GpuAmdOp : uint8_t {
  WAVE_BARRIER = 0xD0,  ///< Wave barrier
  WAVE_VOTE    = 0xD1   ///< Wave vote
};

/**
* @brief Instruction Set
*/
namespace Instr {
  union ImmediateValue {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    int64_t i128[2];

    uint8_t u8;
    uint8_t u16;
    uint32_t u32;
    uint64_t u64;
    uint64_t u128[2];

    float f32; // f16 and f8s can just be stored in f32 since there are no native types
    double f64;
    double f128[2];

    char vec128[16];
    char vec256[32];
    char vec512[64];
  };
  struct Operand {
    uint8_t top;
    uint8_t ctrl;

    void *typedata; // for future composite and complex types which require extra information

    union {
      ImmediateValue Imm;
      uint64_t VarID;
      uint64_t symref;
      uint64_t ExpID;
      uint32_t RegID;
    } data;
  };
  struct Param {
    uint8_t top;
    uint16_t data;
  };

  // Void Instruction has just a uint8_t opcode
  // Encoding: [opcode: uint8_t]
  struct Void {
    uint8_t opcode;
  };
  typedef Void NOP;
  
  struct VoidParam {
    uint8_t opcode;
    uint8_t opcount; // 0 or 1
    Param param;
  };  
  typedef VoidParam RET;
  typedef VoidParam SCOPE;
  typedef VoidParam SCOPL;

  // Encoding: [opcode: uint8_t][opcount: uint8_t] [[op: {Operand, Param}], opcount...]
  struct CtxChange {
    uint8_t opcode;
    uint8_t opcount;

    Operand location;

    OpParam condition;
  };
  typedef CtxChange BR;
  typedef CtxChange CALL;

  // Encoding: [opcode: uint8_t][opcount: uint8_t] [[op: {Operand, Param}], opcount...]
  struct Unary {
    uint8_t opcode;
    uint8_t opcount;

    Operand op;

    OpParam condition;
  };
  typedef Unary PUSH;
  typedef Unary POP;
  typedef Unary INC;
  typedef Unary DEC;
  typedef Unary NOT;

  // Encoding: [opcode: uint8_t][opcount: uint8_t] [[op: {Operand, Param}], opcount...]
  struct Binary {
    uint8_t opcode;
    uint8_t opcount;

    Operand opL; // left and dest / dest
    Operand opR; // right

    OpParam condition;
  };
  typedef Binary CMP;
  typedef Binary TEST;
  typedef Binary MOV;
  typedef Binary LEA;
  typedef Binary VAR;
  typedef Binary XCHG;
  typedef Binary POPCNT;
  typedef Binary LEN;
  typedef Binary TRANS;
  typedef Binary INV;
  typedef Binary NORM; // this may be a tenary

  // Encoding: [opcode: uint8_t][opcount: uint8_t] [[op: {Operand, Param}], opcount...]
  struct Tenary {
    uint8_t opcode;
    uint8_t opcount;

    Operand opD; // destination
    Operand opL; // left / src 0
    Operand opR; // right / src 1

    OpParam condition;
  };
  typedef Tenary CAS;
  typedef Tenary ADD;
  typedef Tenary SUB;
  typedef Tenary MUL;
  typedef Tenary DIV;
  typedef Tenary MOD;
  typedef Tenary AND;
  typedef Tenary OR;
  typedef Tenary XOR;
  typedef Tenary SHL;
  typedef Tenary SHR;
  typedef Tenary SAL;
  typedef Tenary SAR;
  typedef Tenary SETE;
  typedef Tenary GETE;
  typedef Tenary DOT;
  typedef Tenary CROSS;

  typedef Binary PPDEF;
  typedef Unary PPUDEF;
  typedef Binary PPIF;
  typedef Binary PPELIF;
  typedef Void PPEIF;
  typedef Void PPEIF;

  // Encoding: [opcode: uint8_t][strtableref: uint64_t]
  struct PPINC {
    uint8_t opcode;
    uint64_t file;
  };
  // Encoding: [opcode: uint8_t][strtableref: uint64_t]
  struct PPSEC {
    uint8_t opcode;
    uint64_t name;
  };
  // Encoding: [opcode: uint8_t][elementcount: uint64_t][elementsize: uint64_t][element: void*]
  struct PPDATA {
    uint8_t opcode;
    uint64_t count; // the amount of times to insert the data
    uint64_t len; // length of data in bytes.
    void *data;
  };
  // Encoding: [opcode: uint8_t][byteindex: uint64_t]
  struct PPPADD {
    uint8_t opcode;
    uint64_t byteindex;
  };
  namespace CPU {
    // CPU
    struct CtxChangeInt {
      uint8_t opcode;
      uint8_t opcount; // 0, 1 or 2
      
      uint8_t interrupt; // maybe more then 8 bits is needed

      Operand location;
      OpParam condition;
    };
    typedef CtxChangeInt INT;
    typedef VoidParam IRET;
    typedef VoidParam CLI;
    typedef VoidParam STI;
    typedef CtxChangeInt SYSCALL;
    typedef VoidParam SYSRET;
    typedef Unary RDTSC;

    // X86
    typedef VoidParam CPUID;
    typedef Binary RDMSR;
    typedef Binary WRMSR;
    
    // ARM
    typedef VoidParam SEV;
    typedef VoidParam WFE;
    typedef Binar MRS;
    typedef Binar MSR;
  };
};


// Helper functions

/**
* @brief Check if an opcode is a control flow instruction
* 
* @param op Opcode to check
* @return true if it's a control flow instruction
*/
inline bool isControlFlowOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::CF_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::CF_RANGE_END);
}

/**
* @brief Check if an opcode is a memory instruction
* 
* @param op Opcode to check
* @return true if it's a memory instruction
*/
inline bool isMemoryOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::MEM_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::MEM_RANGE_END);
}

/**
* @brief Check if an opcode is an arithmetic instruction
* 
* @param op Opcode to check
* @return true if it's an arithmetic instruction
*/
inline bool isArithmeticOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::ARITH_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::ARITH_RANGE_END);
}

/**
* @brief Check if an opcode is a bitwise instruction
* 
* @param op Opcode to check
* @return true if it's a bitwise instruction
*/
inline bool isBitwiseOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::BIT_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::BIT_RANGE_END);
}

/**
* @brief Check if an opcode is a mathematical instruction
* 
* @param op Opcode to check
* @return true if it's a mathematical instruction
*/
inline bool isMathOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::MD_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::MD_RANGE_END);
}

/**
* @brief Check if an opcode is a directive instruction
* 
* @param op Opcode to check
* @return true if it's a directive instruction
*/
inline bool isDirectiveOp(Opcode op) {
  return static_cast<uint8_t>(op) >= static_cast<uint8_t>(Opcode::DIR_RANGE_START) && 
          static_cast<uint8_t>(op) <= static_cast<uint8_t>(Opcode::DIR_RANGE_END);
}

/**
* @brief Check if a type is a fixed-width type
* 
* @param type Type to check
* @return true if it's a fixed-width type
*/
inline bool isFixedWidthType(TypeOpcode type) {
  return static_cast<uint8_t>(type) >= static_cast<uint8_t>(TypeOpcode::FW_RANGE_START) && 
          static_cast<uint8_t>(type) <= static_cast<uint8_t>(TypeOpcode::FW_RANGE_END);
}

/**
* @brief Check if a type is a platform-specific type
* 
* @param type Type to check
* @return true if it's a platform-specific type
*/
inline bool isPlatformType(TypeOpcode type) {
  return static_cast<uint8_t>(type) >= static_cast<uint8_t>(TypeOpcode::PLT_RANGE_START) && 
          static_cast<uint8_t>(type) <= static_cast<uint8_t>(TypeOpcode::PLT_RANGE_END);
}

/**
* @brief Check if a type is a COIL-specific type
* 
* @param type Type to check
* @return true if it's a COIL-specific type
*/
inline bool isCoilType(TypeOpcode type) {
  return static_cast<uint8_t>(type) >= static_cast<uint8_t>(TypeOpcode::COIL_RANGE_START) && 
          static_cast<uint8_t>(type) <= static_cast<uint8_t>(TypeOpcode::COIL_RANGE_END);
}

/**
* @brief Check if type control indicates an immediate value
* 
* @param ctrl Type control flags
* @return true if it indicates an immediate value
*/
inline bool isImmediate(uint8_t ctrl) {
  return (ctrl & TypeControl::IMM) != 0;
}

/**
* @brief Check if type control indicates a variable reference
* 
* @param ctrl Type control flags
* @return true if it indicates a variable reference
*/
inline bool isVariable(uint8_t ctrl) {
  return (ctrl & TypeControl::VAR) != 0;
}

/**
* @brief Check if type control indicates a symbol reference
* 
* @param ctrl Type control flags
* @return true if it indicates a symbol reference
*/
inline bool isSymbol(uint8_t ctrl) {
  return (ctrl & TypeControl::SYM) != 0;
}

/**
* @brief Check if type control indicates a register reference
* 
* @param ctrl Type control flags
* @return true if it indicates a register reference
*/
inline bool isRegister(uint8_t ctrl) {
  return (ctrl & TypeControl::REG) != 0;
}

/**
* @brief Get the name of a type
* 
* @param type Type opcode
* @return std::string Type name
*/
std::string getTypeName(TypeOpcode type);

/**
* @brief Get the name of an opcode
* 
* @param op Opcode
* @return std::string Opcode name
*/
std::string getOpcodeName(Opcode op);

} // namespace coil