#include "coil/utils/validation.h"
#include "coil/binary_format.h"
#include "coil/type_system.h"
#include "coil/instruction_set.h"
#include "coil/error_codes.h"
#include <regex>
#include <unordered_set>

namespace coil {
namespace utils {

bool Validation::validateCoilObject(const CoilObject& object, ErrorManager& errorManager) {
  bool isValid = true;
  
  // Validate symbol table
  isValid &= validateSymbolTable(object, errorManager);
  
  // Validate section table
  isValid &= validateSectionTable(object, errorManager);
  
  // Validate relocations
  isValid &= validateRelocations(object, errorManager);
  
  // Validate each section's data
  for (uint16_t i = 0; i < object.getSectionCount(); ++i) {
      isValid &= validateSectionData(object, i, errorManager);
  }
  
  return isValid;
}

bool Validation::validateCoilHeader(const std::vector<uint8_t>& data, ErrorManager& errorManager) {
  // Check minimum size
  if (data.size() < sizeof(CoilHeader)) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0001),
          "File is too small to contain a valid COIL header",
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  // Decode and validate
  try {
      size_t offset = 0;
      CoilHeader header = CoilHeader::decode(data, offset);
      
      // Check magic
      if (header.magic[0] != 'C' || header.magic[1] != 'O' || 
          header.magic[2] != 'I' || header.magic[3] != 'L') {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0002),
              "Invalid magic number in COIL header",
              ErrorSeverity::ERROR
          );
          return false;
      }
      
      // Check version
      if (header.major > 1) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0003),
              "Unsupported COIL version",
              ErrorSeverity::ERROR
          );
          return false;
      }
      
      // Check file size
      if (header.file_size > data.size()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0004),
              "File size in header doesn't match actual file size",
              ErrorSeverity::ERROR
          );
          return false;
      }
      
      // Check offsets
      if (header.symbol_offset >= header.file_size ||
          header.section_offset >= header.file_size ||
          (header.reloc_offset > 0 && header.reloc_offset >= header.file_size) ||
          (header.debug_offset > 0 && header.debug_offset >= header.file_size)) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0005),
              "Invalid offset in COIL header",
              ErrorSeverity::ERROR
          );
          return false;
      }
      
      return true;
  } catch (const std::exception& e) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::FORMAT, 0x0006),
          std::string("Error decoding COIL header: ") + e.what(),
          ErrorSeverity::ERROR
      );
      return false;
  }
}

bool Validation::validateInstruction(const Instruction& instruction, ErrorManager& errorManager) {
  // Check if the opcode is valid
  if (!InstructionSet::isValidOpcode(instruction.getOpcode())) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::INSTRUCTION_VALIDITY, 0x0001),
          "Invalid opcode: " + std::to_string(instruction.getOpcode()),
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  // Check operand count
  auto expectedCount = InstructionSet::getExpectedOperandCount(instruction.getOpcode());
  if (expectedCount && *expectedCount != instruction.getOperands().size()) {
      // Some instructions have variable operand counts, so we only validate certain ones
      uint8_t opcode = instruction.getOpcode();
      if (opcode != Opcode::CALL && opcode != Opcode::RET && 
          opcode != Opcode::VAR && opcode != Opcode::SWITCH) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::INSTRUCTION_VALIDITY, 0x0002),
              "Invalid operand count for instruction " + InstructionSet::getInstructionName(opcode) +
              ": expected " + std::to_string(*expectedCount) + 
              ", got " + std::to_string(instruction.getOperands().size()),
              ErrorSeverity::ERROR
          );
          return false;
      }
  }
  
  // Additional validation could be performed based on the specific instruction
  
  return true;
}

