/**
 * @file test_instr.cpp
 * @brief Tests for the COIL instruction set
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "coil/instr.hpp"
#include <vector>
#include <array>

TEST_CASE("Instruction creation", "[instr]") {
  SECTION("No operand instruction") {
    coil::Instruction instr = coil::createInstr(coil::Opcode::Nop);
    
    CHECK(instr.opcode == coil::Opcode::Nop);
    CHECK(instr.flag0 == coil::InstrFlag0::None);
    CHECK(instr.dest.type == coil::OperandType::None);
    CHECK(instr.src1.type == coil::OperandType::None);
    CHECK(instr.src2.type == coil::OperandType::None);
  }
  
  SECTION("Single operand instruction") {
    coil::Operand reg = coil::createRegOp(1, coil::ValueType::I32);
    coil::Instruction instr = coil::createInstr(coil::Opcode::Push, reg);
    
    CHECK(instr.opcode == coil::Opcode::Push);
    CHECK(instr.flag0 == coil::InstrFlag0::None);
    CHECK(instr.dest.type == coil::OperandType::Reg);
    CHECK(instr.dest.value_type == coil::ValueType::I32);
    CHECK(instr.dest.reg == 1);
    CHECK(instr.src1.type == coil::OperandType::None);
    CHECK(instr.src2.type == coil::OperandType::None);
  }
  
  SECTION("Two operand instruction") {
    coil::Operand dest = coil::createRegOp(1, coil::ValueType::I32);
    coil::Operand src = coil::createRegOp(2, coil::ValueType::I32);
    coil::Instruction instr = coil::createInstr(coil::Opcode::Add, dest, src);
    
    CHECK(instr.opcode == coil::Opcode::Add);
    CHECK(instr.flag0 == coil::InstrFlag0::None);
    CHECK(instr.dest.type == coil::OperandType::Reg);
    CHECK(instr.dest.reg == 1);
    CHECK(instr.src1.type == coil::OperandType::Reg);
    CHECK(instr.src1.reg == 2);
    CHECK(instr.src2.type == coil::OperandType::None);
  }
  
  SECTION("Three operand instruction") {
    coil::Operand dest = coil::createRegOp(1, coil::ValueType::I32);
    coil::Operand src1 = coil::createRegOp(2, coil::ValueType::I32);
    coil::Operand src2 = coil::createRegOp(3, coil::ValueType::I32);
    coil::Instruction instr = coil::createInstr(coil::Opcode::Add, dest, src1, src2);
    
    CHECK(instr.opcode == coil::Opcode::Add);
    CHECK(instr.flag0 == coil::InstrFlag0::None);
    CHECK(instr.dest.type == coil::OperandType::Reg);
    CHECK(instr.dest.reg == 1);
    CHECK(instr.src1.type == coil::OperandType::Reg);
    CHECK(instr.src1.reg == 2);
    CHECK(instr.src2.type == coil::OperandType::Reg);
    CHECK(instr.src2.reg == 3);
  }
  
  SECTION("With condition flag") {
    coil::Operand reg = coil::createRegOp(1, coil::ValueType::I32);
    coil::Instruction instr = coil::createInstr(coil::Opcode::Jump, reg, coil::InstrFlag0::EQ);
    
    CHECK(instr.opcode == coil::Opcode::Jump);
    CHECK(instr.flag0 == coil::InstrFlag0::EQ);
  }
}

TEST_CASE("Operand creation", "[instr]") {
  SECTION("Register operand") {
    coil::Operand op = coil::createRegOp(5, coil::ValueType::I64);
    
    CHECK(op.type == coil::OperandType::Reg);
    CHECK(op.value_type == coil::ValueType::I64);
    CHECK(op.modifiers == 0);
    CHECK(op.reg == 5);
  }
  
  SECTION("Immediate integer operands") {
    // Test integer immediate values of different sizes and types
    const std::array<std::pair<coil::ValueType, int64_t>, 8> tests = {{
      {coil::ValueType::I8, 42},
      {coil::ValueType::I16, 1000},
      {coil::ValueType::I32, 100000},
      {coil::ValueType::I64, 1000000000000LL},
      {coil::ValueType::U8, 200},
      {coil::ValueType::U16, 50000},
      {coil::ValueType::U32, 3000000000UL},
      {coil::ValueType::U64, 10000000000000ULL}
    }};
    
    for (const auto& [type, value] : tests) {
      coil::Operand op = coil::createImmOpInt(value, type);
      
      CHECK(op.type == coil::OperandType::Imm);
      CHECK(op.value_type == type);
      
      // Verify the value was stored correctly
      switch (type) {
        case coil::ValueType::I8:
          CHECK(op.imm.i8_val == static_cast<coil::i8>(value));
          break;
        case coil::ValueType::I16:
          CHECK(op.imm.i16_val == static_cast<coil::i16>(value));
          break;
        case coil::ValueType::I32:
          CHECK(op.imm.i32_val == static_cast<coil::i32>(value));
          break;
        case coil::ValueType::I64:
          CHECK(op.imm.i64_val == value);
          break;
        case coil::ValueType::U8:
          CHECK(op.imm.u8_val == static_cast<coil::u8>(value));
          break;
        case coil::ValueType::U16:
          CHECK(op.imm.u16_val == static_cast<coil::u16>(value));
          break;
        case coil::ValueType::U32:
          CHECK(op.imm.u32_val == static_cast<coil::u32>(value));
          break;
        case coil::ValueType::U64:
          CHECK(op.imm.u64_val == static_cast<coil::u64>(value));
          break;
        default:
          FAIL("Unexpected value type");
      }
    }
  }
  
  SECTION("Immediate float operands") {
    // F32 operand
    coil::Operand op32 = coil::createImmOpFp(3.14159, coil::ValueType::F32);
    CHECK(op32.type == coil::OperandType::Imm);
    CHECK(op32.value_type == coil::ValueType::F32);
    CHECK(op32.imm.f32_val == Catch::Approx(3.14159f));
    
    // F64 operand
    coil::Operand op64 = coil::createImmOpFp(3.14159265358979, coil::ValueType::F64);
    CHECK(op64.type == coil::OperandType::Imm);
    CHECK(op64.value_type == coil::ValueType::F64);
    CHECK(op64.imm.f64_val == Catch::Approx(3.14159265358979));
  }
  
  SECTION("Memory operand") {
    coil::Operand op = coil::createMemOp(1, 100, coil::ValueType::I32);
    
    CHECK(op.type == coil::OperandType::Mem);
    CHECK(op.value_type == coil::ValueType::I32);
    CHECK(op.modifiers == 0);
    CHECK(op.mem.base == 1);
    CHECK(op.mem.offset == 100);
  }
  
  SECTION("Label operand") {
    coil::Operand op = coil::createLabelOp(42);
    
    CHECK(op.type == coil::OperandType::Label);
    CHECK(op.value_type == coil::ValueType::Void);
    CHECK(op.modifiers == 0);
    CHECK(op.label == 42);
  }
}

TEST_CASE("InstructionBlock operations", "[instr]") {
  coil::InstructionBlock block;
  
  // Check initial state
  CHECK(block.getInstructionCount() == 0);
  CHECK(block.getData() != nullptr);
  
  SECTION("Adding instructions") {
    // Create a few instructions
    coil::Instruction nop = coil::createInstr(coil::Opcode::Nop);
    coil::Operand reg1 = coil::createRegOp(1, coil::ValueType::I32);
    coil::Instruction push = coil::createInstr(coil::Opcode::Push, reg1);
    
    // Add to block
    uint32_t idx1 = block.addInstruction(nop);
    uint32_t idx2 = block.addInstruction(push);
    
    // Verify indices
    CHECK(idx1 == 0);
    CHECK(idx2 == 1);
    
    // Verify count
    CHECK(block.getInstructionCount() == 2);
    
    // Retrieve instructions
    const coil::Instruction* instr1 = block.getInstruction(0);
    const coil::Instruction* instr2 = block.getInstruction(1);
    
    REQUIRE(instr1 != nullptr);
    REQUIRE(instr2 != nullptr);
    
    // Verify first instruction
    CHECK(instr1->opcode == coil::Opcode::Nop);
    
    // Verify second instruction
    CHECK(instr2->opcode == coil::Opcode::Push);
    CHECK(instr2->dest.type == coil::OperandType::Reg);
    CHECK(instr2->dest.reg == 1);
  }
  
  SECTION("Adding many instructions") {
    // Add 100 instructions to test dynamic resizing
    std::vector<uint32_t> indices;
    
    for (int i = 0; i < 100; i++) {
      coil::Operand reg = coil::createRegOp(i, coil::ValueType::I32);
      coil::Instruction instr = coil::createInstr(coil::Opcode::Push, reg);
      
      uint32_t idx = block.addInstruction(instr);
      indices.push_back(idx);
    }
    
    // Verify count
    CHECK(block.getInstructionCount() == 100);
    
    // Verify each instruction
    for (int i = 0; i < 100; i++) {
      const coil::Instruction* instr = block.getInstruction(i);
      REQUIRE(instr != nullptr);
      CHECK(instr->opcode == coil::Opcode::Push);
      CHECK(instr->dest.reg == i);
    }
    
    // Verify indices
    for (int i = 0; i < 100; i++) {
      CHECK(indices[i] == i);
    }
    
    // Get all instructions
    const auto& instrs = block.getInstructions();
    CHECK(instrs.size() == 100);
  }
  
  SECTION("Retrieving invalid instruction") {
    // Try to get instruction at invalid index
    const coil::Instruction* instr = block.getInstruction(100);
    CHECK(instr == nullptr);
  }
}