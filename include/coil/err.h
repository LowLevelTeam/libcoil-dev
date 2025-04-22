/**
* @file err.h
* @brief Error handling and reporting for COIL
* 
* This file defines the error handling system for the COIL library,
* including error codes, severity levels, and reporting mechanisms.
*/

#ifndef __COIL_INCLUDE_GUARD_ERR_H
#define __COIL_INCLUDE_GUARD_ERR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief COIL Error Codes
*/
typedef enum coil_err_e {
  COIL_ERR_GOOD = 0,       ///< No error
  COIL_ERR_NOMEM = 1,      ///< Memory allocation failure
  COIL_ERR_INVAL = 2,      ///< Invalid argument
  COIL_ERR_IO = 3,         ///< I/O error
  COIL_ERR_FORMAT = 4,     ///< Format error
  COIL_ERR_NOTFOUND = 5,   ///< Not found
  COIL_ERR_NOTSUP = 6,     ///< Not supported
  COIL_ERR_BADSTATE = 7,   ///< Bad state
  COIL_ERR_EXISTS = 8,     ///< Already exists
  COIL_ERR_UNKNOWN = 9,    ///< Unknown error
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

/**
* @brief Error position information
* 
* Contains source location information for detailed error reporting
*/
typedef struct coil_error_position_s {
  const char* file;  ///< Source file
  size_t line;       ///< Line number
  size_t index;      ///< Byte position in file
} coil_error_position_t;

/**
* @brief Error context
* 
* Complete information about an error, including code, message, and position
*/
typedef struct coil_error_context_s {
  coil_err_t code;                  ///< Error code
  coil_error_level_t level;         ///< Error level
  char message[256];                ///< Error message
  coil_error_position_t position;   ///< Error position
} coil_error_context_t;

/**
* @brief Error callback function type
* 
* @param level Error severity level
* @param message Error message
* @param position Error position information (can be NULL)
* @param user_data User data pointer passed to callback
*/
typedef void (*coil_error_callback_t)(
  coil_error_level_t level,
  const char* message,
  const coil_error_position_t* position,
  void* user_data
);

/**
* @brief Initialize the error system
* 
* @return coil_err_t COIL_ERR_GOOD on success
*/
coil_err_t coil_error_init(void);

/**
* @brief Shutdown the error system
* 
* Frees any resources allocated by the error system
*/
void coil_error_shutdown(void);

/**
* @brief Set error callback function
* 
* @param callback Callback function to handle errors
* @param user_data User data to pass to the callback
*/
void coil_error_set_callback(coil_error_callback_t callback, void* user_data);

/**
* @brief Log an error message
* 
* @param level Error severity level
* @param code Error code
* @param message Error message
* @param file Source file where error occurred
* @param line Line number where error occurred
* @param index Byte position where error occurred (0 if not applicable)
* @return coil_err_t The error code passed in
*/
coil_err_t coil_error_report(
  coil_error_level_t level,
  coil_err_t code,
  const char* message,
  const char* file,
  size_t line,
  size_t index
);

/**
* @brief Set detailed error information
* 
* @param code Error code
* @param level Error severity level
* @param message Detailed error message
* @param file Source file where the error occurred
* @param line Line number where the error occurred
* @param index Position in the file or stream (if applicable)
* @return coil_err_t The error code (for convenience in return statements)
*/
coil_err_t coil_error_set_detailed(
  coil_err_t code,
  coil_error_level_t level,
  const char* message,
  const char* file,
  size_t line,
  size_t index
);

/**
* @brief Get the last error context
* 
* @return const coil_error_context_t* Pointer to the last error context
*/
const coil_error_context_t* coil_error_get_last(void);

/**
* @brief Clear the last error
*/
void coil_error_clear(void);

/**
* @brief Get string representation of an error code
* 
* @param code Error code
* @return const char* String representation
*/
const char* coil_error_string(coil_err_t code);

/**
* @brief Helper macros for error reporting
*/
#define COIL_CURRENT_POS __FILE__, __LINE__, 0

#define COIL_INFO(code, message) \
  coil_error_report(COIL_LEVEL_INFO, code, message, COIL_CURRENT_POS)

#define COIL_WARNING(code, message) \
  coil_error_report(COIL_LEVEL_WARNING, code, message, COIL_CURRENT_POS)

#define COIL_ERROR(code, message) \
  coil_error_report(COIL_LEVEL_ERROR, code, message, COIL_CURRENT_POS)

#define COIL_FATAL(code, message) \
  coil_error_report(COIL_LEVEL_FATAL, code, message, COIL_CURRENT_POS)

/**
* @brief Simple macros for returning error codes with context
*/
#define COIL_RETURN_ERROR(code, message) \
  return coil_error_set_detailed(code, COIL_LEVEL_ERROR, message, __FILE__, __LINE__, 0)

/**
* @brief Check if a pointer is NULL and return an error if it is
*/
#define COIL_CHECK_NULL(ptr, message) \
  if ((ptr) == NULL) { COIL_RETURN_ERROR(COIL_ERR_INVAL, message); }

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_ERR_H */