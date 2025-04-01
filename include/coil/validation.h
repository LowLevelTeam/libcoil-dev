#ifndef COIL_VALIDATION_H
#define COIL_VALIDATION_H

#include <cstdint>
#include <vector>
#include <string>

namespace coil {

// Forward declarations
class CoilObject;
class Instruction;
class ErrorManager;

namespace utils {

/**
* Validation utilities for COIL objects and components
*/
class Validation {
public:
  // Validate a COIL object
  static bool validateCoilObject(const CoilObject& object, ErrorManager& errorManager);
  
  // Validate a COIL header
  static bool validateCoilHeader(const std::vector<uint8_t>& data, ErrorManager& errorManager);
  
  // Validate an instruction
  static bool validateInstruction(const Instruction& instruction, ErrorManager& errorManager);
  
  // Validate a symbol table
  static bool validateSymbolTable(const CoilObject& object, ErrorManager& errorManager);
  
  // Validate a section table
  static bool validateSectionTable(const CoilObject& object, ErrorManager& errorManager);
  
  // Validate section data
  static bool validateSectionData(const CoilObject& object, uint16_t sectionIndex, ErrorManager& errorManager);
  
  // Validate relocations
  static bool validateRelocations(const CoilObject& object, ErrorManager& errorManager);
  
  // Validate type compatibility
  static bool validateTypeCompatibility(uint16_t sourceType, uint16_t destType, ErrorManager& errorManager);
  
  // Validate memory alignment
  static bool validateMemoryAlignment(uint32_t address, uint16_t type, ErrorManager& errorManager);
  
  // Validate variable usage
  static bool validateVariableUsage(uint16_t varId, const CoilObject& object, ErrorManager& errorManager);
  
  // Check if an identifier is valid
  static bool isValidIdentifier(const std::string& identifier);
  
  // Check if a section name is valid
  static bool isValidSectionName(const std::string& name);
  
  // Check if a memory access is valid
  static bool isValidMemoryAccess(uint32_t address, uint32_t size, uint32_t boundaries, ErrorManager& errorManager);
};

} // namespace utils
} // namespace coil

#endif // COIL_VALIDATION_H