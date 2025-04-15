/**
 * @file coil.hpp
 * @brief Main header for the COIL development library.
 * 
 * This header includes the core components for versioning.
 * focusing on zero-cost abstractions.
 */

#pragma once
#include <string>

namespace coil {

/**
 * @brief Library version information
 */
struct Version {
  int major;           ///< Major version
  int minor;           ///< Minor version
  int patch;           ///< Patch version
  const char* string;  ///< Version as string (static lifetime)
};

/**
 * @brief Get the library version
 * 
 * @return Version structure with version information
 */
Version getVersion();

} // namespace coil