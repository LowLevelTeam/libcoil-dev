/**
 * @file types.hpp
 * @brief Common type definitions for COIL
 */

#pragma once
#include <cstdint>

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
};

} // namespace coil