/**
* @file err.h
* @brief Error management functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_ERR_H
#define __COIL_INCLUDE_GUARD_ERR_H

#include <coil/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief COIL Toolchain Error Codes
*/
typedef enum coil_err_e {
  COIL_ERR_GOOD,       ///< No error
  COIL_ERR_NOMEM,      ///< Memory allocation failure
  COIL_ERR_INVAL,      ///< Invalid argument
  COIL_ERR_IO,         ///< I/O error
  COIL_ERR_FORMAT,     ///< Format error
  COIL_ERR_NOTFOUND,   ///< Not found
  COIL_ERR_NOTSUP,     ///< Not supported
  COIL_ERR_BADSTATE,   ///< Bad state
  COIL_ERR_EXISTS,     ///< Already exists
  COIL_ERR_UNKNOWN,    ///< Unknown error
} coil_err_t;

/**
* @brief Error severity levels
*/
typedef enum coil_error_level_e {
  COIL_LEVEL_INFO = 0,     ///< Informational message
  COIL_LEVEL_WARNING = 1,  ///< Warning message
  COIL_LEVEL_ERROR = 2,    ///< Error message
  COIL_LEVEL_FATAL = 3,    ///< Fatal error message
} coil_error_level_t;

const char *coil_strerr(coil_err_t code);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_ERR_H