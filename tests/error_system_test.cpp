#include "test_helper.h"

bool testErrorCodes() {
  // Test error code creation
  uint32_t code1 = coil::ErrorManager::makeErrorCode(
      coil::ErrorCategory::COMPILATION,
      coil::CompilationSubcategory::SYNTAX,
      0x0001
  );
  
  uint32_t code2 = coil::ErrorManager::makeErrorCode(
      coil::ErrorCategory::RUNTIME,
      coil::RuntimeSubcategory::MEMORY,
      0x0002
  );
  
  // Test error code extraction
  TEST_ASSERT(coil::ErrorManager::getErrorCategory(code1) == coil::ErrorCategory::COMPILATION);
  TEST_ASSERT(coil::ErrorManager::getErrorSubcategory(code1) == coil::CompilationSubcategory::SYNTAX);
  TEST_ASSERT(coil::ErrorManager::getSpecificError(code1) == 0x0001);
  
  TEST_ASSERT(coil::ErrorManager::getErrorCategory(code2) == coil::ErrorCategory::RUNTIME);
  TEST_ASSERT(coil::ErrorManager::getErrorSubcategory(code2) == coil::RuntimeSubcategory::MEMORY);
  TEST_ASSERT(coil::ErrorManager::getSpecificError(code2) == 0x0002);
  
  // Check predefined error codes
  TEST_ASSERT(coil::ErrorManager::getErrorCategory(coil::ErrorCode::INVALID_TOKEN) == coil::ErrorCategory::COMPILATION);
  TEST_ASSERT(coil::ErrorManager::getErrorSubcategory(coil::ErrorCode::INVALID_TOKEN) == coil::CompilationSubcategory::SYNTAX);
  
  TEST_ASSERT(coil::ErrorManager::getErrorCategory(coil::ErrorCode::TYPE_MISMATCH) == coil::ErrorCategory::COMPILATION);
  TEST_ASSERT(coil::ErrorManager::getErrorSubcategory(coil::ErrorCode::TYPE_MISMATCH) == coil::CompilationSubcategory::TYPE);
  
  TEST_ASSERT(coil::ErrorManager::getErrorCategory(coil::ErrorCode::DIVISION_BY_ZERO) == coil::ErrorCategory::RUNTIME);
  TEST_ASSERT(coil::ErrorManager::getErrorSubcategory(coil::ErrorCode::DIVISION_BY_ZERO) == coil::RuntimeSubcategory::ARITHMETIC);
  
  return true;
}

bool testErrorInfo() {
  // Create an error info structure
  coil::ErrorInfo error;
  error.error_code = coil::ErrorCode::INVALID_TOKEN;
  error.location = 100;
  error.file_id = 1;
  error.line = 10;
  error.column = 20;
  error.symbol_index = 5;
  error.section_index = 2;
  error.message = "Test error message";
  error.severity = coil::ErrorSeverity::ERROR;
  
  // Test toString
  std::string errorString = error.toString();
  
  // Should contain key information
  TEST_ASSERT(errorString.find("error") != std::string::npos);
  TEST_ASSERT(errorString.find("compilation") != std::string::npos);
  TEST_ASSERT(errorString.find("1:10:20") != std::string::npos);
  TEST_ASSERT(errorString.find("Test error message") != std::string::npos);
  
  // Create a warning
  coil::ErrorInfo warning;
  warning.error_code = coil::ErrorCode::TYPE_MISMATCH;
  warning.message = "Test warning message";
  warning.severity = coil::ErrorSeverity::WARNING;
  
  // Test toString
  std::string warningString = warning.toString();
  
  // Should contain key information
  TEST_ASSERT(warningString.find("warning") != std::string::npos);
  TEST_ASSERT(warningString.find("compilation") != std::string::npos);
  TEST_ASSERT(warningString.find("Test warning message") != std::string::npos);
  
  return true;
}

