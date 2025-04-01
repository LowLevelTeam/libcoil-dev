#ifndef COIL_ERROR_CODES_H
#define COIL_ERROR_CODES_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace coil {

/**
* Error category codes
*/
namespace ErrorCategory {
  constexpr uint8_t COMPILATION = 0x01;   // Errors during CASM assembly
  constexpr uint8_t LINKING     = 0x02;   // Errors during linking process
  constexpr uint8_t VALIDATION  = 0x03;   // Errors during validation
  constexpr uint8_t RUNTIME     = 0x04;   // Errors during program execution
}

/**
* Compilation subcategory codes
*/
namespace CompilationSubcategory {
  constexpr uint8_t SYNTAX     = 0x00;    // Syntax errors
  constexpr uint8_t SCOPE      = 0x01;    // Scope-related errors
  constexpr uint8_t INSTRUCTION = 0x02;   // Instruction-related errors
  constexpr uint8_t VARIABLE   = 0x03;    // Variable-related errors
  constexpr uint8_t TYPE       = 0x04;    // Type-related errors
  constexpr uint8_t SYMBOL     = 0x05;    // Symbol-related errors
  constexpr uint8_t SECTION    = 0x06;    // Section-related errors
}

/**
* Linking subcategory codes
*/
namespace LinkingSubcategory {
  constexpr uint8_t SYMBOL_RESOLUTION = 0x00;  // Symbol resolution errors
  constexpr uint8_t SECTION_ALIGNMENT = 0x01;  // Section alignment errors
  constexpr uint8_t RELOCATION        = 0x02;  // Relocation errors
  constexpr uint8_t FORMAT            = 0x03;  // Format errors
  constexpr uint8_t COMPATIBILITY     = 0x04;  // Compatibility errors
}

/**
* Validation subcategory codes
*/
namespace ValidationSubcategory {
  constexpr uint8_t TYPE_CHECK      = 0x00;   // Type checking errors
  constexpr uint8_t MEMORY_SAFETY   = 0x01;   // Memory safety errors
  constexpr uint8_t INSTRUCTION_VALIDITY = 0x02; // Instruction validity errors
  constexpr uint8_t ABI_COMPLIANCE  = 0x03;   // ABI compliance errors
  constexpr uint8_t RESOURCE_USAGE  = 0x04;   // Resource usage errors
}

/**
* Runtime subcategory codes
*/
namespace RuntimeSubcategory {
  constexpr uint8_t ARITHMETIC      = 0x00;   // Arithmetic errors
  constexpr uint8_t MEMORY          = 0x01;   // Memory errors
  constexpr uint8_t FUNCTION_CALL   = 0x02;   // Function call errors
  constexpr uint8_t CONTROL_FLOW    = 0x03;   // Control flow errors
  constexpr uint8_t TYPE            = 0x04;   // Type errors
  constexpr uint8_t RESOURCE        = 0x05;   // Resource errors
  constexpr uint8_t EXTERNAL        = 0x06;   // External errors
  constexpr uint8_t DEVICE          = 0x0F;   // Device-specific errors
}

/**
* Error severity levels
*/
enum class ErrorSeverity {
  ERROR,      // Prevents successful completion
  WARNING,    // Potential issue, but not fatal
  NOTE        // Informational message
};

/**
* Error information structure
*/
struct ErrorInfo {
  uint32_t error_code;       // Full 32-bit error code
  uint32_t location;         // File offset or memory address
  uint32_t file_id;          // Source file identifier (if applicable)
  uint32_t line;             // Source line number (if applicable)
  uint32_t column;           // Source column number (if applicable)
  uint16_t symbol_index;     // Related symbol index (if applicable)
  uint16_t section_index;    // Related section index (if applicable)
  std::string message;       // Error message
  ErrorSeverity severity;    // Error severity
  
  // Create a standard error message string
  std::string toString() const;
};

/**
* Error manager for tracking and reporting errors
*/
class ErrorManager {
public:
  // Create an error
  void addError(uint32_t errorCode, const std::string& message, ErrorSeverity severity = ErrorSeverity::ERROR,
                uint32_t location = 0, uint32_t fileId = 0, uint32_t line = 0, uint32_t column = 0,
                uint16_t symbolIndex = 0, uint16_t sectionIndex = 0);
  