bool Validation::validateSymbolTable(const CoilObject& object, ErrorManager& errorManager) {
  bool isValid = true;
  
  // Check for duplicate symbol names
  std::unordered_set<std::string> symbolNames;
  
  for (uint16_t i = 0; i < object.getSymbolCount(); ++i) {
      const Symbol& symbol = object.getSymbol(i);
      
      // Check for duplicate names
      if (!symbol.name.empty()) {
          if (symbolNames.find(symbol.name) != symbolNames.end()) {
              errorManager.addError(
                  ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SYMBOL_RESOLUTION, 0x0001),
                  "Duplicate symbol name: " + symbol.name,
                  ErrorSeverity::ERROR,
                  0, 0, 0, 0, i
              );
              isValid = false;
          } else {
              symbolNames.insert(symbol.name);
          }
      }
      
      // Check if the symbol's section index is valid
      if (symbol.section_index != 0xFFFF && 
          symbol.section_index >= object.getSectionCount()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SYMBOL_RESOLUTION, 0x0002),
              "Symbol references invalid section index: " + std::to_string(symbol.section_index),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, i
          );
          isValid = false;
      }
      
      // Check if the symbol name is valid
      if (!symbol.name.empty() && !isValidIdentifier(symbol.name)) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SYMBOL_RESOLUTION, 0x0003),
              "Invalid symbol name: " + symbol.name,
              ErrorSeverity::WARNING,
              0, 0, 0, 0, i
          );
          // Not a fatal error, so we don't set isValid = false
      }
  }
  
  return isValid;
}

bool Validation::validateSectionTable(const CoilObject& object, ErrorManager& errorManager) {
  bool isValid = true;
  
  for (uint16_t i = 0; i < object.getSectionCount(); ++i) {
      const Section& section = object.getSection(i);
      
      // Check if the section name index is valid
      if (section.name_index >= object.getSymbolCount()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SECTION_ALIGNMENT, 0x0001),
              "Section references invalid name index: " + std::to_string(section.name_index),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, 0, i
          );
          isValid = false;
      }
      
      // Check section alignment
      if (section.alignment > 0 && 
          (section.alignment & (section.alignment - 1)) != 0) {
          // Alignment is not a power of 2
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SECTION_ALIGNMENT, 0x0002),
              "Section alignment is not a power of 2: " + std::to_string(section.alignment),
              ErrorSeverity::WARNING,
              0, 0, 0, 0, 0, i
          );
          // Not a fatal error
      }
      
      // Check if the section size matches the data size
      if (section.size != section.data.size()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::SECTION_ALIGNMENT, 0x0003),
              "Section size doesn't match data size: " + std::to_string(section.size) + 
              " vs " + std::to_string(section.data.size()),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, 0, i
          );
          isValid = false;
      }
  }
  
  return isValid;
}

bool Validation::validateSectionData(const CoilObject& object, uint16_t sectionIndex, ErrorManager& errorManager) {
  const Section& section = object.getSection(sectionIndex);
  bool isValid = true;
  
  // Check section attributes
  if ((section.attributes & SectionFlags::EXECUTABLE) != 0) {
      // For executable sections, validate instructions
      size_t offset = 0;
      
      while (offset < section.data.size()) {
          try {
              Instruction instr = Instruction::decode(section.data, offset);
              
              // Validate the instruction
              if (!validateInstruction(instr, errorManager)) {
                  isValid = false;
              }
          } catch (const std::exception& e) {
              errorManager.addError(
                  ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::INSTRUCTION_VALIDITY, 0x0003),
                  std::string("Error decoding instruction at offset ") + 
                  std::to_string(offset) + ": " + e.what(),
                  ErrorSeverity::ERROR,
                  offset, 0, 0, 0, 0, sectionIndex
              );
              isValid = false;
              
              // Skip to the next byte to try to continue validation
              offset++;
          }
      }
  }
  
  return isValid;
}

