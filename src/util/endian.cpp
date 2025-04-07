#include "endian.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace coil {
namespace util {

namespace {
    // Runtime detection of host endianness
    bool isLittleEndian() {
        static const uint16_t test = 0x0001;
        return *reinterpret_cast<const uint8_t*>(&test) == 0x01;
    }
}

bool isHostLittleEndian() {
    return isLittleEndian();
}

// Swap bytes for different integral types
uint16_t swapBytes(uint16_t value) {
    return (value << 8) | (value >> 8);
}

uint32_t swapBytes(uint32_t value) {
    return ((value & 0xFF) << 24) |
           ((value & 0xFF00) << 8) |
           ((value & 0xFF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
}

uint64_t swapBytes(uint64_t value) {
    return ((value & 0xFF) << 56) |
           ((value & 0xFF00) << 40) |
           ((value & 0xFF0000) << 24) |
           ((value & 0xFF000000) << 8) |
           ((value & 0xFF00000000) >> 8) |
           ((value & 0xFF0000000000) >> 24) |
           ((value & 0xFF000000000000) >> 40) |
           ((value & 0xFF00000000000000) >> 56);
}

// Endian conversion functions
uint16_t convertEndian(uint16_t value, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return value; // No conversion needed
    }
    
    return swapBytes(value);
}

uint32_t convertEndian(uint32_t value, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return value; // No conversion needed
    }
    
    return swapBytes(value);
}

uint64_t convertEndian(uint64_t value, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return value; // No conversion needed
    }
    
    return swapBytes(value);
}

// Convert in-place
void convertEndianInPlace(uint16_t& value, bool targetLittleEndian) {
    value = convertEndian(value, targetLittleEndian);
}

void convertEndianInPlace(uint32_t& value, bool targetLittleEndian) {
    value = convertEndian(value, targetLittleEndian);
}

void convertEndianInPlace(uint64_t& value, bool targetLittleEndian) {
    value = convertEndian(value, targetLittleEndian);
}

// Convert floating-point values
float convertEndian(float value, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return value; // No conversion needed
    }
    
    uint32_t temp;
    std::memcpy(&temp, &value, sizeof(float));
    temp = swapBytes(temp);
    float result;
    std::memcpy(&result, &temp, sizeof(float));
    return result;
}

double convertEndian(double value, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return value; // No conversion needed
    }
    
    uint64_t temp;
    std::memcpy(&temp, &value, sizeof(double));
    temp = swapBytes(temp);
    double result;
    std::memcpy(&result, &temp, sizeof(double));
    return result;
}

// Convert in-place
void convertEndianInPlace(float& value, bool targetLittleEndian) {
    value = convertEndian(value, targetLittleEndian);
}

void convertEndianInPlace(double& value, bool targetLittleEndian) {
    value = convertEndian(value, targetLittleEndian);
}

// Convert a buffer of elements
void convertEndianBuffer(void* buffer, size_t elementSize, size_t elementCount, bool targetLittleEndian) {
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return; // No conversion needed
    }
    
    uint8_t* bytes = static_cast<uint8_t*>(buffer);
    
    switch (elementSize) {
        case 2: // 16-bit values
            for (size_t i = 0; i < elementCount; i++) {
                uint16_t* value = reinterpret_cast<uint16_t*>(bytes + i * elementSize);
                *value = swapBytes(*value);
            }
            break;
            
        case 4: // 32-bit values
            for (size_t i = 0; i < elementCount; i++) {
                uint32_t* value = reinterpret_cast<uint32_t*>(bytes + i * elementSize);
                *value = swapBytes(*value);
            }
            break;
            
        case 8: // 64-bit values
            for (size_t i = 0; i < elementCount; i++) {
                uint64_t* value = reinterpret_cast<uint64_t*>(bytes + i * elementSize);
                *value = swapBytes(*value);
            }
            break;
            
        default: // Arbitrary size elements - swap byte by byte
            for (size_t i = 0; i < elementCount; i++) {
                uint8_t* element = bytes + i * elementSize;
                for (size_t j = 0; j < elementSize / 2; j++) {
                    std::swap(element[j], element[elementSize - 1 - j]);
                }
            }
            break;
    }
}

// Convert std::vector
std::vector<uint8_t> convertEndianVector(const std::vector<uint8_t>& data, 
                                        size_t elementSize, 
                                        bool targetLittleEndian) {
    if (elementSize <= 1) {
        return data; // No conversion needed for byte-sized elements
    }
    
    bool hostLittleEndian = isLittleEndian();
    
    if (hostLittleEndian == targetLittleEndian) {
        return data; // No conversion needed
    }
    
    std::vector<uint8_t> result = data; // Copy the input
    
    size_t elementCount = data.size() / elementSize;
    convertEndianBuffer(result.data(), elementSize, elementCount, targetLittleEndian);
    
    return result;
}

} // namespace util
} // namespace coil
