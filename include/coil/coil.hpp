/**
 * @file coil.hpp
 * @brief Main header for the COIL library
 */

#pragma once
#include "coil/types.hpp"

namespace coil {

/**
 * @brief Library version information
 */
struct Version {
    int major;           ///< Major version
    int minor;           ///< Minor version
    int patch;           ///< Patch version
    const char* string;  ///< Version as string
};

/**
 * @brief Get the library version
 */
Version getVersion();

} // namespace coil