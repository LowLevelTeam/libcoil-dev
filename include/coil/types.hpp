/**
 * @file types.hpp
 * @brief Common type definitions for COIL
 */

#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <exception>
#include <stdexcept>

namespace coil {

/**
 * @brief Basic numeric types used throughout COIL
 */
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

/**
 * @brief COIL base exception class
 */
class CoilException : public std::runtime_error {
public:
  explicit CoilException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Invalid argument exception
 */
class InvalidArgException : public CoilException {
public:
  explicit InvalidArgException(const std::string& message) 
    : CoilException("Invalid argument: " + message) {}
};

/**
 * @brief Out of memory exception
 */
class OutOfMemoryException : public CoilException {
public:
  explicit OutOfMemoryException(const std::string& message) 
    : CoilException("Out of memory: " + message) {}
};

/**
 * @brief I/O operation exception
 */
class IOException : public CoilException {
public:
  explicit IOException(const std::string& message) 
    : CoilException("I/O error: " + message) {}
};

/**
 * @brief Format error exception
 */
class FormatException : public CoilException {
public:
  explicit FormatException(const std::string& message) 
    : CoilException("Format error: " + message) {}
};

/**
 * @brief Not found exception
 */
class NotFoundException : public CoilException {
public:
  explicit NotFoundException(const std::string& message) 
    : CoilException("Not found: " + message) {}
};

/**
 * @brief Not supported exception
 */
class NotSupportedException : public CoilException {
public:
  explicit NotSupportedException(const std::string& message) 
    : CoilException("Not supported: " + message) {}
};

/**
 * @brief Bad state exception
 */
class BadStateException : public CoilException {
public:
  explicit BadStateException(const std::string& message) 
    : CoilException("Bad state: " + message) {}
};

/**
 * @brief Already exists exception
 */
class AlreadyExistsException : public CoilException {
public:
  explicit AlreadyExistsException(const std::string& message) 
    : CoilException("Already exists: " + message) {}
};

/**
 * @brief Helper functions for alignment
 */
template<typename T>
inline T alignUp(T value, T alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T>
inline T alignDown(T value, T alignment) {
  return value & ~(alignment - 1);
}

template<typename T>
inline bool isAligned(T value, T alignment) {
  return (value & (alignment - 1)) == 0;
}

/**
 * @brief Helper to check if a value is a power of 2
 */
template<typename T>
inline bool isPowerOfTwo(T value) {
  return value && !(value & (value - 1));
}

/**
 * @brief Helper to get the next power of 2
 */
template<typename T>
inline T nextPowerOfTwo(T value) {
  if (value == 0) return 1;
  value--;
  for (size_t i = 1; i < sizeof(T) * 8; i *= 2)
    value |= value >> i;
  return value + 1;
}

} // namespace coil