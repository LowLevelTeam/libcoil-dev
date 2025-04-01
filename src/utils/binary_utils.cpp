#include "coil/utils/binary_utils.h"
#include <cstring>
#include <stdexcept>

namespace coil {
namespace utils {

uint8_t BinaryUtils::readUint8(const std::vector<uint8_t>& data, size_t offset) {
  if (offset >= data.size()) {
      throw std::out_of_range("Offset out of range in readUint8");
  }
  
  return data[offset];
}

uint16_t BinaryUtils::readUint16(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  if (offset + 1 >= data.size()) {
      throw std::out_of_range("Offset out of range in readUint16");
  }
  
  uint16_t value;
  if (!bigEndian) {
      // Little endian (default)
      value = static_cast<uint16_t>(data[offset]) |
              (static_cast<uint16_t>(data[offset + 1]) << 8);
  } else {
      // Big endian
      value = (static_cast<uint16_t>(data[offset]) << 8) |
              static_cast<uint16_t>(data[offset + 1]);
  }
  
  return value;
}

uint32_t BinaryUtils::readUint32(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  if (offset + 3 >= data.size()) {
      throw std::out_of_range("Offset out of range in readUint32");
  }
  
  uint32_t value;
  if (!bigEndian) {
      // Little endian (default)
      value = static_cast<uint32_t>(data[offset]) |
              (static_cast<uint32_t>(data[offset + 1]) << 8) |
              (static_cast<uint32_t>(data[offset + 2]) << 16) |
              (static_cast<uint32_t>(data[offset + 3]) << 24);
  } else {
      // Big endian
      value = (static_cast<uint32_t>(data[offset]) << 24) |
              (static_cast<uint32_t>(data[offset + 1]) << 16) |
              (static_cast<uint32_t>(data[offset + 2]) << 8) |
              static_cast<uint32_t>(data[offset + 3]);
  }
  
  return value;
}

uint64_t BinaryUtils::readUint64(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  if (offset + 7 >= data.size()) {
      throw std::out_of_range("Offset out of range in readUint64");
  }
  
  uint64_t value;
  if (!bigEndian) {
      // Little endian (default)
      value = static_cast<uint64_t>(data[offset]) |
              (static_cast<uint64_t>(data[offset + 1]) << 8) |
              (static_cast<uint64_t>(data[offset + 2]) << 16) |
              (static_cast<uint64_t>(data[offset + 3]) << 24) |
              (static_cast<uint64_t>(data[offset + 4]) << 32) |
              (static_cast<uint64_t>(data[offset + 5]) << 40) |
              (static_cast<uint64_t>(data[offset + 6]) << 48) |
              (static_cast<uint64_t>(data[offset + 7]) << 56);
  } else {
      // Big endian
      value = (static_cast<uint64_t>(data[offset]) << 56) |
              (static_cast<uint64_t>(data[offset + 1]) << 48) |
              (static_cast<uint64_t>(data[offset + 2]) << 40) |
              (static_cast<uint64_t>(data[offset + 3]) << 32) |
              (static_cast<uint64_t>(data[offset + 4]) << 24) |
              (static_cast<uint64_t>(data[offset + 5]) << 16) |
              (static_cast<uint64_t>(data[offset + 6]) << 8) |
              static_cast<uint64_t>(data[offset + 7]);
  }
  
  return value;
}

int8_t BinaryUtils::readInt8(const std::vector<uint8_t>& data, size_t offset) {
  return static_cast<int8_t>(readUint8(data, offset));
}

int16_t BinaryUtils::readInt16(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  return static_cast<int16_t>(readUint16(data, offset, bigEndian));
}

int32_t BinaryUtils::readInt32(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  return static_cast<int32_t>(readUint32(data, offset, bigEndian));
}

int64_t BinaryUtils::readInt64(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  return static_cast<int64_t>(readUint64(data, offset, bigEndian));
}

float BinaryUtils::readFloat(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  if (offset + 3 >= data.size()) {
      throw std::out_of_range("Offset out of range in readFloat");
  }
  
  uint32_t intValue = readUint32(data, offset, bigEndian);
  float result;
  std::memcpy(&result, &intValue, sizeof(float));
  return result;
}

double BinaryUtils::readDouble(const std::vector<uint8_t>& data, size_t offset, bool bigEndian) {
  if (offset + 7 >= data.size()) {
      throw std::out_of_range("Offset out of range in readDouble");
  }
  
  uint64_t intValue = readUint64(data, offset, bigEndian);
  double result;
  std::memcpy(&result, &intValue, sizeof(double));
  return result;
}

void BinaryUtils::writeUint8(std::vector<uint8_t>& data, size_t offset, uint8_t value) {
  if (offset >= data.size()) {
      data.resize(offset + 1);
  }
  
  data[offset] = value;
}

void BinaryUtils::writeUint16(std::vector<uint8_t>& data, size_t offset, uint16_t value, bool bigEndian) {
  if (offset + 1 >= data.size()) {
      data.resize(offset + 2);
  }
  
  if (!bigEndian) {
      // Little endian (default)
      data[offset] = static_cast<uint8_t>(value);
      data[offset + 1] = static_cast<uint8_t>(value >> 8);
  } else {
      // Big endian
      data[offset] = static_cast<uint8_t>(value >> 8);
      data[offset + 1] = static_cast<uint8_t>(value);
  }
}

void BinaryUtils::writeUint32(std::vector<uint8_t>& data, size_t offset, uint32_t value, bool bigEndian) {
  if (offset + 3 >= data.size()) {
      data.resize(offset + 4);
  }
  
  if (!bigEndian) {
      // Little endian (default)
      data[offset] = static_cast<uint8_t>(value);
      data[offset + 1] = static_cast<uint8_t>(value >> 8);
      data[offset + 2] = static_cast<uint8_t>(value >> 16);
      data[offset + 3] = static_cast<uint8_t>(value >> 24);
  } else {
      // Big endian
      data[offset] = static_cast<uint8_t>(value >> 24);
      data[offset + 1] = static_cast<uint8_t>(value >> 16);
      data[offset + 2] = static_cast<uint8_t>(value >> 8);
      data[offset + 3] = static_cast<uint8_t>(value);
  }
}

void BinaryUtils::writeUint64(std::vector<uint8_t>& data, size_t offset, uint64_t value, bool bigEndian) {
  if (offset + 7 >= data.size()) {
      data.resize(offset + 8);
  }
  
  if (!bigEndian) {
      // Little endian (default)
      data[offset] = static_cast<uint8_t>(value);
      data[offset + 1] = static_cast<uint8_t>(value >> 8);
      data[offset + 2] = static_cast<uint8_t>(value >> 16);
      data[offset + 3] = static_cast<uint8_t>(value >> 24);
      data[offset + 4] = static_cast<uint8_t>(value >> 32);
      data[offset + 5] = static_cast<uint8_t>(value >> 40);
      data[offset + 6] = static_cast<uint8_t>(value >> 48);
      data[offset + 7] = static_cast<uint8_t>(value >> 56);
  } else {
      // Big endian
      data[offset] = static_cast<uint8_t>(value >> 56);
      data[offset + 1] = static_cast<uint8_t>(value >> 48);
      data[offset + 2] = static_cast<uint8_t>(value >> 40);
      data[offset + 3] = static_cast<uint8_t>(value >> 32);
      data[offset + 4] = static_cast<uint8_t>(value >> 24);
      data[offset + 5] = static_cast<uint8_t>(value >> 16);
      data[offset + 6] = static_cast<uint8_t>(value >> 8);
      data[offset + 7] = static_cast<uint8_t>(value);
  }
}

void BinaryUtils::writeInt8(std::vector<uint8_t>& data, size_t offset, int8_t value) {
  writeUint8(data, offset, static_cast<uint8_t>(value));
}

void BinaryUtils::writeInt16(std::vector<uint8_t>& data, size_t offset, int16_t value, bool bigEndian) {
  writeUint16(data, offset, static_cast<uint16_t>(value), bigEndian);
}

void BinaryUtils::writeInt32(std::vector<uint8_t>& data, size_t offset, int32_t value, bool bigEndian) {
  writeUint32(data, offset, static_cast<uint32_t>(value), bigEndian);
}

void BinaryUtils::writeInt64(std::vector<uint8_t>& data, size_t offset, int64_t value, bool bigEndian) {
  writeUint64(data, offset, static_cast<uint64_t>(value), bigEndian);
}

void BinaryUtils::writeFloat(std::vector<uint8_t>& data, size_t offset, float value, bool bigEndian) {
  uint32_t intValue;
  std::memcpy(&intValue, &value, sizeof(float));
  writeUint32(data, offset, intValue, bigEndian);
}

void BinaryUtils::writeDouble(std::vector<uint8_t>& data, size_t offset, double value, bool bigEndian) {
  uint64_t intValue;
  std::memcpy(&intValue, &value, sizeof(double));
  writeUint64(data, offset, intValue, bigEndian);
}

void BinaryUtils::appendUint8(std::vector<uint8_t>& data, uint8_t value) {
  data.push_back(value);
}

void BinaryUtils::appendUint16(std::vector<uint8_t>& data, uint16_t value, bool bigEndian) {
  if (!bigEndian) {
      // Little endian (default)
      data.push_back(static_cast<uint8_t>(value));
      data.push_back(static_cast<uint8_t>(value >> 8));
  } else {
      // Big endian
      data.push_back(static_cast<uint8_t>(value >> 8));
      data.push_back(static_cast<uint8_t>(value));
  }
}

void BinaryUtils::appendUint32(std::vector<uint8_t>& data, uint32_t value, bool bigEndian) {
  if (!bigEndian) {
      // Little endian (default)
      data.push_back(static_cast<uint8_t>(value));
      data.push_back(static_cast<uint8_t>(value >> 8));
      data.push_back(static_cast<uint8_t>(value >> 16));
      data.push_back(static_cast<uint8_t>(value >> 24));
  } else {
      // Big endian
      data.push_back(static_cast<uint8_t>(value >> 24));
      data.push_back(static_cast<uint8_t>(value >> 16));
      data.push_back(static_cast<uint8_t>(value >> 8));
      data.push_back(static_cast<uint8_t>(value));
  }
}

void BinaryUtils::appendUint64(std::vector<uint8_t>& data, uint64_t value, bool bigEndian) {
  if (!bigEndian) {
      // Little endian (default)
      data.push_back(static_cast<uint8_t>(value));
      data.push_back(static_cast<uint8_t>(value >> 8));
      data.push_back(static_cast<uint8_t>(value >> 16));
      data.push_back(static_cast<uint8_t>(value >> 24));
      data.push_back(static_cast<uint8_t>(value >> 32));
      data.push_back(static_cast<uint8_t>(value >> 40));
      data.push_back(static_cast<uint8_t>(value >> 48));
      data.push_back(static_cast<uint8_t>(value >> 56));
  } else {
      // Big endian
      data.push_back(static_cast<uint8_t>(value >> 56));
      data.push_back(static_cast<uint8_t>(value >> 48));
      data.push_back(static_cast<uint8_t>(value >> 40));
      data.push_back(static_cast<uint8_t>(value >> 32));
      data.push_back(static_cast<uint8_t>(value >> 24));
      data.push_back(static_cast<uint8_t>(value >> 16));
      data.push_back(static_cast<uint8_t>(value >> 8));
      data.push_back(static_cast<uint8_t>(value));
  }
}

void BinaryUtils::appendInt8(std::vector<uint8_t>& data, int8_t value) {
  appendUint8(data, static_cast<uint8_t>(value));
}

void BinaryUtils::appendInt16(std::vector<uint8_t>& data, int16_t value, bool bigEndian) {
  appendUint16(data, static_cast<uint16_t>(value), bigEndian);
}

void BinaryUtils::appendInt32(std::vector<uint8_t>& data, int32_t value, bool bigEndian) {
  appendUint32(data, static_cast<uint32_t>(value), bigEndian);
}

void BinaryUtils::appendInt64(std::vector<uint8_t>& data, int64_t value, bool bigEndian) {
  appendUint64(data, static_cast<uint64_t>(value), bigEndian);
}

void BinaryUtils::appendFloat(std::vector<uint8_t>& data, float value, bool bigEndian) {
  uint32_t intValue;
  std::memcpy(&intValue, &value, sizeof(float));
  appendUint32(data, intValue, bigEndian);
}

void BinaryUtils::appendDouble(std::vector<uint8_t>& data, double value, bool bigEndian) {
  uint64_t intValue;
  std::memcpy(&intValue, &value, sizeof(double));
  appendUint64(data, intValue, bigEndian);
}

void BinaryUtils::appendString(std::vector<uint8_t>& data, const std::string& value) {
  data.insert(data.end(), value.begin(), value.end());
}

void BinaryUtils::appendStringWithLength(std::vector<uint8_t>& data, const std::string& value, bool bigEndian) {
  uint32_t length = static_cast<uint32_t>(value.length());
  appendUint32(data, length, bigEndian);
  appendString(data, value);
}

std::string BinaryUtils::readString(const std::vector<uint8_t>& data, size_t offset, size_t length) {
  if (offset + length > data.size()) {
      throw std::out_of_range("Offset out of range in readString");
  }
  
  return std::string(reinterpret_cast<const char*>(data.data() + offset), length);
}

std::string BinaryUtils::readNullTerminatedString(const std::vector<uint8_t>& data, size_t& offset) {
  std::string result;
  
  while (offset < data.size() && data[offset] != 0) {
      result.push_back(static_cast<char>(data[offset]));
      offset++;
  }
  
  // Skip the null terminator
  if (offset < data.size()) {
      offset++;
  }
  
  return result;
}

std::string BinaryUtils::readLengthPrefixedString(const std::vector<uint8_t>& data, size_t& offset, bool bigEndian) {
  uint32_t length = readUint32(data, offset, bigEndian);
  offset += 4;
  
  std::string result = readString(data, offset, length);
  offset += length;
  
  return result;
}

bool BinaryUtils::isMachineBigEndian() {
  union {
      uint32_t i;
      char c[4];
  } bint = {0x01020304};
  
  return bint.c[0] == 1;
}

uint16_t BinaryUtils::swapEndian16(uint16_t value) {
  return ((value & 0x00FF) << 8) |
          ((value & 0xFF00) >> 8);
}

uint32_t BinaryUtils::swapEndian32(uint32_t value) {
  return ((value & 0x000000FF) << 24) |
          ((value & 0x0000FF00) << 8) |
          ((value & 0x00FF0000) >> 8) |
          ((value & 0xFF000000) >> 24);
}

uint64_t BinaryUtils::swapEndian64(uint64_t value) {
  return ((value & 0x00000000000000FF) << 56) |
          ((value & 0x000000000000FF00) << 40) |
          ((value & 0x0000000000FF0000) << 24) |
          ((value & 0x00000000FF000000) << 8) |
          ((value & 0x000000FF00000000) >> 8) |
          ((value & 0x0000FF0000000000) >> 24) |
          ((value & 0x00FF000000000000) >> 40) |
          ((value & 0xFF00000000000000) >> 56);
}

float BinaryUtils::swapEndianFloat(float value) {
  uint32_t temp;
  std::memcpy(&temp, &value, sizeof(float));
  temp = swapEndian32(temp);
  std::memcpy(&value, &temp, sizeof(float));
  return value;
}

double BinaryUtils::swapEndianDouble(double value) {
  uint64_t temp;
  std::memcpy(&temp, &value, sizeof(double));
  temp = swapEndian64(temp);
  std::memcpy(&value, &temp, sizeof(double));
  return value;
}

} // namespace utils
} // namespace coil