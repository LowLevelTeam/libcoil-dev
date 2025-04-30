/**
* @file srcdeps.h
* @brief Common dependencies for source files
*
* @author Low Level Team
*/

#ifndef __COIL_SRCDEPS_H
#define __COIL_SRCDEPS_H

#include <coil/base.h>

/**
* @brief Set the last error code and return it
*
* @param code Error code to set
* @param msg Error message (optional, used in debug builds)
*/
static inline coil_err_t COIL_ERROR(coil_err_t code, const char *msg) {
  extern void coil_error_set(coil_err_t code);
  
  coil_error_set(code);
  
#ifdef DEBUG
  if (msg != NULL) {
    coil_log(COIL_LEVEL_ERROR, "Error: %s - %s", coil_strerr(code), msg);
  }
#else
  (void)msg; // Avoid unused parameter warning
#endif

  return code;
}

#endif // __COIL_SRCDEPS_H