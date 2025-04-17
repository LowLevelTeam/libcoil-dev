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
  // Integer types
  I8 = 0,    // 8-bit signed integer
  I16 = 1,   // 16-bit signed integer
  I32 = 2,   // 32-bit signed integer
  I64 = 3,   // 64-bit signed integer
  
  // Unsigned integer types
  U8 = 4,    // 8-bit unsigned integer
  U16 = 5,   // 16-bit unsigned integer
  U32 = 6,   // 32-bit unsigned integer
  U64 = 7,   // 64-bit unsigned integer
  
  // Floating point types
  F32 = 8,   // 32-bit float
  F64 = 9,   // 64-bit float
  
  // Special types
  Ptr = 10,  // Pointer type (platform width)
  Void = 11, // Void type
};

/**
 * @brief Instruction Flag0 (for conditional execution)
 */
enum class InstrFlag0 : u8 {
  None = 0,
  EQ = 1,
  NEQ = 2,
  GT = 3,
  GTE = 4,
  LT = 5,
  LTE = 6,
  // TODO: Add more...
};

/**
 * @brief Value type modifiers
 */
enum class TypeModifier : u8 {
  None     = 0,      // No modifiers
  Const    = 1 << 0, // Constant value
  Volatile = 1 << 1, // Volatile value
  Atomic   = 1 << 2, // Atomic access
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
  // Control flow
  Nop = 0,    // No operation
  Br = 1,     // Branch (conditional jump)
  Jump = 2,   // Unconditional jump
  Call = 3,   // Call function
  Ret = 4,    // Return from function
  
  // Memory ops
  Load = 5,   // Load from memory
  Store = 6,  // Store to memory
  Push = 7,   // Push onto stack
  Pop = 8,    // Pop from stack
  
  // Arithmetic
  Add = 9,    // Addition
  Sub = 10,   // Subtraction
  Mul = 11,   // Multiplication
  Div = 12,   // Division
  Rem = 13,   // Remainder
  Inc = 14,   // Increment
  Dec = 15,   // Decrement
  
  // Bitwise
  And = 16,   // Bitwise AND
  Or = 17,    // Bitwise OR
  Xor = 18,   // Bitwise XOR
  Not = 19,   // Bitwise NOT
  Shl = 20,   // Shift left
  Shr = 21,   // Shift right (logical)
  Sar = 22,   // Shift arithmetic right
  
  // Comparison
  Cmp = 23,   // Compare (sets flags)
  Test = 24,  // Test (sets flags)
};

/**
 * @brief Operand types
 */
enum class OperandType : u8 {
  None = 0,   // No operand
  Reg = 1,    // Register
  Imm = 2,    // Immediate value
  Mem = 3,    // Memory reference
  Label = 4,  // Label reference
};

/**
 * @brief Immediate value union
 */
union ImmediateValue {
  i8  i8_val;
  i16 i16_val;
  i32 i32_val;
  i64 i64_val;
  
  u8  u8_val;
  u16 u16_val;
  u32 u32_val;
  u64 u64_val;

  f32 f32_val;
  f64 f64_val;
};

/**
 * @brief Memory reference structure
 */
struct MemoryReference {
  u32 base;    // Base register
  i32 offset;  // Memory offset
};

/**
 * @brief Instruction operand
 */
struct Operand {
  OperandType type;       // Operand type
  ValueType   value_type; // Value type
  u8          modifiers;  // Type modifiers (TypeModifier flags)
  
  union {
    u32             reg;    // Register index
    ImmediateValue  imm;    // Immediate value
    MemoryReference mem;    // Memory reference
    u32             label;  // Label index
  };
  
  // Default constructor
  Operand() 
    : type(OperandType::None), 
      value_type(ValueType::Void), 
      modifiers(0), 
      reg(0) {}
};

/**
 * @brief Single instruction
 * 
 * A compact instruction representation with up to three operands.
 * Not all instructions use all operands.
 */
struct Instruction {
  Opcode     opcode;  // Operation code
  InstrFlag0 flag0;   // Instruction flag 0
  Operand    dest;    // Destination operand
  Operand    src1;    // Source operand 1
  Operand    src2;    // Source operand 2
  
  // Default constructor
  Instruction() 
    : opcode(Opcode::Nop), 
      flag0(InstrFlag0::None) {}
};

/**
 * @brief Create an instruction with no operands
 */
Instruction createInstr(Opcode op, InstrFlag0 flag0 = InstrFlag0::None);

/**
 * @brief Create an instruction with one operand
 */
Instruction createInstr(Opcode op, const Operand& dest, InstrFlag0 flag0 = InstrFlag0::None);

/**
 * @brief Create an instruction with two operands
 */
Instruction createInstr(Opcode op, const Operand& dest, const Operand& src, InstrFlag0 flag0 = InstrFlag0::None);

/**
 * @brief Create an instruction with three operands
 */
Instruction createInstr(Opcode op, const Operand& dest, const Operand& src1, const Operand& src2, InstrFlag0 flag0 = InstrFlag0::None);

/**
 * @brief Create a register operand
 */
Operand createRegOp(u32 reg, ValueType type);

/**
 * @brief Create an immediate integer operand
 */
Operand createImmOpInt(i64 value, ValueType type);

/**
 * @brief Create an immediate float operand
 */
Operand createImmOpFp(f64 value, ValueType type);

/**
 * @brief Create a memory operand
 */
Operand createMemOp(u32 base, i32 offset, ValueType type);

/**
 * @brief Create a label operand
 */
Operand createLabelOp(u32 label);

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