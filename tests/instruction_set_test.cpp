#include "test_helper.h"

bool testOperandCreation() {
  // Test variable operand
  coil::Operand varOperand = coil::Operand::createVariable(42);
  TEST_ASSERT(varOperand.getType() == coil::Type::VAR);
  TEST_ASSERT(varOperand.getValue().size() == 2);  // 2 bytes for variable ID
  TEST_ASSERT(varOperand.getValue()[0] == 42 && varOperand.getValue()[1] == 0);  // Little endian
  
  // Test immediate operands
  coil::Operand int8Operand = coil::Operand::createImmediate<int8_t>(-42);
  TEST_ASSERT((int8Operand.getType() & ~coil::Type::IMM) == coil::Type::INT8);
  TEST_ASSERT(int8Operand.getValue().size() == 1);
  TEST_ASSERT(static_cast<int8_t>(int8Operand.getValue()[0]) == -42);
  
  coil::Operand uint32Operand = coil::Operand::createImmediate<uint32_t>(0x12345678);
  TEST_ASSERT((uint32Operand.getType() & ~coil::Type::IMM) == coil::Type::UNT32);
  TEST_ASSERT(uint32Operand.getValue().size() == 4);
  TEST_ASSERT(uint32Operand.getValue()[0] == 0x78);
  TEST_ASSERT(uint32Operand.getValue()[1] == 0x56);
  TEST_ASSERT(uint32Operand.getValue()[2] == 0x34);
  TEST_ASSERT(uint32Operand.getValue()[3] == 0x12);
  
  // Test symbol operand
  coil::Operand symbolOperand = coil::Operand::createSymbol(123);
  TEST_ASSERT(symbolOperand.getType() == coil::Type::SYM);
  TEST_ASSERT(symbolOperand.getValue().size() == 2);
  TEST_ASSERT(symbolOperand.getValue()[0] == 123 && symbolOperand.getValue()[1] == 0);
  
  // Test register operand
  coil::Operand regOperand = coil::Operand::createRegister(5, coil::Type::RGP);
  TEST_ASSERT(regOperand.getType() == coil::Type::RGP);
  TEST_ASSERT(regOperand.getValue().size() == 2);
  TEST_ASSERT(regOperand.getValue()[0] == 5 && regOperand.getValue()[1] == 0);
  
  // Test memory operand
  coil::Operand memOperand = coil::Operand::createMemory(1, 2, 4, 0x100);
  TEST_ASSERT(memOperand.getType() == coil::Type::PTR);
  TEST_ASSERT(memOperand.getValue().size() == 9);  // base(2) + index(2) + scale(1) + disp(4)
  TEST_ASSERT(memOperand.getValue()[0] == 1);  // Base register (low byte)
  TEST_ASSERT(memOperand.getValue()[2] == 2);  // Index register (low byte)
  TEST_ASSERT(memOperand.getValue()[4] == 4);  // Scale
  TEST_ASSERT(memOperand.getValue()[5] == 0x00);  // Displacement (little endian)
  TEST_ASSERT(memOperand.getValue()[6] == 0x01);
  
  return true;
}

bool testOperandEncoding() {
  // Create an operand and test encoding/decoding
  coil::Operand original = coil::Operand::createImmediate<int32_t>(0x12345678);
  
  // Encode to binary
  std::vector<uint8_t> encoded = original.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Operand decoded = coil::Operand::decode(encoded, offset);
  
  // Check results
  TEST_ASSERT(decoded.getType() == original.getType());
  TEST_ASSERT(decoded.getValue().size() == original.getValue().size());
  TEST_ASSERT(std::equal(decoded.getValue().begin(), decoded.getValue().end(), original.getValue().begin()));
  
  // Check that offset was advanced correctly
  TEST_ASSERT(offset == encoded.size());
  
  return true;
}

