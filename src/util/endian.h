#ifndef COIL_UTIL_ENDIAN_H
#define COIL_UTIL_ENDIAN_H

#include <cstdint>
#include <vector>

namespace coil {
namespace util {

/**
 * @brief Determines if the host machine is little-endian
 * 
 * @return true if host is little-endian, false if big-endian
 */
bool isHostLittleEndian();

/**
 * @brief Swaps the byte order of a 16-bit value
 * 
 * @param value The value to swap
 * @return The byte-swapped value
 */
uint16_t swapBytes(uint16_t value);

/**
 * @brief Swaps the byte order of a 32-bit value
 * 
 * @param value The value to swap
 * @return The byte-swapped value
 */
uint32_t swapBytes(uint32_t value);

/**
 * @brief Swaps the byte order of a 64-bit value
 * 
 * @param value The value to swap
 * @return The byte-swapped value
 */
uint64_t swapBytes(uint64_t value);

/**
 * @brief Converts a 16-bit value to target endianness
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return The converted value
 */
uint16_t convertEndian(uint16_t value, bool targetLittleEndian);

/**
 * @brief Converts a 32-bit value to target endianness
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return The converted value
 */
uint32_t convertEndian(uint32_t value, bool targetLittleEndian);

/**
 * @brief Converts a 64-bit value to target endianness
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return The converted value
 */
uint64_t convertEndian(uint64_t value, bool targetLittleEndian);

/**
 * @brief Converts a float value to target endianness
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return The converted value
 */
float convertEndian(float value, bool targetLittleEndian);

/**
 * @brief Converts a double value to target endianness
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return The converted value
 */
double convertEndian(double value, bool targetLittleEndian);

/**
 * @brief Converts a 16-bit value to target endianness in-place
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianInPlace(uint16_t& value, bool targetLittleEndian);

/**
 * @brief Converts a 32-bit value to target endianness in-place
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianInPlace(uint32_t& value, bool targetLittleEndian);

/**
 * @brief Converts a 64-bit value to target endianness in-place
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianInPlace(uint64_t& value, bool targetLittleEndian);

/**
 * @brief Converts a float value to target endianness in-place
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianInPlace(float& value, bool targetLittleEndian);

/**
 * @brief Converts a double value to target endianness in-place
 * 
 * @param value The value to convert
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianInPlace(double& value, bool targetLittleEndian);

/**
 * @brief Converts a buffer of elements to target endianness in-place
 * 
 * @param buffer Pointer to the buffer
 * @param elementSize Size of each element in bytes
 * @param elementCount Number of elements in the buffer
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 */
void convertEndianBuffer(void* buffer, size_t elementSize, size_t elementCount, bool targetLittleEndian);

/**
 * @brief Converts a vector of bytes to target endianness
 * 
 * @param data Vector of bytes to convert
 * @param elementSize Size of each element in bytes
 * @param targetLittleEndian true if target is little-endian, false if big-endian
 * @return Converted vector
 */
std::vector<uint8_t> convertEndianVector(const std::vector<uint8_t>& data, 
                                        size_t elementSize, 
                                        bool targetLittleEndian);

} // namespace util
} // namespace coil

#endif // COIL_UTIL_ENDIAN_H
