#include "coil/instruction_set.h"
#include "coil/type_system.h"
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

namespace coil {

// Operand implementation
Operand::Operand(uint16_t type, const std::vector<uint8_t>& value)
  : type_(type), value_(value) {
}

Operand Operand::createVariable(uint16_t varId) {
  std::vector<uint8_t> value(2);
  value[0] = varId & 0xFF;
  value[1] = (varId >> 8) & 0xFF;
  return Operand(Type::VAR, value);
}

Operand Operand::createImmediate(int32_t value, uint16_t type) {
  std::vector<uint8_t> data(4);
  data[0] = value & 0xFF;
  data[1] = (value >> 8) & 0xFF;
  data[2] = (value >> 16) & 0xFF;
  data[3] = (value >> 24) & 0xFF;
  return Operand(type | Type::IMM, data);
}

Operand Operand::createImmediate(int64_t value, uint16_t type) {
  std::vector<uint8_t> data(8);
  data[0] = value & 0xFF;
  data[1] = (value >> 8) & 0xFF;
  data[2] = (value >> 16) & 0xFF;
  data[3] = (value >> 24) & 0xFF;
  data[4] = (value >> 32) & 0xFF;
  data[5] = (value >> 40) & 0xFF;
  data[6] = (value >> 48) & 0xFF;
  data[7] = (value >> 56) & 0xFF;
  return Operand(type | Type::IMM, data);
}

Operand Operand::createImmediate(float value) {
  std::vector<uint8_t> data(4);
  memcpy(data.data(), &value, 4);
  return Operand(Type::FP32 | Type::IMM, data);
}

Operand Operand::createImmediate(double value) {
  std::vector<uint8_t> data(8);
  memcpy(data.data(), &value, 8);
  return Operand(Type::FP64 | Type::IMM, data);
}

Operand Operand::createSymbol(uint16_t symbolId) {
  std::vector<uint8_t> value(2);
  value[0] = symbolId & 0xFF;
  value[1] = (symbolId >> 8) & 0xFF;
  return Operand(Type::SYM, value);
}

Operand Operand::createRegister(uint16_t registerId, uint16_t registerType) {
  std::vector<uint8_t> value(2);
  value[0] = registerId & 0xFF;
  value[1] = (registerId >> 8) & 0xFF;
  return Operand(registerType, value);
}

Operand Operand::createMemory(uint16_t baseReg, uint16_t indexReg, uint8_t scale, int32_t displacement) {
  std::vector<uint8_t> value(9); // base(2) + index(2) + scale(1) + disp(4)
  
  // Base register
  value[0] = baseReg & 0xFF;
  value[1] = (baseReg >> 8) & 0xFF;
  
  // Index register
  value[2] = indexReg & 0xFF;
  value[3] = (indexReg >> 8) & 0xFF;
  
  // Scale
  value[4] = scale;
  
  // Displacement
  value[5] = displacement & 0xFF;
  value[6] = (displacement >> 8) & 0xFF;
  value[7] = (displacement >> 16) & 0xFF;
  value[8] = (displacement >> 24) & 0xFF;
  
  return Operand(Type::PTR, value);
}

std::vector<uint8_t> Operand::encode() const {
  std::vector<uint8_t> result;
  
  // Type (16 bits)
  result.push_back(type_ & 0xFF);
  result.push_back((type_ >> 8) & 0xFF);
  
  // Value (variable length)
  result.insert(result.end(), value_.begin(), value_.end());
  
  return result;
}

Operand Operand::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 2) {
      throw std::runtime_error("Insufficient data for Operand");
  }
  
  // Type
  uint16_t type = data[offset] | (data[offset + 1] << 8);
  offset += 2;
  
  // Value size depends on type
  size_t valueSize = 0;
  uint8_t mainType = TypeInfo::getMainType(type);
  
  if (type == Type::VAR || type == Type::SYM) {
      valueSize = 2; // Variable/Symbol ID (16 bits)
  } else if (mainType == TypeInfo::getMainType(Type::RGP) || 
              mainType == TypeInfo::getMainType(Type::RFP) || 
              mainType == TypeInfo::getMainType(Type::RV)) {
      valueSize = 2; // Register ID (16 bits)
  } else if (type & Type::IMM) {
      // Immediate value, size based on type
      valueSize = TypeInfo::getTypeSize(type & ~Type::IMM);
  } else if (mainType == TypeInfo::getMainType(Type::PTR)) {
      // Memory operand
      valueSize = 9; // base(2) + index(2) + scale(1) + disp(4)
  } else {
      // Default 4 bytes for unknown types
      valueSize = 4;
  }
  
  if (data.size() < offset + valueSize) {
      throw std::runtime_error("Insufficient data for Operand value");
  }
  
  // Extract value
  std::vector<uint8_t> value(data.begin() + offset, data.begin() + offset + valueSize);
  offset += valueSize;
  
  return Operand(type, value);
}

