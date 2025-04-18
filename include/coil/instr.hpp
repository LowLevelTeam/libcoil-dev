/**
 * @file instr.hpp
 * @brief COIL instruction set definitions
 */

#pragma once
#include "coil/types.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace coil {

/**
 * @brief Value types supported by COIL
 */
enum class ValueType : u8 {
  // Signed Integer (0x00-0x0F)
  I8 = 0x00,  // 8-bit signed integer
  I16 = 0x01, // 16-bit signed integer
  I32 = 0x02, // 32-bit signed integer
  I64 = 0x03, // 64-bit signed integer
  
  // Unsigned Integer (0x10-0x1F)
  U8 = 0x10,  // 8-bit unsigned integer
  U16 = 0x11, // 16-bit unsigned integer
  U32 = 0x12, // 32-bit unsigned integer
  U64 = 0x13, // 64-bit unsigned integer
  
  // Floating Point (0x20-0x2F)
  F32 = 0x20, // 32-bit float (IEEE-754)
  F64 = 0x21, // 64-bit float
  
  // Reserved (0x30-BF)

  // Composite (C0-CF)

  // Not Implemented Yet (requires a way to encode type info into the operand)
  // STRUCT  = 0xC0, // Structure of types
  // ASTRUCT = 0xC1, // Anonymous/inline structure of types
  // UNION   = 0xC2, // Union of types
  // AUNION  = 0xC3, // Anonymous/inline union of types
  // PACK    = 0xC4, // Pack of types (structure with no padding)
  // APACK   = 0xC5, // Anonymous/inline pack of types
  // ALIAS   = 0xC6, // Alias type (real type is stored at type id)

  // Complex (D0-DF)

  // Not Implemented Yet (requires a way to encode type info into the operand)
  // ARR     = 0xD0, // A variable size array
  // INT     = 0xD1, // A variable bit width signed integer
  // UNT     = 0xD2, // A variable bit width unsigned integer
  // FP      = 0xD3, // A variable bit width floating point
  // DINT    = 0xD4, // A dynamic bit width signed integer
  // DUNT    = 0xD5, // A dynamic bit width unsigned integer
  // DFP     = 0xD6, // A dynamic bit width floating point

  // Platform Types
  PTR = 0xE0,   // Pointer type (platform width)
  SIZE = 0xE1,  // Unsigned type (platform width)
  SSIZE = 0xE2, // Signed type (platform width)

  // COIL Types
  VAR = 0xF0, // Value is u64 Variable ID
  SYM = 0xF1, // Value is u64 symbol reference
  EXP = 0xF2, // Value is u64 Expression ID
  REG = 0xF3, // Value is u32 register reference
  STR = 0xF4, // Value is u64 string table offset
  
  // Instruction Flags
  Flag0 = 0xFA,
  Flag1 = 0xFB,
  Flag2 = 0xFC,
  Flag3 = 0xFD,

  // Bit Type
  Bit = 0xFE,

  // Void Type
  Void = 0xFF, // Void type
};

/**
 * @brief Instruction Flag0 (for conditional execution)
 */
enum class InstrFlag0 : u8 {
  None = 0x00,
  EQ = 0x01,
  NEQ = 0x02,
  GT = 0x03,
  GTE = 0x04,
  LT = 0x05,
  LTE = 0x06,
  O = 0x07, // Overflow
  NO = 0x08, // Not overflow
  S = 0x09, // Sign
  NS = 0x0A, // Not sign
  Z = 0x0B, // Zero
  NZ = 0x0C, // Not zero
  C = 0x0D, // Carry
  NC = 0x0E, // No Carry
  P = 0x0F, // Parity
  PE = 0x10, // Parity Even
  NP = 0x0F, // Not Parity
  PO = 0x10, // Parity Odd
};

/**
 * @brief Value type modifiers
 */
enum class TypeModifier : u8 {
  None     = 0,      // No modifiers
  Const    = 1 << 0, // Constant value (value should not be changed)
  Volatile = 1 << 1, // Volatile value (value could change unexpectedly)
  Atomic   = 1 << 2, // Atomic access (value requires atomic access)
  Mutable  = 1 << 3, // Value can change (finds use cases in composite types and possible uses in the future)
};

// Bitwise operators for TypeModifier
inline TypeModifier operator|(TypeModifier a, TypeModifier b) {
  return static_cast<TypeModifier>(
    static_cast<u8>(a) | static_cast<u8>(b)
  );
}

inline bool operator&(TypeModifier a, TypeModifier b) {
  return (static_cast<u8>(a) & static_cast<u8>(b)) != 0;
}

/**
 * @brief Instruction opcodes
 */
