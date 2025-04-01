#ifndef COIL_BINARY_UTILS_H
#define COIL_BINARY_UTILS_H

#include <cstdint>
#include <vector>
#include <string>

namespace coil {
namespace utils {

/**
* Utilities for working with binary data
*/
class BinaryUtils {
public:
  // Read integers from binary data
  static uint8_t readUint8(const std::vector<uint8_t>& data, size_t offset);
  static uint16_t readUint16(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  static uint32_t readUint32(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  static uint64_t readUint64(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  
  static int8_t readInt8(const std::vector<uint8_t>& data, size_t offset);
  static int16_t readInt16(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  static int32_t readInt32(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  static int64_t readInt64(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  
  // Read floating point from binary data
  static float readFloat(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  static double readDouble(const std::vector<uint8_t>& data, size_t offset, bool bigEndian = false);
  
  // Write integers to binary data
  static void writeUint8(std::vector<uint8_t>& data, size_t offset, uint8_t value);
  static void writeUint16(std::vector<uint8_t>& data, size_t offset, uint16_t value, bool bigEndian = false);
  static void writeUint32(std::vector<uint8_t>& data, size_t offset, uint32_t value, bool bigEndian = false);
  static void writeUint64(std::vector<uint8_t>& data, size_t offset, uint64_t value, bool bigEndian = false);
  
  static void writeInt8(std::vector<uint8_t>& data, size_t offset, int8_t value);
  static void writeInt16(std::vector<uint8_t>& data, size_t offset, int16_t value, bool bigEndian = false);
  static void writeInt32(std::vector<uint8_t>& data, size_t offset, int32_t value, bool bigEndian = false);
  static void writeInt64(std::vector<uint8_t>& data, size_t offset, int64_t value, bool bigEndian = false);
  
  // Write floating point to binary data
  static void writeFloat(std::vector<uint8_t>& data, size_t offset, float value, bool bigEndian = false);
  static void writeDouble(std::vector<uint8_t>& data, size_t offset, double value, bool bigEndian = false);
  
  // Append values to binary data
  static void appendUint8(std::vector<uint8_t>& data, uint8_t value);
  static void appendUint16(std::vector<uint8_t>& data, uint16_t value, bool bigEndian = false);
  static void appendUint32(std::vector<uint8_t>& data, uint32_t value, bool bigEndian = false);
  static void appendUint64(std::vector<uint8_t>& data, uint64_t value, bool bigEndian = false);
  
  static void appendInt8(std::vector<uint8_t>& data, int8_t value);
  static void appendInt16(std::vector<uint8_t>& data, int16_t value, bool bigEndian = false);
  static void appendInt32(std::vector<uint8_t>& data, int32_t value, bool bigEndian = false);
  static void appendInt64(std::vector<uint8_t>& data, int64_t value, bool bigEndian = false);
  
  static void appendFloat(std::vector<uint8_t>& data, float value, bool bigEndian = false);
  static void appendDouble(std::vector<uint8_t>& data, double value, bool bigEndian = false);
  
  // Append strings to binary data
  static void appendString(std::vector<uint8_t>& data, const std::string& value);
  static void appendStringWithLength(std::vector<uint8_t>& data, const std::string& value, bool bigEndian = false);
  
  // Read strings from binary data
  static std::string readString(const std::vector<uint8_t>& data, size_t offset, size_t length);
  static std::string readNullTerminatedString(const std::vector<uint8_t>& data, size_t& offset);
  static std::string readLengthPrefixedString(const std::vector<uint8_t>& data, size_t& offset, bool bigEndian = false);
  
  // Check if machine is big endian
  static bool isBigEndian();
  
  // Swap endianness
  static uint16_t swapEndian16(uint16_t value);
  static uint32_t swapEndian32(uint32_t value);
  static uint64_t swapEndian64(uint64_t value);
  static float swapEndianFloat(float value);
  static double swapEndianDouble(double value);
};

} // namespace utils
} // namespace coil

#endif // COIL_BINARY_UTILS_H