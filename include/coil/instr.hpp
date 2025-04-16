/**
* @file instr.hpp
* @brief COIL instruction set definitions
*/

#pragma once
#include "coil/types.hpp"

namespace coil {

/**
* @brief Value types supported by COIL
*/
enum class ValueType : u8 {
  // Integer types
  I8,   ///< 8-bit signed integer
  I16,  ///< 16-bit signed integer
  I32,  ///< 32-bit signed integer
  I64,  ///< 64-bit signed integer
  
  // Unsigned integer types
  U8,   ///< 8-bit unsigned integer
  U16,  ///< 16-bit unsigned integer
  U32,  ///< 32-bit unsigned integer
  U64,  ///< 64-bit unsigned integer
  
  // Floating point types
  F32,  ///< 32-bit float
  F64,  ///< 64-bit float
  
  // Special types
  Ptr,  ///< Pointer type (platform width)
  Void, ///< Void type
};

/**
* @brief Instruction Flag0 (for conditional execution)
*/
enum class InstrFlag0 : u8 {
  None = 0,
  EQ,
  NEQ,
  GT,
  GTE,
  LT,
  LTE,
  /// TODO: Add more...
};

/**
* @brief Value type modifiers
*/
enum class TypeModifier : u8 {
  None     = 0,      ///< No modifiers
  Const    = 1 << 0, ///< Constant value
  Volatile = 1 << 1, ///< Volatile value
  Atomic   = 1 << 2, ///< Atomic access
};

/**
* @brief Instruction opcodes
*/
enum class Opcode : u8 {
  // Control flow
  Nop = 0,  ///< No operation
  Br,       ///< Branch (conditional jump)
  Jump,     ///< Unconditional jump
  Call,     ///< Call function
  Ret,      ///< Return from function
  
  // Memory ops
  Load,     ///< Load from memory
  Store,    ///< Store to memory
  Push,     ///< Push onto stack
  Pop,      ///< Pop from stack
  
  // Arithmetic
  Add,      ///< Addition
  Sub,      ///< Subtraction
  Mul,      ///< Multiplication
  Div,      ///< Division
  Rem,      ///< Remainder
  Inc,      ///< Increment
  Dec,      ///< Decrement
  
  // Bitwise
  And,      ///< Bitwise AND
  Or,       ///< Bitwise OR
  Xor,      ///< Bitwise XOR
  Not,      ///< Bitwise NOT
  Shl,      ///< Shift left
  Shr,      ///< Shift right (logical)
  Sar,      ///< Shift arithmetic right
  
  // Comparison
  Cmp,      ///< Compare (sets flags)
  Test,     ///< Test (sets flags)
};

/**
* @brief Operand types
*/
enum class OperandType : u8 {
  None,     ///< No operand
  Reg,      ///< Register
  Imm,      ///< Immediate value
  Mem,      ///< Memory reference
  Label,    ///< Label reference
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
* @brief Instruction operand
*/
struct Operand {
  OperandType type;      ///< Operand type
  ValueType   value_type;///< Value type
  u8          modifiers; ///< Type modifiers (TypeModifier flags)
  
  union {
      u32           reg;   ///< Register index
      ImmediateValue imm;  ///< Immediate value
      struct {
          u32 base;        ///< Base register
          i32 offset;      ///< Memory offset
      } mem;               ///< Memory reference
      u32          label;  ///< Label index
  };
};

/**
* @brief Single instruction
* 
* A compact instruction representation with up to three operands.
* Not all instructions use all operands.
*/
struct Instruction {
  Opcode     opcode;  ///< Operation code
  InstrFlag0 flag0;   ///< Instruction flag 0
  Operand    dest;    ///< Destination operand
  Operand    src1;    ///< Source operand 1
  Operand    src2;    ///< Source operand 2
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
Operand createMemOp(u32 base, i32 offset, ValueType type); // maybe add displacement and other memory offseting etc...

/**
* @brief Create a label operand
* TODO: Is this just a symbol?
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
    * @brief Destructor
    */
  ~InstructionBlock();
  
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
  u32 getInstructionCount() const;
  
  /**
    * @brief Get raw instruction data
    */
  const Instruction* getData() const;
  
private:
  Instruction* instructions;  ///< Instruction array
  u32 capacity;              ///< Array capacity
  u32 count;                 ///< Number of instructions
};

} // namespace coil