// Instruction implementation
Instruction::Instruction(uint8_t opcode, const std::vector<Operand>& operands)
  : opcode_(opcode), operands_(operands) {
}

std::vector<uint8_t> Instruction::encode() const {
  std::vector<uint8_t> result;
  
  // Opcode (8 bits)
  result.push_back(opcode_);
  
  // Operand count (8 bits)
  result.push_back(static_cast<uint8_t>(operands_.size()));
  
  // Operands (variable)
  for (const auto& operand : operands_) {
      auto encodedOperand = operand.encode();
      result.insert(result.end(), encodedOperand.begin(), encodedOperand.end());
  }
  
  return result;
}

Instruction Instruction::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 2) {
      throw std::runtime_error("Insufficient data for Instruction");
  }
  
  // Opcode
  uint8_t opcode = data[offset++];
  
  // Operand count
  uint8_t operandCount = data[offset++];
  
  // Operands
  std::vector<Operand> operands;
  operands.reserve(operandCount);
  
  for (uint8_t i = 0; i < operandCount; i++) {
      operands.push_back(Operand::decode(data, offset));
  }
  
  return Instruction(opcode, operands);
}

bool Instruction::validate() const {
  return InstructionSet::validateInstruction(*this);
}

size_t Instruction::getSize() const {
  // Opcode + Operand count
  size_t size = 2;
  
  // Add operand sizes
  for (const auto& operand : operands_) {
      // Type (16 bits) + value (variable)
      size += 2 + operand.getValue().size();
  }
  
  return size;
}

std::string Instruction::getInstructionName() const {
  return InstructionSet::getInstructionName(opcode_);
}

// InstructionSet implementation
std::optional<size_t> InstructionSet::getExpectedOperandCount(uint8_t opcode) {
  // Define expected operand counts for each opcode
  static const std::unordered_map<uint8_t, size_t> expectedCounts = {
      // Control Flow
      {Opcode::SYM, 1},
      {Opcode::BR, 1},
      {Opcode::CALL, 1},    // Variable with ABI
      {Opcode::RET, 0},     // Variable with ABI
      {Opcode::CMP, 2},
      {Opcode::SWITCH, 3},  // Variable
      
      // Memory Operations
      {Opcode::MOV, 2},
      {Opcode::PUSH, 1},
      {Opcode::POP, 1},
      {Opcode::LEA, 2},
      {Opcode::SCOPEE, 0},
      {Opcode::SCOPEL, 0},
      {Opcode::VAR, 2},     // Variable (can have initialization)
      {Opcode::MEMCPY, 3},
      {Opcode::MEMSET, 3},
      {Opcode::MEMCMP, 4},
      {Opcode::XCHG, 2},
      {Opcode::CAS, 3},
      
      // Bit Manipulation
      {Opcode::AND, 3},
      {Opcode::OR, 3},
      {Opcode::XOR, 3},
      {Opcode::NOT, 2},
      {Opcode::SHL, 3},
      {Opcode::SHR, 3},
      {Opcode::SAR, 3},
      
      // Arithmetic
      {Opcode::ADD, 3},
      {Opcode::SUB, 3},
      {Opcode::MUL, 3},
      {Opcode::DIV, 3},
      {Opcode::MOD, 3},
      {Opcode::INC, 1},
      {Opcode::DEC, 1},
      {Opcode::NEG, 2},
      {Opcode::ABS, 2},
      {Opcode::SQRT, 2},
      {Opcode::CEIL, 2},
      {Opcode::FLOOR, 2},
      {Opcode::ROUND, 2},
      
      // Vector/Array
      {Opcode::VLOAD, 2},
      {Opcode::VSTORE, 2},
      {Opcode::VEXTRACT, 3},
      {Opcode::VINSERT, 4},
      {Opcode::VDOT, 3},
      
      // Type Operations
      {Opcode::TYPEOF, 2},
      {Opcode::SIZEOF, 2},
      {Opcode::CONVERT, 2},
      {Opcode::CAST, 2},
      {Opcode::GET, 3},
      {Opcode::SET, 3},
      {Opcode::INDEX, 3},
      {Opcode::UPDT, 3},
      
      // Directives
      {Opcode::ARCH, 1},
      {Opcode::PROC, 1},
      {Opcode::MODE, 1},
      {Opcode::ALIGN, 1},
      {Opcode::SECTION, 2},
      {Opcode::DATA, 2},
      {Opcode::IF, 1},
      {Opcode::ELIF, 1},
      {Opcode::ELSE, 0},
      {Opcode::ENDIF, 0},
      {Opcode::ABI, 1},
      {Opcode::EXTERN, 1},
      {Opcode::GLOBAL, 1},
      {Opcode::INCLUDE, 1},
      {Opcode::VERSION, 3},
      
      // No operation
      {Opcode::NOP, 0}
  };
  
  auto it = expectedCounts.find(opcode);
  if (it != expectedCounts.end()) {
      return it->second;
  }
  
  return std::nullopt;
}

