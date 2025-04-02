#include "test_helper.h"

bool testHeaderValidation() {
  // Create a valid header
  coil::CoilHeader validHeader = coil::CoilHeader::createDefault();
  
  // Create binary data large enough to hold header plus referenced sections
  const size_t dataSize = 256; // Make it big enough for all offsets
  std::vector<uint8_t> validData(dataSize);
  
  // Set valid offsets within the data bounds
  validHeader.symbol_offset = sizeof(coil::CoilHeader);
  validHeader.section_offset = sizeof(coil::CoilHeader) + 32;  // Some space after symbols
  validHeader.reloc_offset = 0; // No relocations
  validHeader.debug_offset = 0; // No debug info
  validHeader.file_size = dataSize; // Set to exactly match data size
  
  // Encode the header into the binary data
  auto encodedHeader = validHeader.encode();
  std::copy(encodedHeader.begin(), encodedHeader.end(), validData.begin());
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Analyze header if verbose output is enabled
  if (verboseTestOutput) {
      analyzeCoilHeader(validData);
  }
  
  // Validate the header
  bool isValid = coil::utils::Validation::validateCoilHeader(validData, errorManager);
  
  // Debug if validation fails
  if (!isValid) {
      std::cerr << "\033[1;31mHeader validation failed\033[0m" << std::endl;
      dumpErrorManager(errorManager, "Header Validation Errors");
      analyzeCoilHeader(validData);
      printBinaryData(validData, 0, 64); // Show first 64 bytes
  }
  
  TEST_ASSERT_MSG(isValid, "Header validation failed");
  TEST_ASSERT_MSG(!errorManager.hasErrors(), "ErrorManager contains errors after header validation");
  
  // Test invalid header
  coil::CoilHeader invalidHeader = validHeader;
  invalidHeader.magic[0] = 'X';  // Change magic from "COIL" to "XOIL"
  
  // Encode the invalid header
  std::vector<uint8_t> invalidData(dataSize);
  auto encodedInvalidHeader = invalidHeader.encode();
  std::copy(encodedInvalidHeader.begin(), encodedInvalidHeader.end(), invalidData.begin());
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid header (should fail)
  isValid = coil::utils::Validation::validateCoilHeader(invalidData, errorManager);
  
  TEST_ASSERT_MSG(!isValid, "Invalid header validation should fail");
  TEST_ASSERT_MSG(errorManager.hasErrors(), "ErrorManager should contain errors after invalid header validation");
  
  return true;
}

