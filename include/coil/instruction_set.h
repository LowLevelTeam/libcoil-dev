#ifndef COIL_INSTRUCTION_SET_H
#define COIL_INSTRUCTION_SET_H

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace coil {

/**
* Instruction opcodes
*/
namespace Opcode {
  // Control Flow (0x01-0x0F)
  constexpr uint8_t SYM     = 0x01;
  constexpr uint8_t BR      = 0x02;
  constexpr uint8_t CALL    = 0x03;
  constexpr uint8_t RET     = 0x04;
  constexpr uint8_t CMP     = 0x05;
  constexpr uint8_t SWITCH  = 0x09;
  
  // Memory Operations (0x10-0x2F)
  constexpr uint8_t MOV     = 0x10;
  constexpr uint8_t PUSH    = 0x11;
  constexpr uint8_t POP     = 0x12;
  constexpr uint8_t LEA     = 0x13;
  constexpr uint8_t SCOPEE  = 0x14;
  constexpr uint8_t SCOPEL  = 0x15;
  constexpr uint8_t VAR     = 0x16;
  constexpr uint8_t MEMCPY  = 0x17;
  constexpr uint8_t MEMSET  = 0x18;
  constexpr uint8_t MEMCMP  = 0x19;
  constexpr uint8_t XCHG    = 0x1A;
  constexpr uint8_t CAS     = 0x1B;
  
  // Bit Manipulation (0x50-0x5F)
  constexpr uint8_t AND     = 0x50;
  constexpr uint8_t OR      = 0x51;
  constexpr uint8_t XOR     = 0x52;
  constexpr uint8_t NOT     = 0x53;
  constexpr uint8_t SHL     = 0x54;
  constexpr uint8_t SHR     = 0x55;
  constexpr uint8_t SAR     = 0x56;
  
  // Arithmetic (0x60-0x8F)
  constexpr uint8_t ADD     = 0x60;
  constexpr uint8_t SUB     = 0x61;
  constexpr uint8_t MUL     = 0x62;
  constexpr uint8_t DIV     = 0x63;
  constexpr uint8_t MOD     = 0x64;
  constexpr uint8_t INC     = 0x65;
  constexpr uint8_t DEC     = 0x66;
  constexpr uint8_t NEG     = 0x67;
  constexpr uint8_t ABS     = 0x68;
  constexpr uint8_t SQRT    = 0x69;
  constexpr uint8_t CEIL    = 0x6B;
  constexpr uint8_t FLOOR   = 0x6C;
  constexpr uint8_t ROUND   = 0x6D;
  
  // Vector/Array (0x90-0x9F)
  constexpr uint8_t VLOAD   = 0x90;
  constexpr uint8_t VSTORE  = 0x91;
  constexpr uint8_t VEXTRACT = 0x96;
  constexpr uint8_t VINSERT = 0x97;
  constexpr uint8_t VDOT    = 0x9C;
  
  // Type Operations (0xA0-0xAF)
  constexpr uint8_t TYPEOF  = 0xA0;
  constexpr uint8_t SIZEOF  = 0xA1;
  constexpr uint8_t CONVERT = 0xA3;
  constexpr uint8_t CAST    = 0xA4;
  constexpr uint8_t GET     = 0xA6;
  constexpr uint8_t SET     = 0xA7;
  constexpr uint8_t INDEX   = 0xA8;
  constexpr uint8_t UPDT    = 0xA9;
  
  // Directives (0xB0-0xBF)
  constexpr uint8_t ARCH    = 0xB0;
  constexpr uint8_t PROC    = 0xB1;
  constexpr uint8_t MODE    = 0xB2;
  constexpr uint8_t ALIGN   = 0xB3;
  constexpr uint8_t SECTION = 0xB4;
  constexpr uint8_t DATA    = 0xB5;
  constexpr uint8_t IF      = 0xB6;
  constexpr uint8_t ELIF    = 0xB7;
  constexpr uint8_t ELSE    = 0xB8;
  constexpr uint8_t ENDIF   = 0xB9;
  constexpr uint8_t ABI     = 0xBA;
  constexpr uint8_t EXTERN  = 0xBB;
  constexpr uint8_t GLOBAL  = 0xBC;
  constexpr uint8_t INCLUDE = 0xBD;
  constexpr uint8_t VERSION = 0xBE;
  
  // No operation
  constexpr uint8_t NOP     = 0x00;
}

/**
* Operand class representing a single instruction operand
*/
class Operand {
public:
  // Default constructor
  Operand() = default;
  
  // Construct from type and value
  Operand(uint16_t type, const std::vector<uint8_t>& value);
  
  // Create variable operand
  static Operand createVariable(uint16_t varId);
  
  // Create immediate operand
  static Operand createImmediate(int32_t value, uint16_t type);
  static Operand createImmediate(int64_t value, uint16_t type);
  static Operand createImmediate(float value);
  static Operand createImmediate(double value);
  
  // Create symbol operand
  static Operand createSymbol(uint16_t symbolId);
  
  // Create register operand
  static Operand createRegister(uint16_t registerId, uint16_t registerType);
  
  // Create memory operand
  static Operand createMemory(uint16_t baseReg, uint16_t indexReg = 0, uint8_t scale = 0, int32_t displacement = 0);
  
  // Encode operand to binary
  std::vector<uint8_t> encode() const;
  
  // Decode operand from binary
  static Operand decode(const std::vector<uint8_t>& data, size_t& offset);
  
  // Get properties
  uint16_t getType() const { return type_; }
  const std::vector<uint8_t>& getValue() const { return value_; }
  
private:
  uint16_t type_ = 0;
  std::vector<uint8_t> value_;
};

/**
* Instruction class representing a single COIL instruction
*/
class Instruction {
public:
  // Default constructor
  Instruction() = default;
  
  // Construct from opcode and operands
  Instruction(uint8_t opcode, const std::vector<Operand>& operands);
  
  // Encode instruction to binary
  std::vector<uint8_t> encode() const;
  
  // Decode instruction from binary
  static Instruction decode(const std::vector<uint8_t>& data, size_t& offset);
  
  // Validate instruction
  bool validate() const;
  
  // Get instruction size in bytes
  size_t getSize() const;
  
  // Get opcode and operands
  uint8_t getOpcode() const { return opcode_; }
  const std::vector<Operand>& getOperands() const { return operands_; }
  
  // Get instruction name for debug and error messages
  std::string getInstructionName() const;
  
private:
  uint8_t opcode_ = 0;
  std::vector<Operand> operands_;
};

/**
* Instruction parser and validator
*/
class InstructionSet {
public:
  // Get expected operand count for opcode
  static std::optional<size_t> getExpectedOperandCount(uint8_t opcode);
  
  // Check if instruction is valid
  static bool validateInstruction(const Instruction& instruction);
  
  // Get instruction name from opcode
  static std::string getInstructionName(uint8_t opcode);
  
  // Get opcode from instruction name
  static std::optional<uint8_t> getOpcodeFromName(const std::string& name);
  
  // Check if opcode exists
  static bool isValidOpcode(uint8_t opcode);
};

} // namespace coil

#endif // COIL_INSTRUCTION_SET_H