/**
 * @file coil.h
 * @brief Main header for the COIL library
 */

#ifndef __COIL_INCLUDE_GUARD_COIL_H
#define __COIL_INCLUDE_GUARD_COIL_H

#include <ccoil/err.h>
#include <ccoil/arena.h>
#include <ccoil/instr.h>
#include <ccoil/obj.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Library version information
 */
typedef struct coil_version_s {
  int major;                // Major version
  int minor;                // Minor version
  int patch;                // Patch version
  const char* string;       // Version as string
  const char* build;        // Build information
} coil_version_t;

/**
 * @brief Library configuration information
 */
typedef struct coil_configuration_s {
  int debug_enabled;        // Whether debug is enabled
  int asserts_enabled;      // Whether asserts are enabled
} coil_configuration_t;

/**
 * @brief Initialize the COIL library
 * Call this before using any other functions
 * 
 * @return coil_err_t Error code
 */
coil_err_t coil_initialize(void);

/**
 * @brief Shutdown the COIL library
 * Call this when done using the library
 */
void coil_shutdown(void);

/**
 * @brief Get the library version
 * 
 * @param version Pointer to store version information
 * @return coil_err_t Error code
 */
coil_err_t coil_get_version(coil_version_t* version);

/**
 * @brief Get the library configuration
 * 
 * @param config Pointer to store configuration information
 * @return coil_err_t Error code
 */
coil_err_t coil_get_configuration(coil_configuration_t* config);

/**
 * @brief Check if library is initialized
 * 
 * @return int Non-zero if initialized
 */
int coil_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_COIL_H */