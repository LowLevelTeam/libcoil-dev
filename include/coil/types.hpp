/**
* @file types.hpp
* @brief Common type definitions for COIL
*/

#pragma once
#include <cstdint>
#include <limits>
#include <cstddef>

namespace coil {

/**
* @brief Basic numeric types used throughout COIL
*/
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

/**
* @brief Result codes for operations
*/
enum class Result {
  Success,      ///< Operation succeeded
  InvalidArg,   ///< Invalid argument
  OutOfMemory,  ///< Memory allocation failed
  IoError,      ///< I/O error occurred
  InvalidFormat,///< Format error
  NotFound,     ///< Item not found
  NotSupported, ///< Operation not supported
  BadState,     ///< Object in bad state
  AlreadyExists ///< Item already exists
};

/**
* @brief Constant limits for various types
*/
namespace limits {
  constexpr u8 U8_MAX = std::numeric_limits<u8>::max();
  constexpr u16 U16_MAX = std::numeric_limits<u16>::max();
  constexpr u32 U32_MAX = std::numeric_limits<u32>::max();
  constexpr u64 U64_MAX = std::numeric_limits<u64>::max();
  
  constexpr i8 I8_MIN = std::numeric_limits<i8>::min();
  constexpr i8 I8_MAX = std::numeric_limits<i8>::max();
  constexpr i16 I16_MIN = std::numeric_limits<i16>::min();
  constexpr i16 I16_MAX = std::numeric_limits<i16>::max();
  constexpr i32 I32_MIN = std::numeric_limits<i32>::min();
  constexpr i32 I32_MAX = std::numeric_limits<i32>::max();
  constexpr i64 I64_MIN = std::numeric_limits<i64>::min();
  constexpr i64 I64_MAX = std::numeric_limits<i64>::max();
}

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