  // Create a pre-defined error with a specific code
  void addStandardError(uint32_t errorCode, ErrorSeverity severity = ErrorSeverity::ERROR,
                        uint32_t location = 0, uint32_t fileId = 0, uint32_t line = 0, uint32_t column = 0,
                        uint16_t symbolIndex = 0, uint16_t sectionIndex = 0);
  
  // Check if there are any errors
  bool hasErrors() const;
  
  // Check if there are any errors of a specific severity
  bool hasErrors(ErrorSeverity severity) const;
  
  // Get all errors
  const std::vector<ErrorInfo>& getErrors() const;
  
  // Get errors of a specific severity
  std::vector<ErrorInfo> getErrors(ErrorSeverity severity) const;
  
  // Clear all errors
  void clear();
  
  // Get standard error message for a given error code
  static std::string getStandardErrorMessage(uint32_t errorCode);
  
  // Create a full error code from category, subcategory, and specific error
  static constexpr uint32_t makeErrorCode(uint8_t category, uint8_t subcategory, uint16_t specificError) {
      return (static_cast<uint32_t>(category) << 24) | 
              (static_cast<uint32_t>(subcategory) << 16) | 
              specificError;
  }
  
  // Extract parts from an error code
  static constexpr uint8_t getErrorCategory(uint32_t errorCode) {
      return static_cast<uint8_t>((errorCode >> 24) & 0xFF);
  }
  
  static constexpr uint8_t getErrorSubcategory(uint32_t errorCode) {
      return static_cast<uint8_t>((errorCode >> 16) & 0xFF);
  }
  
  static constexpr uint16_t getSpecificError(uint32_t errorCode) {
      return static_cast<uint16_t>(errorCode & 0xFFFF);
  }
  
private:
  std::vector<ErrorInfo> errors_;
};

/**
* Common error codes
*/
namespace ErrorCode {
  // Compilation - Syntax Errors (0x0100xx)
  constexpr uint32_t INVALID_TOKEN = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::SYNTAX, 0x0001);
  
  constexpr uint32_t UNEXPECTED_EOF = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::SYNTAX, 0x0002);
  
  constexpr uint32_t MISSING_OPERAND = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::SYNTAX, 0x0003);
  
  constexpr uint32_t EXTRA_OPERAND = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::SYNTAX, 0x0004);
  
  constexpr uint32_t INVALID_LABEL = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::SYNTAX, 0x0005);
  
  // Compilation - Variable Errors (0x0103xx)
  constexpr uint32_t VARIABLE_ALREADY_DEFINED = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::VARIABLE, 0x0001);
  
  constexpr uint32_t VARIABLE_NOT_DEFINED = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::VARIABLE, 0x0002);
  
  constexpr uint32_t INVALID_VARIABLE_ID = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::VARIABLE, 0x0003);
  
  // Compilation - Type Errors (0x0104xx)
  constexpr uint32_t INVALID_TYPE = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::TYPE, 0x0001);
  
  constexpr uint32_t TYPE_MISMATCH = ErrorManager::makeErrorCode(
      ErrorCategory::COMPILATION, CompilationSubcategory::TYPE, 0x0002);
  
  // Runtime - Arithmetic Errors (0x0400xx)
  constexpr uint32_t DIVISION_BY_ZERO = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::ARITHMETIC, 0x0001);
  
  constexpr uint32_t INTEGER_OVERFLOW = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::ARITHMETIC, 0x0002);
  
  constexpr uint32_t INTEGER_UNDERFLOW = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::ARITHMETIC, 0x0003);
  
  constexpr uint32_t FLOAT_OVERFLOW = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::ARITHMETIC, 0x0004);
  
  constexpr uint32_t FLOAT_UNDERFLOW = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::ARITHMETIC, 0x0005);
  
  // Runtime - Memory Errors (0x0401xx)
  constexpr uint32_t NULL_POINTER_DEREFERENCE = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::MEMORY, 0x0001);
  
  constexpr uint32_t OUT_OF_BOUNDS_ACCESS = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::MEMORY, 0x0002);
  
  constexpr uint32_t MISALIGNED_ACCESS = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::MEMORY, 0x0003);
  
  constexpr uint32_t MEMORY_LEAK = ErrorManager::makeErrorCode(
      ErrorCategory::RUNTIME, RuntimeSubcategory::MEMORY, 0x0004);
}

} // namespace coil

#endif // COIL_ERROR_CODES_H