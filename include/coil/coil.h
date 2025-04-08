#ifndef COIL_H
#define COIL_H

/**
 * @file coil.h
 * @brief Main header for the optimized COIL library.
 * 
 * This header includes the core components for stream parsing,
 * focusing on zero-cost abstractions.
 */

#include <string>
#include <string_view>

namespace coil {

/**
 * @brief Library version information
 */
struct Version {
    int major;           ///< Major version
    int minor;           ///< Minor version
    int patch;           ///< Patch version
    std::string string;  ///< Version as string
};

/**
 * @brief Get the library version
 * 
 * @return Version structure with version information
 */
inline Version getVersion();

} // namespace coil

#endif // COIL_H