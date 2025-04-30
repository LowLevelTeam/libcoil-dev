/**
* @file log.h
* @brief Logging functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_LOG_H
#define __COIL_INCLUDE_GUARD_LOG_H

#include <coil/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Set the logging level threshold
*
* @param level Minimum level to log
*/
void coil_log_set_level(coil_error_level_t level);

/**
* @brief Log a message with a specified level
*
* @param level Message level
* @param format Printf-style format string
* @param ... Format arguments
*/
void coil_log(coil_error_level_t level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_LOG_H