#ifndef COIL_TYPE_SYSTEM_H
#define COIL_TYPE_SYSTEM_H

#include <cstdint>
#include <string>
#include <vector>

namespace coil {

namespace Type {
  // Integer Types
  constexpr uint16_t INT8  = 0x0100;
  constexpr uint16_t INT16 = 0x0200;
  constexpr uint16_t INT32 = 0x0300;
  constexpr uint16_t INT64 = 0x0400;
  constexpr uint16_t UNT8  = 0x1000;
  constexpr uint16_t UNT16 = 0x1100;
  constexpr uint16_t UNT32 = 0x1300;
  constexpr uint16_t UNT64 = 0x1400;
  
  // Floating-Point Types
  constexpr uint16_t FP16  = 0x2300;
  constexpr uint16_t FP32  = 0x2500;
  constexpr uint16_t FP64  = 0x2600;
  constexpr uint16_t FP128 = 0x2800;
  
  // Vector Types
  constexpr uint16_t V128  = 0x3000;
  constexpr uint16_t V256  = 0x3100;
  constexpr uint16_t V512  = 0x3200;
  
  // Special Types
  constexpr uint16_t BIT   = 0x4000;
  constexpr uint16_t VOID  = 0xFF00;
  
  // Platform-Dependent Types
  constexpr uint16_t INT   = 0xA000;
  constexpr uint16_t UNT   = 0xA100;
  constexpr uint16_t FP    = 0xA200;
  constexpr uint16_t PTR   = 0xA600;
  
  // Reference Types
  constexpr uint16_t VAR   = 0x9000;
  constexpr uint16_t SYM   = 0x9100;
  constexpr uint16_t RGP   = 0x9200;  // General purpose register
  constexpr uint16_t RFP   = 0x9300;  // Floating point register
  constexpr uint16_t RV    = 0x9400;  // Vector register
  
  // Composite Types
  constexpr uint16_t STRUCT = 0xD000;
  constexpr uint16_t PACK   = 0xD100;
  constexpr uint16_t UNION  = 0xD200;
  constexpr uint16_t ARRAY  = 0xD300;
  
  // Parameter Types
  constexpr uint16_t PARAM4 = 0xFA00;
  constexpr uint16_t PARAM3 = 0xFB00;
  constexpr uint16_t PARAM2 = 0xFC00;
  constexpr uint16_t PARAM1 = 0xFD00;
  constexpr uint16_t PARAM0 = 0xFE00;

  // Type Extensions
  constexpr uint8_t CONST    = 0x01;
  constexpr uint8_t VOLATILE = 0x02;
  constexpr uint8_t IMM      = 0x20;  // Immediate value
  constexpr uint8_t VAR_ID   = 0x40;  // Variable ID
  constexpr uint8_t SYM_ID   = 0x80;  // Symbol ID
}

/**
* TypeInfo class for working with COIL types
*/
class TypeInfo {
public:
  // Create a basic type
  static uint16_t createType(uint8_t mainType, uint8_t extensions = 0);
  
  // Create a vector type with element type
  static std::vector<uint8_t> createVectorType(uint16_t elementType, uint16_t vectorType);
  
  // Create a composite type
  static std::vector<uint8_t> createCompositeType(uint16_t baseType, const std::vector<uint16_t>& subtypes);
  
  // Get the main type (first 8 bits)
  static uint8_t getMainType(uint16_t type);
  
  // Get the type extensions (second 8 bits)
  static uint8_t getTypeExtensions(uint16_t type);
  
  // Check if a type is a specific category
  static bool isIntegerType(uint16_t type);
  static bool isSignedIntegerType(uint16_t type);
  static bool isUnsignedIntegerType(uint16_t type);
  static bool isFloatType(uint16_t type);
  static bool isVectorType(uint16_t type);
  static bool isPointerType(uint16_t type);
  static bool isReferenceType(uint16_t type);
  static bool isCompositeType(uint16_t type);
  static bool isParameterType(uint16_t type);
  
  // Type compatibility checks
  static bool areTypesCompatible(uint16_t sourceType, uint16_t destType);
  static bool canConvert(uint16_t sourceType, uint16_t destType);
  
  // Type size in bytes
  static uint32_t getTypeSize(uint16_t type);
  
  // Type name for debug info and error messages
  static std::string getTypeName(uint16_t type);
};

/**
* Type Registry for complex types like structs and arrays
*/
class TypeRegistry {
public:
  // Register a complex type
  uint16_t registerType(const std::vector<uint8_t>& typeData);
  
  // Get type info by ID
  const std::vector<uint8_t>& getTypeInfo(uint16_t typeId) const;
  
  // Check if type exists
  bool typeExists(uint16_t typeId) const;
  
  // Clear registry
  void clear();
  
private:
  std::vector<std::vector<uint8_t>> typeRegistry_;
};

} // namespace coil

#endif // COIL_TYPE_SYSTEM_H