bool testInstructionCreation() {
  // Create a simple instruction: ADD result, a, b
  std::vector<coil::Operand> addOperands = {
      coil::Operand::createVariable(1),  // result
      coil::Operand::createVariable(2),  // a
      coil::Operand::createVariable(3)   // b
  };
  
  coil::Instruction addInstr(coil::Opcode::ADD, addOperands);
  
  // Check properties
  TEST_ASSERT(addInstr.getOpcode() == coil::Opcode::ADD);
  TEST_ASSERT(addInstr.getOperands().size() == 3);
  TEST_ASSERT(addInstr.getInstructionName() == "ADD");
  
  // Create another instruction: MOV a, imm
  std::vector<coil::Operand> movOperands = {
      coil::Operand::createVariable(1),
      coil::Operand::createImmediate<int32_t>(42)
  };
  
  coil::Instruction movInstr(coil::Opcode::MOV, movOperands);
  
  // Check properties
  TEST_ASSERT(movInstr.getOpcode() == coil::Opcode::MOV);
  TEST_ASSERT(movInstr.getOperands().size() == 2);
  TEST_ASSERT(movInstr.getInstructionName() == "MOV");
  
  return true;
}

bool testInstructionEncoding() {
  // Create a simple instruction: ADD result, a, b
  std::vector<coil::Operand> operands = {
      coil::Operand::createVariable(1),
      coil::Operand::createVariable(2),
      coil::Operand::createVariable(3)
  };
  
  coil::Instruction original(coil::Opcode::ADD, operands);
  
  // Encode to binary
  std::vector<uint8_t> encoded = original.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Instruction decoded = coil::Instruction::decode(encoded, offset);
  
  // Check results
  TEST_ASSERT(decoded.getOpcode() == original.getOpcode());
  TEST_ASSERT(decoded.getOperands().size() == original.getOperands().size());
  
  // Check operands
  for (size_t i = 0; i < original.getOperands().size(); i++) {
      TEST_ASSERT(decoded.getOperands()[i].getType() == original.getOperands()[i].getType());
      TEST_ASSERT(decoded.getOperands()[i].getValue().size() == original.getOperands()[i].getValue().size());
      TEST_ASSERT(std::equal(
          decoded.getOperands()[i].getValue().begin(),
          decoded.getOperands()[i].getValue().end(),
          original.getOperands()[i].getValue().begin()
      ));
  }
  
  // Check that offset was advanced correctly
  TEST_ASSERT(offset == encoded.size());
  
  return true;
}

bool testInstructionValidation() {
  // Test valid instructions
  {
      std::vector<coil::Operand> operands = {
          coil::Operand::createVariable(1),
          coil::Operand::createVariable(2),
          coil::Operand::createVariable(3)
      };
      coil::Instruction instr(coil::Opcode::ADD, operands);
      TEST_ASSERT(instr.validate());
      TEST_ASSERT(coil::InstructionSet::validateInstruction(instr));
  }
  
  // Test invalid opcode
  {
      std::vector<coil::Operand> operands = {
          coil::Operand::createVariable(1),
          coil::Operand::createVariable(2)
      };
      coil::Instruction instr(0xFF, operands);  // Invalid opcode
      TEST_ASSERT(!instr.validate());
      TEST_ASSERT(!coil::InstructionSet::validateInstruction(instr));
  }
  
  // Test incorrect operand count for non-variable operand count instructions
  {
      std::vector<coil::Operand> operands = {
          coil::Operand::createVariable(1)
      };
      coil::Instruction instr(coil::Opcode::ADD, operands);  // ADD needs 3 operands
      TEST_ASSERT(!instr.validate());
      TEST_ASSERT(!coil::InstructionSet::validateInstruction(instr));
  }
  
  // Test variable operand count instructions
  {
      // VAR instruction with 2 operands (minimum)
      std::vector<coil::Operand> varOperands2 = {
          coil::Operand::createVariable(1),
          coil::Operand::createImmediate<uint16_t>(coil::Type::INT32)
      };
      coil::Instruction varInstr2(coil::Opcode::VAR, varOperands2);
      TEST_ASSERT(varInstr2.validate());
      TEST_ASSERT(coil::InstructionSet::validateInstruction(varInstr2));
      
      // VAR instruction with 3 operands (with initialization)
      std::vector<coil::Operand> varOperands3 = {
          coil::Operand::createVariable(1),
          coil::Operand::createImmediate<uint16_t>(coil::Type::INT32),
          coil::Operand::createImmediate<int32_t>(42)
      };
      coil::Instruction varInstr3(coil::Opcode::VAR, varOperands3);
      TEST_ASSERT(varInstr3.validate());
      TEST_ASSERT(coil::InstructionSet::validateInstruction(varInstr3));
  }
  
  return true;
}