enum class Opcode : u8 {
  // Control Flow operations (0x00-0x0F)
  NOP = 0x00,    // No operation
  BR = 0x01,     // Branch (conditional jump)
  JMP = 0x02,    // Unconditional jump
  CALL = 0x03,   // Call function
  RET = 0x04,    // Return from function
  CMP = 0x05,    // Compare (sets flags)
  TEST = 0x06,   // Test (sets flags)
  // Reserved: 07-0F

  // Memory Operations (0x10-0x2F)
  MOV = 0x10,    // Copy value from source to destination (MOV has been used for copying since the creation of assembly languages)
  PUSH = 0x11,   // Push onto stack
  POP = 0x12,    // Pop from stack
  LEA = 0x13,    // Load effective address
  LOAD = 0x14,   // Load from memory
  STOR = 0x15,   // Store from memory
  // Reserved: 15-2F

  // Arithmetic (0x30-0x4F)
  ADD = 0x30,    // Addition
  SUB = 0x31,    // Subtraction
  MUL = 0x32,    // Multiplication
  DIV = 0x33,    // Division
  MOD = 0x34,    // Remainder
  INC = 0x35,    // Increment
  DEC = 0x36,    // Decrement
  NEG = 0x37,    // Negate a value
  // Reserved: 38-4F

  // Bitwise (0x50-5F)
  AND    = 0x50,  // Bitwise AND
  OR     = 0x51,  // Bitwise OR
  XOR    = 0x52,  // Bitwise XOR
  NOT    = 0x53,  // Bitwise NOT
  SHL    = 0x54,  // Shift left
  SHR    = 0x55,  // Shift right (logical)
  SAL    = 0x56,  // Shift arithmetic left
  SAR    = 0x57,  // Shift arithmetic right
  POPCNT = 0x58,  // Population count  
  // Reserved: 59-5F

  // Multi-Dimensional (0x60-0x6F)
  GETE  = 0x60,   // Get element
  SETE  = 0x61,   // Set element
  DOT   = 0x62,   // Dot product
  CROSS = 0x63,   // Cross product
  NORM  = 0x64,   // Normalize
  LEN   = 0x65,   // Length/magnitude
  TRANS = 0x66,   // Transpose
  INV   = 0x67,   // Invert/inverse

  // Crpytography and Random Numbers (0x70-0x7F)
  // Reserved: 70-7F

  // (Future Proof reservation) Reserved: 80-9F

  // Type (0xA0-0xAF)
  CVT   = 0xA0,   // Type Cast
  SIZE  = 0xA1,   // Sizeof Type
  ALIGN = 0xA2,   // Allign Types
  // Reserved: A3-AF

  // PU (0xB0-0xCF)
    // CPU
      INT = 0xB0, // Interrupt
      IRET = 0xB1, // Interrupt return
      CLI = 0xB2, // Stop interrupts
      STI = 0xB3, // Start interrupts
      SYSCALL = 0xB4, // Interrupt to supervisor from user
      SYSRET = 0xB5, // Return from supervisor interrupt
      RDTSC = 0xB6, // Read time-tamp counter
    // GPU
      // TODO...

  // Arch (0xD0-0xDF)
    // CPU
      // x86
        CPUID = 0xD0,
        RDMSR = 0xD1,
        WRMSR = 0xD2,
        LGDT = 0xD3,
        SGDT = 0xD4,
        LIDT = 0xD5,
        SIDT = 0xD6,
        RDPMC = 0xD7,
      // arm
        SEV = 0xD0,
        WFE = 0xD1,
        MRS = 0xD2,
        MSR = 0xD3,
    // GPU
      // AMD
        // TODO...
      // NVIDIA
        // TODO...
      // INTEL

  // Directive (0xE0-0xFE)
  DEF   = 0xE0,   // Define an expression
  UDEF  = 0xE1,   // Undefine an expression
  ISDEF = 0xE2,   // Check if an expression exists
  DATA  = 0xE3,   // Insert raw bytes into the program
  PADD  = 0xE4,   // Padd program to byte offset (byte offset represents the native binary offset not the COIL file, this is to provide compatibiltiy with x86 bios bootloaders)
  // Reserved: E5-EF
  ABI   = 0xF0,   // Define an ABI
  SPARAM = 0xF1,  // Set the parameter value utilizing the current ABI (used in the caller)
  GPARAM = 0xF2,  // Get the parameter value utilizing the current ABI (used in the callee)
  SRET   = 0xF3,  // Set the return value utilizing the current ABI (used in the callee)
  GRET   = 0xF4,  // Get the return value utilizing the current ABI (used in the caller)
  // Reserved: F5-FE
  
  // Attribute (0xFF)
  PPEXT  = 0xFF   ///< Extension (similar to GNU __attribute__)
};

