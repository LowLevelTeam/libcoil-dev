#include "coil/error_codes.h"
#include <sstream>
#include <iomanip>
#include <unordered_map>

namespace coil {

// ErrorInfo implementation
std::string ErrorInfo::toString() const {
  std::stringstream ss;
  
  // Add severity
  switch (severity) {
      case ErrorSeverity::ERROR:
          ss << "error";
          break;
      case ErrorSeverity::WARNING:
          ss << "warning";
          break;
      case ErrorSeverity::NOTE:
          ss << "note";
          break;
  }
  
  // Add category
  uint8_t category = ErrorManager::getErrorCategory(error_code);
  switch (category) {
      case ErrorCategory::COMPILATION:
          ss << ":compilation";
          break;
      case ErrorCategory::LINKING:
          ss << ":linking";
          break;
      case ErrorCategory::VALIDATION:
          ss << ":validation";
          break;
      case ErrorCategory::RUNTIME:
          ss << ":runtime";
          break;
      default:
          ss << ":unknown";
          break;
  }
  
  // Add file info if available
  if (file_id != 0) {
      ss << ":" << file_id;
      
      if (line != 0) {
          ss << ":" << line;
          
          if (column != 0) {
              ss << ":" << column;
          }
      }
  }
  
  // Add error code and message
  ss << ": 0x" << std::hex << std::setw(8) << std::setfill('0') << error_code
      << " - " << message;
  
  return ss.str();
}

// ErrorManager implementation
void ErrorManager::addError(uint32_t errorCode, const std::string& message, ErrorSeverity severity,
                          uint32_t location, uint32_t fileId, uint32_t line, uint32_t column,
                          uint16_t symbolIndex, uint16_t sectionIndex) {
  ErrorInfo error;
  error.error_code = errorCode;
  error.location = location;
  error.file_id = fileId;
  error.line = line;
  error.column = column;
  error.symbol_index = symbolIndex;
  error.section_index = sectionIndex;
  error.message = message;
  error.severity = severity;
  
  errors_.push_back(error);
}

void ErrorManager::addStandardError(uint32_t errorCode, ErrorSeverity severity,
                                uint32_t location, uint32_t fileId, uint32_t line, uint32_t column,
                                uint16_t symbolIndex, uint16_t sectionIndex) {
  std::string message = getStandardErrorMessage(errorCode);
  addError(errorCode, message, severity, location, fileId, line, column, symbolIndex, sectionIndex);
}

bool ErrorManager::hasErrors() const {
  return !errors_.empty();
}

bool ErrorManager::hasErrors(ErrorSeverity severity) const {
  for (const auto& error : errors_) {
      if (error.severity == severity) {
          return true;
      }
  }
  
  return false;
}

const std::vector<ErrorInfo>& ErrorManager::getErrors() const {
  return errors_;
}

std::vector<ErrorInfo> ErrorManager::getErrors(ErrorSeverity severity) const {
  std::vector<ErrorInfo> result;
  
  for (const auto& error : errors_) {
      if (error.severity == severity) {
          result.push_back(error);
      }
  }
  
  return result;
}

void ErrorManager::clear() {
  errors_.clear();
}

std::string ErrorManager::getStandardErrorMessage(uint32_t errorCode) {
  static const std::unordered_map<uint32_t, std::string> errorMessages = {
      // Compilation - Syntax Errors (0x0100xx)
      {ErrorCode::INVALID_TOKEN, "Invalid token"},
      {ErrorCode::UNEXPECTED_EOF, "Unexpected end of file"},
      {ErrorCode::MISSING_OPERAND, "Missing operand"},
      {ErrorCode::EXTRA_OPERAND, "Extra operand"},
      {ErrorCode::INVALID_LABEL, "Invalid label"},
      
      // Compilation - Variable Errors (0x0103xx)
      {ErrorCode::VARIABLE_ALREADY_DEFINED, "Variable already defined"},
      {ErrorCode::VARIABLE_NOT_DEFINED, "Variable not defined"},
      {ErrorCode::INVALID_VARIABLE_ID, "Invalid variable ID"},
      
      // Compilation - Type Errors (0x0104xx)
      {ErrorCode::INVALID_TYPE, "Invalid type"},
      {ErrorCode::TYPE_MISMATCH, "Type mismatch"},
      
      // Runtime - Arithmetic Errors (0x0400xx)
      {ErrorCode::DIVISION_BY_ZERO, "Division by zero"},
      {ErrorCode::INTEGER_OVERFLOW, "Integer overflow"},
      {ErrorCode::INTEGER_UNDERFLOW, "Integer underflow"},
      {ErrorCode::FLOAT_OVERFLOW, "Floating-point overflow"},
      {ErrorCode::FLOAT_UNDERFLOW, "Floating-point underflow"},
      
      // Runtime - Memory Errors (0x0401xx)
      {ErrorCode::NULL_POINTER_DEREFERENCE, "Null pointer dereference"},
      {ErrorCode::OUT_OF_BOUNDS_ACCESS, "Out of bounds memory access"},
      {ErrorCode::MISALIGNED_ACCESS, "Misaligned memory access"},
      {ErrorCode::MEMORY_LEAK, "Memory leak detected"}
  };
  
  auto it = errorMessages.find(errorCode);
  if (it != errorMessages.end()) {
      return it->second;
  }
  
  // Create a generic message for unknown error codes
  std::stringstream ss;
  uint8_t category = getErrorCategory(errorCode);
  uint8_t subcategory = getErrorSubcategory(errorCode);
  uint16_t specificError = getSpecificError(errorCode);
  
  ss << "Error code 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(category)
      << std::setw(2) << static_cast<int>(subcategory)
      << std::setw(4) << specificError;
  
  return ss.str();
}

} // namespace coil