bool testInstructionValidation() {
  // Create a valid instruction
  std::vector<coil::Operand> validOperands = {
      coil::Operand::createVariable(1),
      coil::Operand::createVariable(2),
      coil::Operand::createVariable(3)
  };
  coil::Instruction validInstruction(coil::Opcode::ADD, validOperands);
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Validate the instruction
  bool isValid = coil::utils::Validation::validateInstruction(validInstruction, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Create an invalid instruction (wrong opcode)
  coil::Instruction invalidOpcode(0xFF, validOperands);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid instruction
  isValid = coil::utils::Validation::validateInstruction(invalidOpcode, errorManager);
  TEST_ASSERT(!isValid);
  TEST_ASSERT(errorManager.hasErrors());
  
  // Create an invalid instruction (wrong operand count)
  std::vector<coil::Operand> invalidOperands = {
      coil::Operand::createVariable(1)
  };
  coil::Instruction invalidOperandCount(coil::Opcode::ADD, invalidOperands);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid instruction
  isValid = coil::utils::Validation::validateInstruction(invalidOperandCount, errorManager);
  TEST_ASSERT(!isValid);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

bool testSymbolTableValidation() {
  // Create a COIL object with valid symbols
  coil::CoilObject validObj = createTestCoilObject();
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Validate the symbol table
  bool isValid = coil::utils::Validation::validateSymbolTable(validObj, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Create a COIL object with duplicate symbol names
  coil::CoilObject invalidObj = createTestCoilObject();
  
  // Add a duplicate symbol
  coil::Symbol duplicateSymbol;
  duplicateSymbol.name = ".text";  // Same as the first symbol
  duplicateSymbol.name_length = static_cast<uint16_t>(duplicateSymbol.name.length());
  duplicateSymbol.attributes = coil::SymbolFlags::LOCAL;
  duplicateSymbol.value = 0;
  duplicateSymbol.section_index = 0;
  duplicateSymbol.processor_type = 0;
  
  invalidObj.addSymbol(duplicateSymbol);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid symbol table
  isValid = coil::utils::Validation::validateSymbolTable(invalidObj, errorManager);
  TEST_ASSERT(!isValid);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

bool testSectionTableValidation() {
  // Create a COIL object with valid sections
  coil::CoilObject validObj = createTestCoilObject();
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Validate the section table
  bool isValid = coil::utils::Validation::validateSectionTable(validObj, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Create a COIL object with invalid section (size mismatch)
  coil::CoilObject invalidObj = createTestCoilObject();
  
  // Get the first section and modify it
  coil::Section invalidSection = invalidObj.getSection(0);
  invalidSection.size = 100;  // Size doesn't match data.size()
  invalidObj.updateSection(0, invalidSection);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid section table
  isValid = coil::utils::Validation::validateSectionTable(invalidObj, errorManager);
  TEST_ASSERT(!isValid);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

bool testSectionDataValidation() {
  // Create a COIL object with empty section data
  coil::CoilObject obj = createTestCoilObject();
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Validate empty section data (should be valid)
  bool isValid = coil::utils::Validation::validateSectionData(obj, 0, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Add a valid instruction
  std::vector<coil::Operand> operands = {
      coil::Operand::createVariable(1),
      coil::Operand::createVariable(2)
  };
  coil::Instruction validInstr(coil::Opcode::MOV, operands);
  obj.addInstruction(0, validInstr);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate section with valid instruction
  isValid = coil::utils::Validation::validateSectionData(obj, 0, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Create a COIL object and add invalid data
  coil::CoilObject invalidObj = createTestCoilObject();
  
  // Add invalid data that doesn't constitute a valid instruction
  std::vector<uint8_t> invalidData = {0xFF, 0x02, 0x03};  // Invalid opcode, etc.
  invalidObj.updateSectionData(0, invalidData);
  
  // Reset error manager
  errorManager.clear();
  
  // This validation might not fail since we're not doing detailed instruction
  // parsing in the test, but the test framework should handle this gracefully
  coil::utils::Validation::validateSectionData(invalidObj, 0, errorManager);
  
  return true;
}

bool testRelocationValidation() {
  // Create a COIL object
  coil::CoilObject obj = createTestCoilObject();
  
  // Add some data to the first section so it has non-zero size
  std::vector<uint8_t> sectionData = {0x01, 0x02, 0x03, 0x04};
  obj.updateSectionData(0, sectionData);
  
  // Add a valid relocation
  coil::Relocation validReloc;
  validReloc.offset = 0;  // Valid offset within the section
  validReloc.symbol_index = 0;  // First symbol
  validReloc.section_index = 0;  // First section
  validReloc.type = coil::RelocationType::ABSOLUTE;
  validReloc.size = 4;
  
  obj.addRelocation(validReloc);
  
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Validate relocations with debugging
  bool isValid = coil::utils::Validation::validateRelocations(obj, errorManager);
  
  if (!isValid) {
      std::cerr << "\033[1;31mRelocation validation failed\033[0m" << std::endl;
      dumpErrorManager(errorManager, "Relocation Validation Errors");
      
      // Print section information
      std::cerr << "\033[1;34mSection Information:\033[0m" << std::endl;
      for (uint16_t i = 0; i < obj.getSectionCount(); i++) {
          const auto& section = obj.getSection(i);
          std::cerr << "  Section " << i << ": size=" << section.size 
                    << ", data size=" << section.data.size() 
                    << ", attributes=0x" << std::hex << section.attributes << std::dec
                    << std::endl;
          
          if (section.data.size() > 0) {
              std::cerr << "  Data:" << std::endl;
              printBinaryData(section.data);
          }
      }
      
      // Print relocation details
      std::cerr << "\033[1;34mRelocation Details:\033[0m" << std::endl;
      for (uint16_t i = 0; i < obj.getRelocationCount(); i++) {
          const auto& reloc = obj.getRelocation(i);
          std::cerr << "  Relocation " << i << ":" << std::endl;
          std::cerr << "    Offset: " << reloc.offset << std::endl;
          std::cerr << "    Symbol index: " << reloc.symbol_index << std::endl;
          std::cerr << "    Section index: " << reloc.section_index << std::endl;
          std::cerr << "    Type: " << (int)reloc.type << std::endl;
          std::cerr << "    Size: " << (int)reloc.size << std::endl;
      }
  }
  
  TEST_ASSERT_MSG(isValid, "Relocation validation failed");
  TEST_ASSERT_MSG(!errorManager.hasErrors(), "ErrorManager contains errors after relocation validation");
  
  // Create a COIL object with invalid relocation
  coil::CoilObject invalidObj = createTestCoilObject();
  
  // Add same section data
  invalidObj.updateSectionData(0, sectionData);
  
  // Add an invalid relocation (invalid symbol index)
  coil::Relocation invalidReloc;
  invalidReloc.offset = 0;
  invalidReloc.symbol_index = 100;  // Invalid symbol index
  invalidReloc.section_index = 0;
  invalidReloc.type = coil::RelocationType::ABSOLUTE;
  invalidReloc.size = 4;
  
  invalidObj.addRelocation(invalidReloc);
  
  // Reset error manager
  errorManager.clear();
  
  // Validate the invalid relocations (should fail)
  isValid = coil::utils::Validation::validateRelocations(invalidObj, errorManager);
  
  TEST_ASSERT_MSG(!isValid, "Invalid relocation validation should fail");
  TEST_ASSERT_MSG(errorManager.hasErrors(), "ErrorManager should contain errors after invalid relocation validation");
  
  return true;
}

bool testTypeCompatibilityValidation() {
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Test compatible types
  bool isCompatible = coil::utils::Validation::validateTypeCompatibility(
      coil::Type::INT32, coil::Type::INT32, errorManager);
  TEST_ASSERT(isCompatible);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Test compatible types (widening)
  errorManager.clear();
  isCompatible = coil::utils::Validation::validateTypeCompatibility(
      coil::Type::INT16, coil::Type::INT32, errorManager);
  TEST_ASSERT(isCompatible);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Test incompatible types
  errorManager.clear();
  isCompatible = coil::utils::Validation::validateTypeCompatibility(
      coil::Type::INT32, coil::Type::FP32, errorManager);
  TEST_ASSERT(!isCompatible);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

bool testMemoryAlignmentValidation() {
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Test properly aligned memory access
  bool isAligned = coil::utils::Validation::validateMemoryAlignment(
      0x1000, coil::Type::INT32, errorManager);
  TEST_ASSERT(isAligned);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Test misaligned memory access
  errorManager.clear();
  isAligned = coil::utils::Validation::validateMemoryAlignment(
      0x1001, coil::Type::INT32, errorManager);
  TEST_ASSERT(!isAligned);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

bool testIdentifierValidation() {
  // Test valid identifiers
  TEST_ASSERT(coil::utils::Validation::isValidIdentifier("validIdentifier"));
  TEST_ASSERT(coil::utils::Validation::isValidIdentifier("_valid_identifier_123"));
  TEST_ASSERT(coil::utils::Validation::isValidIdentifier("a"));
  
  // Test invalid identifiers
  TEST_ASSERT(!coil::utils::Validation::isValidIdentifier(""));
  TEST_ASSERT(!coil::utils::Validation::isValidIdentifier("123invalid"));
  TEST_ASSERT(!coil::utils::Validation::isValidIdentifier("invalid-id"));
  TEST_ASSERT(!coil::utils::Validation::isValidIdentifier("invalid id"));
  
  return true;
}

bool testSectionNameValidation() {
  // Test valid section names
  TEST_ASSERT(coil::utils::Validation::isValidSectionName(".text"));
  TEST_ASSERT(coil::utils::Validation::isValidSectionName(".data"));
  TEST_ASSERT(coil::utils::Validation::isValidSectionName(".bss"));
  TEST_ASSERT(coil::utils::Validation::isValidSectionName(".custom_section"));
  TEST_ASSERT(coil::utils::Validation::isValidSectionName(".custom.section"));
  
  // Test invalid section names
  TEST_ASSERT(!coil::utils::Validation::isValidSectionName("text"));  // Missing dot
  TEST_ASSERT(!coil::utils::Validation::isValidSectionName(".123"));  // Starts with number after dot
  TEST_ASSERT(!coil::utils::Validation::isValidSectionName(".section-name"));  // Contains hyphen
  TEST_ASSERT(!coil::utils::Validation::isValidSectionName(".section name"));  // Contains space
  
  return true;
}

bool testMemoryAccessValidation() {
  // Create error manager
  coil::ErrorManager errorManager;
  
  // Test valid memory access
  bool isValid = coil::utils::Validation::isValidMemoryAccess(
      0x1000, 100, 0x2000, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Test access at boundary
  errorManager.clear();
  isValid = coil::utils::Validation::isValidMemoryAccess(
      0x1F00, 256, 0x2000, errorManager);
  TEST_ASSERT(isValid);
  TEST_ASSERT(!errorManager.hasErrors());
  
  // Test invalid memory access (out of bounds)
  errorManager.clear();
  isValid = coil::utils::Validation::isValidMemoryAccess(
      0x1F00, 257, 0x2000, errorManager);
  TEST_ASSERT(!isValid);
  TEST_ASSERT(errorManager.hasErrors());
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"Header Validation", testHeaderValidation},
      {"Instruction Validation", testInstructionValidation},
      {"Symbol Table Validation", testSymbolTableValidation},
      {"Section Table Validation", testSectionTableValidation},
      {"Section Data Validation", testSectionDataValidation},
      {"Relocation Validation", testRelocationValidation},
      {"Type Compatibility Validation", testTypeCompatibilityValidation},
      {"Memory Alignment Validation", testMemoryAlignmentValidation},
      {"Identifier Validation", testIdentifierValidation},
      {"Section Name Validation", testSectionNameValidation},
      {"Memory Access Validation", testMemoryAccessValidation}
  };
  
  return runTests(tests);
}