bool InstructionSet::validateInstruction(const Instruction& instruction) {
  uint8_t opcode = instruction.getOpcode();
  const auto& operands = instruction.getOperands();
  
  // Check if opcode is valid
  if (!isValidOpcode(opcode)) {
      return false;
  }
  
  // Check operand count
  auto expectedCount = getExpectedOperandCount(opcode);
  if (expectedCount && *expectedCount != operands.size()) {
      // Some instructions have variable operand counts based on ABI, etc.
      if (opcode == Opcode::CALL || opcode == Opcode::RET || 
          opcode == Opcode::VAR || opcode == Opcode::SWITCH) {
          // Allow variable number of operands for these instructions
      } else {
          return false;
      }
  }
  
  // Additional validation could be performed here based on operand types
  
  return true;
}

std::string InstructionSet::getInstructionName(uint8_t opcode) {
  static const std::unordered_map<uint8_t, std::string> opcodeNames = {
      // Control Flow
      {Opcode::SYM, "SYM"},
      {Opcode::BR, "BR"},
      {Opcode::CALL, "CALL"},
      {Opcode::RET, "RET"},
      {Opcode::CMP, "CMP"},
      {Opcode::SWITCH, "SWITCH"},
      
      // Memory Operations
      {Opcode::MOV, "MOV"},
      {Opcode::PUSH, "PUSH"},
      {Opcode::POP, "POP"},
      {Opcode::LEA, "LEA"},
      {Opcode::SCOPEE, "SCOPEE"},
      {Opcode::SCOPEL, "SCOPEL"},
      {Opcode::VAR, "VAR"},
      {Opcode::MEMCPY, "MEMCPY"},
      {Opcode::MEMSET, "MEMSET"},
      {Opcode::MEMCMP, "MEMCMP"},
      {Opcode::XCHG, "XCHG"},
      {Opcode::CAS, "CAS"},
      
      // Bit Manipulation
      {Opcode::AND, "AND"},
      {Opcode::OR, "OR"},
      {Opcode::XOR, "XOR"},
      {Opcode::NOT, "NOT"},
      {Opcode::SHL, "SHL"},
      {Opcode::SHR, "SHR"},
      {Opcode::SAR, "SAR"},
      
      // Arithmetic
      {Opcode::ADD, "ADD"},
      {Opcode::SUB, "SUB"},
      {Opcode::MUL, "MUL"},
      {Opcode::DIV, "DIV"},
      {Opcode::MOD, "MOD"},
      {Opcode::INC, "INC"},
      {Opcode::DEC, "DEC"},
      {Opcode::NEG, "NEG"},
      {Opcode::ABS, "ABS"},
      {Opcode::SQRT, "SQRT"},
      {Opcode::CEIL, "CEIL"},
      {Opcode::FLOOR, "FLOOR"},
      {Opcode::ROUND, "ROUND"},
      
      // Vector/Array
      {Opcode::VLOAD, "VLOAD"},
      {Opcode::VSTORE, "VSTORE"},
      {Opcode::VEXTRACT, "VEXTRACT"},
      {Opcode::VINSERT, "VINSERT"},
      {Opcode::VDOT, "VDOT"},
      
      // Type Operations
      {Opcode::TYPEOF, "TYPEOF"},
      {Opcode::SIZEOF, "SIZEOF"},
      {Opcode::CONVERT, "CONVERT"},
      {Opcode::CAST, "CAST"},
      {Opcode::GET, "GET"},
      {Opcode::SET, "SET"},
      {Opcode::INDEX, "INDEX"},
      {Opcode::UPDT, "UPDT"},
      
      // Directives
      {Opcode::ARCH, "ARCH"},
      {Opcode::PROC, "PROC"},
      {Opcode::MODE, "MODE"},
      {Opcode::ALIGN, "ALIGN"},
      {Opcode::SECTION, "SECTION"},
      {Opcode::DATA, "DATA"},
      {Opcode::IF, "IF"},
      {Opcode::ELIF, "ELIF"},
      {Opcode::ELSE, "ELSE"},
      {Opcode::ENDIF, "ENDIF"},
      {Opcode::ABI, "ABI"},
      {Opcode::EXTERN, "EXTERN"},
      {Opcode::GLOBAL, "GLOBAL"},
      {Opcode::INCLUDE, "INCLUDE"},
      {Opcode::VERSION, "VERSION"},
      
      // No operation
      {Opcode::NOP, "NOP"}
  };
  
  auto it = opcodeNames.find(opcode);
  if (it != opcodeNames.end()) {
      return it->second;
  }
  
  return "UNKNOWN";
}

