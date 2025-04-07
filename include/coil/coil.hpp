#pragma once

/**
 * @file coil.hpp
 * @brief Main header for the COIL library.
 * 
 * This file includes all the components of the COIL library and
 * provides initialization and cleanup functions for the library as a whole.
 */

#include "coil/log.hpp"
#include "coil/err.hpp"
#include "coil/mem.hpp"
#include "coil/args.hpp"
#include "coil/stream.hpp"
#include "coil/thread.hpp"

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
Version getVersion();

/**
 * @brief Initialize the COIL library.
 * 
 * This function initializes all subsystems of the library:
 * - Logging system
 * - Error handling system
 * - Memory management system
 * - Stream handling system
 * - Thread system
 * 
 * @return true on success, false on failure
 */
bool initialize();

/**
 * @brief Clean up the COIL library.
 * 
 * This function cleans up all subsystems of the library:
 * - Thread system
 * - Stream handling system
 * - Memory management system
 * - Error handling system
 * - Logging system
 */
void cleanup();

/**
 * @brief Get the last error from the default error manager.
 * 
 * @return The last error message, or empty string if no error
 */
std::string getLastError();

/**
 * @brief Set the log level for the default logger.
 * 
 * @param level The log level to set
 */
void setLogLevel(LogLevel level);

/**
 * @brief Set the error handler for the default error manager.
 * 
 * @param handler The error handler function
 * @param userData User data to pass to the handler
 */
void setErrorHandler(ErrorHandlerFunction handler, void* userData);

} // namespace coil