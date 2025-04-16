/**
* @file instr.cpp
* @brief Implementation of COIL instruction set
*/

#include "coil/instr.hpp"
#include "coil/err.hpp"
#include <cstring>
#include <cstdlib>

namespace coil {

namespace {
  // Default initial capacity for instruction blocks
  constexpr u32 DEFAULT_CAPACITY = 32;
  
  // Growth factor when resizing instruction blocks
  constexpr float GROWTH_FACTOR = 1.5f;
}

// Create an instruction with no operands
Instruction createInstr(Opcode op, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  
  // Initialize operands to None
  instr.dest.type = OperandType::None;
  instr.src1.type = OperandType::None;
  instr.src2.type = OperandType::None;
  
  return instr;
}

// Create an instruction with one operand
Instruction createInstr(Opcode op, const Operand& dest, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  instr.dest = dest;
  
  // Initialize source operands to None
  instr.src1.type = OperandType::None;
  instr.src2.type = OperandType::None;
  
  return instr;
}

// Create an instruction with two operands
Instruction createInstr(Opcode op, const Operand& dest, const Operand& src, InstrFlag0 flag0) {
  Instruction instr;
  instr.opcode = op;
  instr.flag0 = flag0;
  instr.dest = dest;
  instr.src1 = src;
  
  // Initialize second source operand to None
  instr.src2.type = OperandType::None;
  
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
          COIL_REPORT_ERROR(ErrorLevel::Error, 
                     "Cannot create integer immediate for non-integer type %d", 
                     static_cast<int>(type));
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
          COIL_REPORT_ERROR(ErrorLevel::Error, 
                     "Cannot create floating point immediate for non-float type %d", 
                     static_cast<int>(type));
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

InstructionBlock::InstructionBlock() 
  : instructions(nullptr), capacity(0), count(0) {
  // Allocate initial instruction array
  instructions = static_cast<Instruction*>(malloc(DEFAULT_CAPACITY * sizeof(Instruction)));
  if (instructions) {
      capacity = DEFAULT_CAPACITY;
  } else {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate memory for instruction block");
  }
}

InstructionBlock::~InstructionBlock() {
  // Free the instruction array
  free(instructions);
  instructions = nullptr;
  capacity = 0;
  count = 0;
}

u32 InstructionBlock::addInstruction(const Instruction& instr) {
  // Check if we need to grow the array
  if (count >= capacity) {
      u32 new_capacity = static_cast<u32>(capacity * GROWTH_FACTOR) + 1;
      
      // Safety check for overflow
      if (new_capacity < capacity) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Instruction block capacity overflow");
          return count;
      }
      
      Instruction* new_instructions = static_cast<Instruction*>(
          realloc(instructions, new_capacity * sizeof(Instruction))
      );
      
      if (!new_instructions) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to resize instruction block");
          return count;  // Return current count, indicating failure
      }
      
      instructions = new_instructions;
      capacity = new_capacity;
  }
  
  // Add the instruction and increment count
  instructions[count] = instr;
  return count++;
}

const Instruction* InstructionBlock::getInstruction(u32 index) const {
  if (index < count) {
      return &instructions[index];
  }
  return nullptr;
}

u32 InstructionBlock::getInstructionCount() const {
  return count;
}

const Instruction* InstructionBlock::getData() const {
  return instructions;
}

} // namespace coil