/**
 * @file err.c
 * @brief Implementation of error handling and reporting for COIL
 */

#include <coil/err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Global error context
 */
static coil_error_context_t g_last_error;

/**
 * @brief Error callback and user data
 */
static coil_error_callback_t g_error_callback = NULL;
static void* g_error_callback_data = NULL;

/**
 * @brief Default error handler
 */
static void default_error_handler(
  coil_error_level_t level,
  const char* message,
  const coil_error_position_t* position,
  void* user_data
) {
  (void)user_data; // Unused parameter
  
  const char* level_str = "Unknown";
  switch (level) {
    case COIL_LEVEL_INFO:
      level_str = "Info";
      break;
    case COIL_LEVEL_WARNING:
      level_str = "Warning";
      break;
    case COIL_LEVEL_ERROR:
      level_str = "Error";
      break;
    case COIL_LEVEL_FATAL:
      level_str = "Fatal";
      break;
  }
  
  if (position && position->file) {
    if (position->line > 0) {
      fprintf(stderr, "COIL %s: %s:%zu: %s\n", 
              level_str, position->file, position->line, message);
    } else if (position->index > 0) {
      fprintf(stderr, "COIL %s: %s:@%zu: %s\n", 
              level_str, position->file, position->index, message);
    } else {
      fprintf(stderr, "COIL %s: %s: %s\n", 
              level_str, position->file, message);
    }
  } else {
    fprintf(stderr, "COIL %s: %s\n", level_str, message);
  }
  
  // Abort on fatal errors
  if (level == COIL_LEVEL_FATAL) {
    fprintf(stderr, "Fatal error: aborting\n");
    abort();
  }
}

coil_err_t coil_error_init(void) {
  // Initialize global error context
  memset(&g_last_error, 0, sizeof(g_last_error));
  
  // Set default error handler
  g_error_callback = default_error_handler;
  g_error_callback_data = NULL;
  
  return COIL_ERR_GOOD;
}

void coil_error_shutdown(void) {
  // Clear any pending errors
  coil_error_clear();
  
  // Reset callback
  g_error_callback = NULL;
  g_error_callback_data = NULL;
}

void coil_error_set_callback(coil_error_callback_t callback, void* user_data) {
  if (callback) {
    g_error_callback = callback;
    g_error_callback_data = user_data;
  } else {
    // Reset to default if NULL
    g_error_callback = default_error_handler;
    g_error_callback_data = NULL;
  }
}

coil_err_t coil_error_report(
  coil_error_level_t level,
  coil_err_t code,
  const char* message,
  const char* file,
  size_t line,
  size_t index
) {
  // Store in last error context
  g_last_error.code = code;
  g_last_error.level = level;
  
  // Copy message with truncation if needed
  if (message) {
    strncpy(g_last_error.message, message, sizeof(g_last_error.message) - 1);
    g_last_error.message[sizeof(g_last_error.message) - 1] = '\0';
  } else {
    g_last_error.message[0] = '\0';
  }
  
  // Store position information
  g_last_error.position.file = file;
  g_last_error.position.line = line;
  g_last_error.position.index = index;
  
  // Call error handler if set
  if (g_error_callback) {
    g_error_callback(level, message, &g_last_error.position, g_error_callback_data);
  }
  
  return code;
}

const coil_error_context_t* coil_error_get_last(void) {
  return &g_last_error;
}

void coil_error_clear(void) {
  memset(&g_last_error, 0, sizeof(g_last_error));
}

const char* coil_error_string(coil_err_t code) {
  switch (code) {
    case COIL_ERR_GOOD:
      return "No error";
    case COIL_ERR_NOMEM:
      return "Memory allocation failure";
    case COIL_ERR_INVAL:
      return "Invalid argument";
    case COIL_ERR_IO:
      return "I/O error";
    case COIL_ERR_FORMAT:
      return "Format error";
    case COIL_ERR_NOTFOUND:
      return "Not found";
    case COIL_ERR_NOTSUP:
      return "Not supported";
    case COIL_ERR_BADSTATE:
      return "Bad state";
    case COIL_ERR_EXISTS:
      return "Already exists";
    case COIL_ERR_UNKNOWN:
    default:
      return "Unknown error";
  }
}