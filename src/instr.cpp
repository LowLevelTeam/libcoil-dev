/**
 * @file instr.cpp
 * @brief Implementation of COIL instruction set
 */

#include "coil/instr.hpp"
#include "coil/err.hpp"
#include <algorithm>
#include <sstream>

namespace coil {

// Create an instruction with no operands
Instruction createInstr(Opcode op, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  
  // Initialize operands to None (already done by default constructor)
  return instr;
}

// Create an instruction with one operand
Instruction createInstr(Opcode op, const Operand& dest, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  instr.dest = dest;
  
  // Initialize source operands to None (already done by default constructor)
  return instr;
}

// Create an instruction with two operands
Instruction createInstr(Opcode op, const Operand& dest, const Operand& src, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  instr.dest = dest;
  instr.src1 = src;
  
  // Initialize second source operand to None (already done by default constructor)
  return instr;
}

// Create an instruction with three operands
Instruction createInstr(Opcode op, const Operand& dest, const Operand& src1, const Operand& src2, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  instr.dest = dest;
  instr.src1 = src1;
  instr.src2 = src2;
  
  return instr;
}

// Create a register operand
Operand createRegOp(u32 reg, ValueType type) {
  Operand op;
  op.type = OperandType::Reg;
  op.value_type = type;
  op.modifiers = 0;
  op.reg = reg;
  return op;
}

// Create an immediate integer operand
Operand createImmOpInt(i64 value, ValueType type) {
  Operand op;
  op.type = OperandType::Imm;
  op.value_type = type;
  op.modifiers = 0;
  
  // Set the appropriate field based on the value type
  switch (type) {
    case ValueType::I8:
      op.imm.i8_val = static_cast<i8>(value);
      break;
    case ValueType::I16:
      op.imm.i16_val = static_cast<i16>(value);
      break;
    case ValueType::I32:
      op.imm.i32_val = static_cast<i32>(value);
      break;
    case ValueType::I64:
      op.imm.i64_val = value;
      break;
    case ValueType::U8:
      op.imm.u8_val = static_cast<u8>(value);
      break;
    case ValueType::U16:
      op.imm.u16_val = static_cast<u16>(value);
      break;
    case ValueType::U32:
      op.imm.u32_val = static_cast<u32>(value);
      break;
    case ValueType::U64:
      op.imm.u64_val = static_cast<u64>(value);
      break;
    default:
      // Report error for incompatible types
      std::stringstream ss;
      ss << "Cannot create integer immediate for non-integer type " 
         << static_cast<int>(type);
      auto pos = COIL_CURRENT_POS;
      Logger::error(ss.str(), pos);
      break;
  }
  
  return op;
}

// Create an immediate float operand
Operand createImmOpFp(f64 value, ValueType type) {
  Operand op;
  op.type = OperandType::Imm;
  op.value_type = type;
  op.modifiers = 0;
  
  // Set the appropriate field based on the value type
  switch (type) {
    case ValueType::F32:
      op.imm.f32_val = static_cast<f32>(value);
      break;
    case ValueType::F64:
      op.imm.f64_val = value;
      break;
    default:
      // Report error for incompatible types
      std::stringstream ss;
      ss << "Cannot create floating point immediate for non-float type " 
         << static_cast<int>(type);
      auto pos = COIL_CURRENT_POS;
      Logger::error(ss.str(), pos);
      break;
  }
  
  return op;
}

// Create a memory operand
Operand createMemOp(u32 base, i32 offset, ValueType type) {
  Operand op;
  op.type = OperandType::Mem;
  op.value_type = type;
  op.modifiers = 0;
  op.mem.base = base;
  op.mem.offset = offset;
  return op;
}

// Create a label operand
Operand createLabelOp(u32 label) {
  Operand op;
  op.type = OperandType::Label;
  op.value_type = ValueType::Void;  // Labels don't have a specific value type
  op.modifiers = 0;
  op.label = label;
  return op;
}

// InstructionBlock implementation
InstructionBlock::InstructionBlock() {
  // Reserve some space for instructions
  m_instructions.reserve(32);
}

u32 InstructionBlock::addInstruction(const Instruction& instr) {
  // Add the instruction and return its index
  m_instructions.push_back(instr);
  return static_cast<u32>(m_instructions.size() - 1);
}

const Instruction* InstructionBlock::getInstruction(u32 index) const {
  if (index < m_instructions.size()) {
    return &m_instructions[index];
  }
  return nullptr;
}

} // namespace coil