bool testInstructionSet() {
  // Test opcode lookup
  auto addOpcode = coil::InstructionSet::getOpcodeFromName("ADD");
  TEST_ASSERT(addOpcode.has_value());
  TEST_ASSERT(*addOpcode == coil::Opcode::ADD);
  
  auto unknownOpcode = coil::InstructionSet::getOpcodeFromName("UNKNOWN");
  TEST_ASSERT(!unknownOpcode.has_value());
  
  // Test instruction name lookup
  TEST_ASSERT(coil::InstructionSet::getInstructionName(coil::Opcode::ADD) == "ADD");
  TEST_ASSERT(coil::InstructionSet::getInstructionName(coil::Opcode::MOV) == "MOV");
  TEST_ASSERT(coil::InstructionSet::getInstructionName(0xFF) == "UNKNOWN");
  
  // Test opcode validation
  TEST_ASSERT(coil::InstructionSet::isValidOpcode(coil::Opcode::ADD));
  TEST_ASSERT(coil::InstructionSet::isValidOpcode(coil::Opcode::MOV));
  TEST_ASSERT(!coil::InstructionSet::isValidOpcode(0xFF));
  
  // Test expected operand count
  auto addCount = coil::InstructionSet::getExpectedOperandCount(coil::Opcode::ADD);
  TEST_ASSERT(addCount.has_value());
  TEST_ASSERT(*addCount == 3);
  
  auto movCount = coil::InstructionSet::getExpectedOperandCount(coil::Opcode::MOV);
  TEST_ASSERT(movCount.has_value());
  TEST_ASSERT(*movCount == 2);
  
  auto nopCount = coil::InstructionSet::getExpectedOperandCount(coil::Opcode::NOP);
  TEST_ASSERT(nopCount.has_value());
  TEST_ASSERT(*nopCount == 0);
  
  return true;
}

bool testSpecificInstructions() {
  // Test arithmetic instructions
  {
      // ADD result, a, b
      std::vector<coil::Operand> addOperands = {
          coil::Operand::createVariable(1),
          coil::Operand::createVariable(2),
          coil::Operand::createVariable(3)
      };
      coil::Instruction addInstr(coil::Opcode::ADD, addOperands);
      std::vector<uint8_t> encoded = addInstr.encode();
      
      // First byte should be opcode
      TEST_ASSERT(encoded[0] == coil::Opcode::ADD);
      
      // Second byte should be operand count
      TEST_ASSERT(encoded[1] == 3);
  }
  
  // Test memory operations
  {
      // MOV dest, src
      std::vector<coil::Operand> movOperands = {
          coil::Operand::createVariable(1),
          coil::Operand::createImmediate<int32_t>(42)
      };
      coil::Instruction movInstr(coil::Opcode::MOV, movOperands);
      std::vector<uint8_t> encoded = movInstr.encode();
      
      // First byte should be opcode
      TEST_ASSERT(encoded[0] == coil::Opcode::MOV);
      
      // Second byte should be operand count
      TEST_ASSERT(encoded[1] == 2);
  }
  
  // Test control flow instructions
  {
      // BR target
      std::vector<coil::Operand> brOperands = {
          coil::Operand::createSymbol(1)
      };
      coil::Instruction brInstr(coil::Opcode::BR, brOperands);
      std::vector<uint8_t> encoded = brInstr.encode();
      
      // First byte should be opcode
      TEST_ASSERT(encoded[0] == coil::Opcode::BR);
      
      // Second byte should be operand count
      TEST_ASSERT(encoded[1] == 1);
  }
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"Operand Creation", testOperandCreation},
      {"Operand Encoding", testOperandEncoding},
      {"Instruction Creation", testInstructionCreation},
      {"Instruction Encoding", testInstructionEncoding},
      {"Instruction Validation", testInstructionValidation},
      {"Instruction Set", testInstructionSet},
      {"Specific Instructions", testSpecificInstructions}
  };
  
  return runTests(tests);
}