/**
* @brief Operand types
* Encoded as a u4
*/
enum class OperandType : u8 {
  None = 0x00, // No operand
  Reg = 0x01,  // Register
  Var = 0x02,  // Variable reference
  Exp = 0x03,  // Expression reference
  Imm = 0x04,  // Immediate value
  Sym = 0x05,  // Symbol reference
  Off = 0x06,  // The instruction includes offsets then another 4 bits follows this for the actual operand type
  // Reserved 0x06-0x0F
};

/**
 * @brief Immediate value union
 */
union ImmediateValue {
  i8  _i8;
  i16 _i16;
  i32 _i32;
  i64 _i64;
  
  u8  _u8;
  u16 _u16;
  u32 _u32;
  u64 _u64;

  f32 _f32;
  f64 _f64;

  void *_ptr;
  unsigned long long int _size;
  signed long long int _ssize;

  u64 _varId;
  u64 _symref;
  u64 _exprid;
  u32 _regref;
  u64 _strref; 

  InstrFlag0 flag;

  bool _bit;
};

/**
 * @brief Instruction operand
 */
struct Operand {
  OperandType  type;       // Operand type
  OperandType  rtype;
  ValueType    value_type; // Value type
  TypeModifier modifiers;  // Type modifiers
  
  union {
    u64             sym; // Symbol Reference
    u64             var; // Variable ID
    u64             exp; // Expression ID
    u32             reg; // Register index
    ImmediateValue  imm; // Immediate value
  }; // base

  // optional for memory referencing
  i64 index;
  i64 scale;
  i64 displacement;
  // base + (index * scale) + displacement
  
  // Default constructor  
  Operand() : 
    type(OperandType::None), 
    value_type(ValueType::Void), 
    modifiers(0), index(0), scale(0), displacement(0)
  {}

  Operand(OperandType type, ValueType valtype, TypeModifier modifiers)
    : type(type), value_type(valtype), modifiers(modifiers)
  {}

  static Operand createSym(u64 symref, ValueType type, TypeModifier mod) { 
    Operand op(OperandType::Sym, type, mod); 
    op.sym = symref; 
    return op; 
  }
  static Operand createVar(u64 varid, ValueType type, TypeModifier mod) { 
    Operand op(OperandType::Var, type, mod); 
    op.var = varid; 
    return op; 
  }
  static Operand createExp(u64 expid, ValueType type, TypeModifier mod) { 
    Operand op(OperandType::Exp, type, mod); 
    op.exp = expid; 
    return op; 
  }
  static Operand createReg(u32 reg, ValueType type, TypeModifier mod) { 
    Operand op(OperandType::Reg, type, mod); 
    op.reg = reg; 
    return op; 
  }
  static Operand createImm(ImmediateValue value, ValueType type, TypeModifier mod) { 
    Operand op(OperandType::Imm, type, mod); 
    op.imm = value; 
    return op; 
  }

  void addOffsets(i64 index, i64 scale, i64 disp) { 
    this->type = OperandType::Off;
    this->rtype = type;
    this->index = index; this->scale = scale; this->displacement = disp; 
  }
};

/**
 * @brief Single instruction
 * 
 * A compact instruction representation with up to three operands.
 * Not all instructions use all operands.
 */
struct Instruction {
  Opcode     opcode;  // Operation code
  u8 operand_count;   // Operands

  InstrFlag0 flag0;   // Instruction flag 0

  Operand    dest;    // Destination operand
  Operand    src1;    // Source operand 1
  Operand    src2;    // Source operand 2
  
  /**
  * @brief Default Constructor
  */
  Instruction() : 
    opcode(Opcode::Nop),
    flag0(InstrFlag0::None)
  {}

  /**
  * @brief Create an instruction
  */
  Instruction(Opcode op, InstrFlag0 _flag0) : opcode(op), flag0(_flag0) {}

  void addDestOperand(const Operand& op) { this->dest = op; }
  void addLeftOperand(const Operand& op) { this->src1 = op; }
  void addRightOperand(const Operand& op) { this->src2 = op; }

  void addCondition(InstrFlag0 flag) { this->flag0 = flag; }
};

/**
 * @brief A block of instructions
 */
class InstructionBlock {
public:
  /**
   * @brief Create an empty instruction block
   */
  InstructionBlock();
  
  /**
   * @brief Add an instruction to the block
   * @return Index of the instruction
   */
  u32 addInstruction(const Instruction& instr);
  
  /**
   * @brief Get an instruction by index
   */
  const Instruction* getInstruction(u32 index) const;
  
  /**
   * @brief Get number of instructions
   */
  u32 getInstructionCount() const { return static_cast<u32>(m_instructions.size()); }
  
  /**
   * @brief Get raw instruction data
   */
  const Instruction* getData() const { 
    return m_instructions.data(); 
  }
  
  /**
   * @brief Get all instructions
   */
  const std::vector<Instruction>& getInstructions() const { 
    return m_instructions; 
  }
  
private:
  std::vector<Instruction> m_instructions;
};

} // namespace coil