bool Validation::validateRelocations(const CoilObject& object, ErrorManager& errorManager) {
  bool isValid = true;
  
  for (uint16_t i = 0; i < object.getRelocationCount(); ++i) {
      const Relocation& reloc = object.getRelocation(i);
      
      // Check if the symbol index is valid
      if (reloc.symbol_index >= object.getSymbolCount()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::RELOCATION, 0x0001),
              "Relocation references invalid symbol index: " + std::to_string(reloc.symbol_index),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, reloc.symbol_index, reloc.section_index
          );
          isValid = false;
      }
      
      // Check if the section index is valid
      if (reloc.section_index >= object.getSectionCount()) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::RELOCATION, 0x0002),
              "Relocation references invalid section index: " + std::to_string(reloc.section_index),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, reloc.symbol_index, reloc.section_index
          );
          isValid = false;
      }
      
      // Check if the relocation offset is within the section's bounds
      if (reloc.section_index < object.getSectionCount()) {
          const Section& section = object.getSection(reloc.section_index);
          
          if (reloc.offset >= section.size) {
              errorManager.addError(
                  ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::RELOCATION, 0x0003),
                  "Relocation offset is outside section bounds: " + std::to_string(reloc.offset) + 
                  " >= " + std::to_string(section.size),
                  ErrorSeverity::ERROR,
                  reloc.offset, 0, 0, 0, reloc.symbol_index, reloc.section_index
              );
              isValid = false;
          }
      }
      
      // Check if the relocation type is valid
      if (reloc.type != RelocationType::ABSOLUTE &&
          reloc.type != RelocationType::RELATIVE &&
          reloc.type != RelocationType::PC_RELATIVE &&
          reloc.type != RelocationType::SECTION_RELATIVE &&
          reloc.type != RelocationType::SYMBOL_ADDEND) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::RELOCATION, 0x0004),
              "Invalid relocation type: " + std::to_string(reloc.type),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, reloc.symbol_index, reloc.section_index
          );
          isValid = false;
      }
      
      // Check if the relocation size is valid (1, 2, 4, or 8 bytes)
      if (reloc.size != 1 && reloc.size != 2 && reloc.size != 4 && reloc.size != 8) {
          errorManager.addError(
              ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::RELOCATION, 0x0005),
              "Invalid relocation size: " + std::to_string(reloc.size),
              ErrorSeverity::ERROR,
              0, 0, 0, 0, reloc.symbol_index, reloc.section_index
          );
          isValid = false;
      }
  }
  
  return isValid;
}

bool Validation::validateTypeCompatibility(uint16_t sourceType, uint16_t destType, ErrorManager& errorManager) {
  if (!TypeInfo::areTypesCompatible(sourceType, destType)) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::TYPE_CHECK, 0x0001),
          "Incompatible types: " + TypeInfo::getTypeName(sourceType) + 
          " and " + TypeInfo::getTypeName(destType),
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  return true;
}

bool Validation::validateMemoryAlignment(uint32_t address, uint16_t type, ErrorManager& errorManager) {
  uint32_t alignment = 1;
  
  // Determine required alignment based on type
  if (TypeInfo::isIntegerType(type) || TypeInfo::isFloatType(type)) {
      alignment = TypeInfo::getTypeSize(type);
  } else if (TypeInfo::isVectorType(type)) {
      // Vector types typically require their size alignment
      alignment = TypeInfo::getTypeSize(type);
  } else if (TypeInfo::isPointerType(type)) {
      // Pointers typically require 4 or 8 byte alignment
      alignment = TypeInfo::getTypeSize(type);
  }
  
  // Check alignment
  if (address % alignment != 0) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::MEMORY_SAFETY, 0x0001),
          "Misaligned memory access: address 0x" + 
          std::to_string(address) + " for type " + 
          TypeInfo::getTypeName(type) + " (requires " + 
          std::to_string(alignment) + "-byte alignment)",
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  return true;
}

bool Validation::validateVariableUsage(uint16_t varId, const CoilObject& object, ErrorManager& errorManager) {
  // This would require full semantic analysis of all instructions
  // Simplified version just checks if the variable is declared somewhere
  
  // Real implementation would need to track variable declarations and uses
  // throughout the program, including scope information
  
  return true;
}

bool Validation::isValidIdentifier(const std::string& identifier) {
  // Check if the identifier matches the pattern [a-zA-Z_][a-zA-Z0-9_]*
  static const std::regex identifierPattern(R"([a-zA-Z_][a-zA-Z0-9_]*)");
  return std::regex_match(identifier, identifierPattern);
}

bool Validation::isValidSectionName(const std::string& name) {
  // Section names typically start with a period
  // Examples: .text, .data, .bss, .rodata
  static const std::regex sectionPattern(R"(\.[a-zA-Z_][a-zA-Z0-9_.]*)");
  return std::regex_match(name, sectionPattern);
}

bool Validation::isValidMemoryAccess(uint32_t address, uint32_t size, uint32_t boundaries, ErrorManager& errorManager) {
  // Check if the memory access is within bounds
  if (address + size > boundaries) {
      errorManager.addError(
          ErrorManager::makeErrorCode(ErrorCategory::VALIDATION, ValidationSubcategory::MEMORY_SAFETY, 0x0002),
          "Memory access out of bounds: address 0x" + 
          std::to_string(address) + " with size " + 
          std::to_string(size) + " exceeds boundary 0x" + 
          std::to_string(boundaries),
          ErrorSeverity::ERROR
      );
      return false;
  }
  
  return true;
}

} // namespace utils
} // namespace coil