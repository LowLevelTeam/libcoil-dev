#include "test_helper.h"

bool testTypeCreation() {
  // Create basic types
  uint16_t int32Type = coil::TypeInfo::createType(coil::TypeInfo::getMainType(coil::Type::INT32));
  uint16_t fp32Type = coil::TypeInfo::createType(coil::TypeInfo::getMainType(coil::Type::FP32));
  uint16_t constInt32Type = coil::TypeInfo::createType(coil::TypeInfo::getMainType(coil::Type::INT32), coil::Type::CONST);
  
  // Check main type extraction
  TEST_ASSERT(coil::TypeInfo::getMainType(int32Type) == coil::TypeInfo::getMainType(coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::getMainType(fp32Type) == coil::TypeInfo::getMainType(coil::Type::FP32));
  
  // Check type extensions
  TEST_ASSERT(coil::TypeInfo::getTypeExtensions(int32Type) == 0);
  TEST_ASSERT(coil::TypeInfo::getTypeExtensions(constInt32Type) == coil::Type::CONST);
  
  // Create complex types
  std::vector<uint8_t> vectorType = coil::TypeInfo::createVectorType(int32Type, coil::Type::V128);
  TEST_ASSERT(vectorType.size() == 4);  // 2 bytes for vector type + 2 bytes for element type

  std::vector<uint16_t> structFields = {int32Type, fp32Type};
  std::vector<uint8_t> structType = coil::TypeInfo::createCompositeType(coil::Type::STRUCT, structFields);
  TEST_ASSERT(structType.size() == 8);  // 2 bytes for struct type + 2 bytes for count + 2 bytes per field (2 fields * 2 bytes = 4 bytes)

  return true;
}

bool testTypeInformation() {
  // Test type category checks
  TEST_ASSERT(coil::TypeInfo::isIntegerType(coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::isSignedIntegerType(coil::Type::INT32));
  TEST_ASSERT(!coil::TypeInfo::isUnsignedIntegerType(coil::Type::INT32));
  
  TEST_ASSERT(coil::TypeInfo::isIntegerType(coil::Type::UNT32));
  TEST_ASSERT(!coil::TypeInfo::isSignedIntegerType(coil::Type::UNT32));
  TEST_ASSERT(coil::TypeInfo::isUnsignedIntegerType(coil::Type::UNT32));
  
  TEST_ASSERT(coil::TypeInfo::isFloatType(coil::Type::FP32));
  TEST_ASSERT(!coil::TypeInfo::isIntegerType(coil::Type::FP32));
  
  TEST_ASSERT(coil::TypeInfo::isVectorType(coil::Type::V128));
  TEST_ASSERT(coil::TypeInfo::isPointerType(coil::Type::PTR));
  TEST_ASSERT(coil::TypeInfo::isReferenceType(coil::Type::VAR));
  
  TEST_ASSERT(coil::TypeInfo::isCompositeType(coil::Type::STRUCT));
  TEST_ASSERT(coil::TypeInfo::isCompositeType(coil::Type::ARRAY));
  
  // Test type sizes
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::INT8) == 1);
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::INT16) == 2);
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::INT32) == 4);
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::INT64) == 8);
  
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::FP32) == 4);
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::FP64) == 8);
  
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::V128) == 16);
  TEST_ASSERT(coil::TypeInfo::getTypeSize(coil::Type::V256) == 32);
  
  // Test type name
  TEST_ASSERT(coil::TypeInfo::getTypeName(coil::Type::INT32) == "INT32");
  TEST_ASSERT(coil::TypeInfo::getTypeName(coil::Type::FP64) == "FP64");
  
  // Test with type extensions
  uint16_t constInt32 = coil::Type::INT32 | coil::Type::CONST;
  TEST_ASSERT(coil::TypeInfo::getTypeName(constInt32) == "INT32+CONST");
  
  return true;
}

bool testTypeCompatibility() {
  // Test exact match
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::INT32, coil::Type::INT32));
  
  // Test integer widening
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::INT8, coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::INT16, coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::INT32, coil::Type::INT64));
  
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::UNT8, coil::Type::UNT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::UNT16, coil::Type::UNT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::UNT32, coil::Type::UNT64));
  
  // Test float precision widening
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::FP32, coil::Type::FP64));
  
  // Test platform types
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::INT, coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::UNT, coil::Type::UNT32));
  TEST_ASSERT(coil::TypeInfo::areTypesCompatible(coil::Type::FP, coil::Type::FP32));
  
  // Test incompatible types
  TEST_ASSERT(!coil::TypeInfo::areTypesCompatible(coil::Type::INT32, coil::Type::FP32));
  TEST_ASSERT(!coil::TypeInfo::areTypesCompatible(coil::Type::INT32, coil::Type::UNT32));
  
  // Test type conversion possibilities
  TEST_ASSERT(coil::TypeInfo::canConvert(coil::Type::INT32, coil::Type::FP32));
  TEST_ASSERT(coil::TypeInfo::canConvert(coil::Type::FP32, coil::Type::INT32));
  TEST_ASSERT(coil::TypeInfo::canConvert(coil::Type::INT32, coil::Type::UNT32));
  
  return true;
}

bool testTypeRegistry() {
  coil::TypeRegistry registry;
  
  // Create a vector type description
  std::vector<uint8_t> vectorType = coil::TypeInfo::createVectorType(coil::Type::FP32, coil::Type::V128);
  
  // Register the type
  uint16_t typeId = registry.registerType(vectorType);
  TEST_ASSERT(typeId == 0);  // First registered type gets ID 0
  
  // Create a struct type
  std::vector<uint16_t> structFields = {coil::Type::INT32, coil::Type::FP64};
  std::vector<uint8_t> structType = coil::TypeInfo::createCompositeType(coil::Type::STRUCT, structFields);
  
  // Register the struct type
  uint16_t structTypeId = registry.registerType(structType);
  TEST_ASSERT(structTypeId == 1);  // Second registered type gets ID 1
  
  // Get type info back from registry
  const auto& retrievedVectorType = registry.getTypeInfo(typeId);
  TEST_ASSERT(retrievedVectorType.size() == vectorType.size());
  TEST_ASSERT(std::equal(retrievedVectorType.begin(), retrievedVectorType.end(), vectorType.begin()));
  
  // Check existence
  TEST_ASSERT(registry.typeExists(typeId));
  TEST_ASSERT(registry.typeExists(structTypeId));
  TEST_ASSERT(!registry.typeExists(100));
  
  // Clear registry
  registry.clear();
  TEST_ASSERT(!registry.typeExists(typeId));
  TEST_ASSERT(!registry.typeExists(structTypeId));
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"Type Creation", testTypeCreation},
      {"Type Information", testTypeInformation},
      {"Type Compatibility", testTypeCompatibility},
      {"Type Registry", testTypeRegistry}
  };
  
  return runTests(tests);
}