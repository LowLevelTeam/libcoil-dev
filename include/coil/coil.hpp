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
    const char* build;   ///< Build information
};

/**
 * @brief Get the library version
 */
Version getVersion();

/**
 * @brief Library configuration information
 */
struct Configuration {
    bool debug_enabled;     ///< Whether debug is enabled
    bool asserts_enabled;   ///< Whether asserts are enabled
};

/**
 * @brief Get the library configuration
 */
Configuration getConfiguration();

/**
 * @brief Initialize the library
 * Call this before using any other functions
 * @return Result of initialization
 */
Result initialize();

/**
 * @brief Shutdown the library
 * Call this when done using the library
 */
void shutdown();

} // namespace coil