bool testErrorManager() {
  coil::ErrorManager manager;
  
  // Initially should have no errors
  TEST_ASSERT(!manager.hasErrors());
  TEST_ASSERT(!manager.hasErrors(coil::ErrorSeverity::ERROR));
  TEST_ASSERT(!manager.hasErrors(coil::ErrorSeverity::WARNING));
  TEST_ASSERT(!manager.hasErrors(coil::ErrorSeverity::NOTE));
  
  // Add an error
  manager.addError(
      coil::ErrorCode::INVALID_TOKEN,
      "Invalid token",
      coil::ErrorSeverity::ERROR,
      100, 1, 10, 20, 5, 2
  );
  
  // Should have an error now
  TEST_ASSERT(manager.hasErrors());
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::ERROR));
  TEST_ASSERT(!manager.hasErrors(coil::ErrorSeverity::WARNING));
  
  // Add a warning
  manager.addError(
      coil::ErrorCode::TYPE_MISMATCH,
      "Type mismatch",
      coil::ErrorSeverity::WARNING
  );
  
  // Should have both error and warning
  TEST_ASSERT(manager.hasErrors());
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::ERROR));
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::WARNING));
  
  // Get all errors
  auto allErrors = manager.getErrors();
  TEST_ASSERT(allErrors.size() == 2);
  
  // Get errors by severity
  auto errors = manager.getErrors(coil::ErrorSeverity::ERROR);
  TEST_ASSERT(errors.size() == 1);
  TEST_ASSERT(errors[0].error_code == coil::ErrorCode::INVALID_TOKEN);
  
  auto warnings = manager.getErrors(coil::ErrorSeverity::WARNING);
  TEST_ASSERT(warnings.size() == 1);
  TEST_ASSERT(warnings[0].error_code == coil::ErrorCode::TYPE_MISMATCH);
  
  // Clear errors
  manager.clear();
  
  // Should have no errors again
  TEST_ASSERT(!manager.hasErrors());
  
  return true;
}

bool testStandardErrors() {
  coil::ErrorManager manager;
  
  // Add a standard error
  manager.addStandardError(
      coil::ErrorCode::INVALID_TOKEN,
      coil::ErrorSeverity::ERROR,
      100, 1, 10, 20, 5, 2
  );
  
  // Should have an error now
  TEST_ASSERT(manager.hasErrors());
  
  // Get the error
  auto errors = manager.getErrors();
  TEST_ASSERT(errors.size() == 1);
  
  // Should have standard message
  std::string standardMessage = coil::ErrorManager::getStandardErrorMessage(coil::ErrorCode::INVALID_TOKEN);
  TEST_ASSERT(!standardMessage.empty());
  TEST_ASSERT(errors[0].message == standardMessage);
  
  return true;
}

bool testErrorHierarchy() {
  coil::ErrorManager manager;
  
  // Add errors of different severities
  manager.addError(
      coil::ErrorCode::NULL_POINTER_DEREFERENCE,
      "Null pointer dereference",
      coil::ErrorSeverity::ERROR
  );
  
  manager.addError(
      coil::ErrorCode::TYPE_MISMATCH,
      "Type mismatch warning",
      coil::ErrorSeverity::WARNING
  );
  
  manager.addError(
      coil::ErrorCode::VARIABLE_ALREADY_DEFINED,
      "Variable already defined note",
      coil::ErrorSeverity::NOTE
  );
  
  // Should have errors of all severities
  TEST_ASSERT(manager.hasErrors());
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::ERROR));
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::WARNING));
  TEST_ASSERT(manager.hasErrors(coil::ErrorSeverity::NOTE));
  
  // Get all errors
  auto allErrors = manager.getErrors();
  TEST_ASSERT(allErrors.size() == 3);
  
  // Get by specific severities
  TEST_ASSERT(manager.getErrors(coil::ErrorSeverity::ERROR).size() == 1);
  TEST_ASSERT(manager.getErrors(coil::ErrorSeverity::WARNING).size() == 1);
  TEST_ASSERT(manager.getErrors(coil::ErrorSeverity::NOTE).size() == 1);
  
  // Clear and check again
  manager.clear();
  TEST_ASSERT(!manager.hasErrors());
  TEST_ASSERT(manager.getErrors().empty());
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"Error Codes", testErrorCodes},
      {"Error Info", testErrorInfo},
      {"Error Manager", testErrorManager},
      {"Standard Errors", testStandardErrors},
      {"Error Hierarchy", testErrorHierarchy}
  };
  
  return runTests(tests);
}