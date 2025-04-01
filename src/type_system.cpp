#include "coil/type_system.h"
#include <stdexcept>
#include <unordered_map>
#include <sstream>

namespace coil {

uint16_t TypeInfo::createType(uint8_t mainType, uint8_t extensions) {
  return (static_cast<uint16_t>(mainType) << 8) | extensions;
}

std::vector<uint8_t> TypeInfo::createVectorType(uint16_t elementType, uint16_t vectorType) {
  std::vector<uint8_t> result;
  
  // Vector type
  result.push_back(static_cast<uint8_t>(vectorType >> 8));
  result.push_back(static_cast<uint8_t>(vectorType & 0xFF));
  
  // Element type
  result.push_back(static_cast<uint8_t>(elementType >> 8));
  result.push_back(static_cast<uint8_t>(elementType & 0xFF));
  
  return result;
}

std::vector<uint8_t> TypeInfo::createCompositeType(uint16_t baseType, const std::vector<uint16_t>& subtypes) {
  std::vector<uint8_t> result;
  
  // Base type
  result.push_back(static_cast<uint8_t>(baseType >> 8));
  result.push_back(static_cast<uint8_t>(baseType & 0xFF));
  
  // Number of subtypes
  result.push_back(static_cast<uint8_t>(subtypes.size()));
  result.push_back(static_cast<uint8_t>(subtypes.size() >> 8));
  
  // Subtypes
  for (uint16_t subtype : subtypes) {
      result.push_back(static_cast<uint8_t>(subtype >> 8));
      result.push_back(static_cast<uint8_t>(subtype & 0xFF));
  }
  
  return result;
}

uint8_t TypeInfo::getMainType(uint16_t type) {
  return static_cast<uint8_t>(type >> 8);
}

uint8_t TypeInfo::getTypeExtensions(uint16_t type) {
  return static_cast<uint8_t>(type & 0xFF);
}

bool TypeInfo::isIntegerType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return (mainType >= 0x01 && mainType <= 0x14) || mainType == 0xA0 || mainType == 0xA1;
}

bool TypeInfo::isSignedIntegerType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return (mainType >= 0x01 && mainType <= 0x04) || mainType == 0xA0;
}

bool TypeInfo::isUnsignedIntegerType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return (mainType >= 0x10 && mainType <= 0x14) || mainType == 0xA1;
}

bool TypeInfo::isFloatType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return (mainType >= 0x23 && mainType <= 0x28) || mainType == 0xA2;
}

bool TypeInfo::isVectorType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return mainType >= 0x30 && mainType <= 0x32;
}

bool TypeInfo::isPointerType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return mainType == 0xA6;
}

bool TypeInfo::isReferenceType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return mainType >= 0x90 && mainType <= 0x94;
}

bool TypeInfo::isCompositeType(uint16_t type) {
  uint8_t mainType = getMainType(type);
  return mainType >= 0xD0 && mainType <= 0xD3;
}

bool TypeInfo::areTypesCompatible(uint16_t sourceType, uint16_t destType) {
  // Exact match is always compatible
  if (sourceType == destType) {
      return true;
  }
  
  // Get main types
  uint8_t sourceMain = getMainType(sourceType);
  uint8_t destMain = getMainType(destType);
  
  // Platform types match their fixed-width equivalents
  if (sourceMain == 0xA0 && (destMain == 0x01 || destMain == 0x02 || destMain == 0x03 || destMain == 0x04)) {
      return true;
  }
  if (sourceMain == 0xA1 && (destMain == 0x10 || destMain == 0x11 || destMain == 0x13 || destMain == 0x14)) {
      return true;
  }
  if (sourceMain == 0xA2 && (destMain == 0x23 || destMain == 0x25 || destMain == 0x26 || destMain == 0x28)) {
      return true;
  }
  
  // Integer widening (signed to signed, unsigned to unsigned)
  if (isSignedIntegerType(sourceType) && isSignedIntegerType(destType)) {
      return getTypeSize(sourceType) <= getTypeSize(destType);
  }
  if (isUnsignedIntegerType(sourceType) && isUnsignedIntegerType(destType)) {
      return getTypeSize(sourceType) <= getTypeSize(destType);
  }
  
  // Float precision widening
  if (isFloatType(sourceType) && isFloatType(destType)) {
      return getTypeSize(sourceType) <= getTypeSize(destType);
  }
  
  // Otherwise, not compatible
  return false;
}

bool TypeInfo::canConvert(uint16_t sourceType, uint16_t destType) {
  // Already compatible
  if (areTypesCompatible(sourceType, destType)) {
      return true;
  }
  
  // Integer to float
  if (isIntegerType(sourceType) && isFloatType(destType)) {
      return true;
  }
  
  // Float to integer
  if (isFloatType(sourceType) && isIntegerType(destType)) {
      return true;
  }
  
  // Signed to unsigned, or unsigned to signed (with possible data loss)
  if (isIntegerType(sourceType) && isIntegerType(destType)) {
      return true;
  }
  
  // Otherwise, can't convert
  return false;
}