std::optional<uint8_t> InstructionSet::getOpcodeFromName(const std::string& name) {
  static const std::unordered_map<std::string, uint8_t> nameOpcodes = {
      // Control Flow
      {"SYM", Opcode::SYM},
      {"BR", Opcode::BR},
      {"CALL", Opcode::CALL},
      {"RET", Opcode::RET},
      {"CMP", Opcode::CMP},
      {"SWITCH", Opcode::SWITCH},
      
      // Memory Operations
      {"MOV", Opcode::MOV},
      {"PUSH", Opcode::PUSH},
      {"POP", Opcode::POP},
      {"LEA", Opcode::LEA},
      {"SCOPEE", Opcode::SCOPEE},
      {"SCOPEL", Opcode::SCOPEL},
      {"VAR", Opcode::VAR},
      {"MEMCPY", Opcode::MEMCPY},
      {"MEMSET", Opcode::MEMSET},
      {"MEMCMP", Opcode::MEMCMP},
      {"XCHG", Opcode::XCHG},
      {"CAS", Opcode::CAS},
      
      // Bit Manipulation
      {"AND", Opcode::AND},
      {"OR", Opcode::OR},
      {"XOR", Opcode::XOR},
      {"NOT", Opcode::NOT},
      {"SHL", Opcode::SHL},
      {"SHR", Opcode::SHR},
      {"SAR", Opcode::SAR},
      
      // Arithmetic
      {"ADD", Opcode::ADD},
      {"SUB", Opcode::SUB},
      {"MUL", Opcode::MUL},
      {"DIV", Opcode::DIV},
      {"MOD", Opcode::MOD},
      {"INC", Opcode::INC},
      {"DEC", Opcode::DEC},
      {"NEG", Opcode::NEG},
      {"ABS", Opcode::ABS},
      {"SQRT", Opcode::SQRT},
      {"CEIL", Opcode::CEIL},
      {"FLOOR", Opcode::FLOOR},
      {"ROUND", Opcode::ROUND},
      
      // Vector/Array
      {"VLOAD", Opcode::VLOAD},
      {"VSTORE", Opcode::VSTORE},
      {"VEXTRACT", Opcode::VEXTRACT},
      {"VINSERT", Opcode::VINSERT},
      {"VDOT", Opcode::VDOT},
      
      // Type Operations
      {"TYPEOF", Opcode::TYPEOF},
      {"SIZEOF", Opcode::SIZEOF},
      {"CONVERT", Opcode::CONVERT},
      {"CAST", Opcode::CAST},
      {"GET", Opcode::GET},
      {"SET", Opcode::SET},
      {"INDEX", Opcode::INDEX},
      {"UPDT", Opcode::UPDT},
      
      // Directives
      {"ARCH", Opcode::ARCH},
      {"PROC", Opcode::PROC},
      {"MODE", Opcode::MODE},
      {"ALIGN", Opcode::ALIGN},
      {"SECTION", Opcode::SECTION},
      {"DATA", Opcode::DATA},
      {"IF", Opcode::IF},
      {"ELIF", Opcode::ELIF},
      {"ELSE", Opcode::ELSE},
      {"ENDIF", Opcode::ENDIF},
      {"ABI", Opcode::ABI},
      {"EXTERN", Opcode::EXTERN},
      {"GLOBAL", Opcode::GLOBAL},
      {"INCLUDE", Opcode::INCLUDE},
      {"VERSION", Opcode::VERSION},
      
      // No operation
      {"NOP", Opcode::NOP}
  };
  
  // Convert to uppercase for case-insensitive comparison
  std::string upperName = name;
  std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
  
  auto it = nameOpcodes.find(upperName);
  if (it != nameOpcodes.end()) {
      return it->second;
  }
  
  return std::nullopt;
}

bool InstructionSet::isValidOpcode(uint8_t opcode) {
  // Check if the opcode is defined
  return getInstructionName(opcode) != "UNKNOWN";
}

} // namespace coil