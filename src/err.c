/**
* @file err.c
* @brief Error management implementation for libcoil-dev
*/

#include <coil/err.h>

/**
* @brief Error message strings corresponding to coil_err_t values
*/
static const char *coil_error_strings[] = {
  "No error",                  // COIL_ERR_GOOD
  "Memory allocation failure", // COIL_ERR_NOMEM
  "Invalid argument",          // COIL_ERR_INVAL
  "I/O error",                 // COIL_ERR_IO
  "Format error",              // COIL_ERR_FORMAT
  "Not found",                 // COIL_ERR_NOTFOUND
  "Not supported",             // COIL_ERR_NOTSUP
  "Bad state",                 // COIL_ERR_BADSTATE
  "Already exists",            // COIL_ERR_EXISTS
  "Unknown error",             // COIL_ERR_UNKNOWN
};

/**
* @brief Last error code
*/
static __thread coil_err_t coil_last_error = COIL_ERR_GOOD;

/**
* @brief Get string description for an error code
*
* @param code Error code
* @return const char* String description
*/
const char *coil_strerr(coil_err_t code) {
  if (code >= COIL_ERR_GOOD && code <= COIL_ERR_UNKNOWN) {
    return coil_error_strings[code];
  }
  return "Invalid error code";
}

/**
* @brief Set the last error code
*
* @param code Error code to set
*/
void coil_error_set(coil_err_t code) {
  coil_last_error = code;
}

/**
* @brief Get the last error code
*
* @return coil_err_t Last error code
*/
coil_err_t coil_error_get_last(void) {
  return coil_last_error;
}