uint32_t TypeInfo::getTypeSize(uint16_t type) {
  uint8_t mainType = getMainType(type);
  
  switch (mainType) {
      // Integer types
      case 0x01: return 1; // INT8
      case 0x02: return 2; // INT16
      case 0x03: return 4; // INT32
      case 0x04: return 8; // INT64
      case 0x10: return 1; // UNT8
      case 0x11: return 2; // UNT16
      case 0x13: return 4; // UNT32
      case 0x14: return 8; // UNT64
      
      // Float types
      case 0x23: return 2; // FP16
      case 0x25: return 4; // FP32
      case 0x26: return 8; // FP64
      case 0x28: return 16; // FP128
      
      // Vector types
      case 0x30: return 16; // V128
      case 0x31: return 32; // V256
      case 0x32: return 64; // V512
      
      // Special types
      case 0x40: return 1; // BIT
      case 0xFF: return 0; // VOID
      
      // Platform-dependent types
      case 0xA0: return 4; // INT (assuming 32-bit)
      case 0xA1: return 4; // UNT (assuming 32-bit)
      case 0xA2: return 4; // FP (assuming 32-bit)
      case 0xA6: return 8; // PTR (assuming 64-bit)
      
      // Reference types
      case 0x90: // VAR
      case 0x91: // SYM
      case 0x92: // RGP
      case 0x93: // RFP
      case 0x94: // RV
          return 8; // All references are 64-bit
      
      // Composite types (would need more context)
      case 0xD0: // STRUCT
      case 0xD1: // PACK
      case 0xD2: // UNION
      case 0xD3: // ARRAY
          return 0; // Size depends on the specific composite type
      
      default:
          return 0; // Unknown type
  }
}

std::string TypeInfo::getTypeName(uint16_t type) {
  uint8_t mainType = getMainType(type);
  uint8_t extensions = getTypeExtensions(type);
  std::string result;
  
  // Handle the main type
  switch (mainType) {
      // Integer types
      case 0x01: result = "INT8"; break;
      case 0x02: result = "INT16"; break;
      case 0x03: result = "INT32"; break;
      case 0x04: result = "INT64"; break;
      case 0x10: result = "UNT8"; break;
      case 0x11: result = "UNT16"; break;
      case 0x13: result = "UNT32"; break;
      case 0x14: result = "UNT64"; break;
      
      // Float types
      case 0x23: result = "FP16"; break;
      case 0x25: result = "FP32"; break;
      case 0x26: result = "FP64"; break;
      case 0x28: result = "FP128"; break;
      
      // Vector types
      case 0x30: result = "V128"; break;
      case 0x31: result = "V256"; break;
      case 0x32: result = "V512"; break;
      
      // Special types
      case 0x40: result = "BIT"; break;
      case 0xFF: result = "VOID"; break;
      
      // Platform-dependent types
      case 0xA0: result = "INT"; break;
      case 0xA1: result = "UNT"; break;
      case 0xA2: result = "FP"; break;
      case 0xA6: result = "PTR"; break;
      
      // Reference types
      case 0x90: result = "VAR"; break;
      case 0x91: result = "SYM"; break;
      case 0x92: result = "RGP"; break;
      case 0x93: result = "RFP"; break;
      case 0x94: result = "RV"; break;
      
      // Composite types
      case 0xD0: result = "STRUCT"; break;
      case 0xD1: result = "PACK"; break;
      case 0xD2: result = "UNION"; break;
      case 0xD3: result = "ARRAY"; break;
      
      default:
          std::stringstream ss;
          ss << "0x" << std::hex << static_cast<int>(mainType);
          result = "UNKNOWN(" + ss.str() + ")";
          break;
  }
  
  // Add extensions
  if (extensions & Type::CONST) {
      result += "+CONST";
  }
  if (extensions & Type::VOLATILE) {
      result += "+VOLATILE";
  }
  if (extensions & Type::IMM) {
      result += "+IMM";
  }
  if (extensions & Type::VAR_ID) {
      result += "+VAR_ID";
  }
  if (extensions & Type::SYM_ID) {
      result += "+SYM_ID";
  }
  
  return result;
}

// TypeRegistry implementation
uint16_t TypeRegistry::registerType(const std::vector<uint8_t>& typeData) {
  typeRegistry_.push_back(typeData);
  return static_cast<uint16_t>(typeRegistry_.size() - 1);
}

const std::vector<uint8_t>& TypeRegistry::getTypeInfo(uint16_t typeId) const {
  if (typeId >= typeRegistry_.size()) {
      throw std::out_of_range("Type ID out of range");
  }
  return typeRegistry_[typeId];
}

bool TypeRegistry::typeExists(uint16_t typeId) const {
  return typeId < typeRegistry_.size();
}

void TypeRegistry::clear() {
  typeRegistry_.clear();
}

} // namespace coil