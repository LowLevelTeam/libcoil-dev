/* coil/coil.h */
#ifndef COIL_H
#define COIL_H

/**
 * @file coil.h
 * @brief Main header for the libcoil-dev library.
 * 
 * This file includes all the components of the libcoil-dev library and
 * provides initialization and cleanup functions for the library as a whole.
 */

#include "coil_log.h"
#include "coil_error.h"
#include "coil_stream.h"
#include "coil_memory.h"
#include "coil_args.h"
#include "coil_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Library version information */
typedef struct {
    int major;
    int minor;
    int patch;
    const char *version_string;
} coil_version_t;

/** Get the library version */
coil_version_t coil_get_version(void);

/**
 * Initialize the libcoil-dev library.
 * 
 * This function initializes all subsystems of the library:
 * - Logging system
 * - Error handling system
 * - Memory management system
 * - Stream handling system
 * - Thread system
 * 
 * @return 0 on success, non-zero on failure
 */
int coil_init(void);

/**
 * Clean up the libcoil-dev library.
 * 
 * This function cleans up all subsystems of the library:
 * - Thread system
 * - Stream handling system
 * - Memory management system
 * - Error handling system
 * - Logging system
 */
void coil_cleanup(void);

/**
 * Get the last error from the default error manager.
 * 
 * @return The last error message, or NULL if no error
 */
const char *coil_get_last_error(void);

/**
 * Set the log level for the default logger.
 * 
 * @param level The log level to set
 */
void coil_set_log_level(coil_log_level_t level);

/**
 * Set the error handler for the default error manager.
 * 
 * @param handler The error handler function
 * @param user_data User data to pass to the handler
 */
typedef void (*coil_error_handler_fn)(coil_error_code_t code, 
                                     coil_error_severity_t severity,
                                     const coil_stream_pos_t *position,
                                     const char *message, 
                                     void *user_data);

void coil_set_error_handler(coil_error_handler_fn handler, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* COIL_H */