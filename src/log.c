/**
* @file log.c
* @brief Logging functionality implementation for libcoil-dev
*/

#include <coil/log.h>
#include <coil/deps.h>
#include "srcdeps.h"

/**
* @brief Log level names
*/
static const char *coil_level_names[] = {
  "INFO",     // COIL_LEVEL_INFO
  "WARNING",  // COIL_LEVEL_WARNING
  "ERROR",    // COIL_LEVEL_ERROR
  "FATAL",    // COIL_LEVEL_FATAL
};

/**
* @brief Current log level threshold
*/
static coil_error_level_t coil_current_log_level = COIL_LEVEL_INFO;

/**
* @brief Set the logging level threshold
*
* @param level Minimum level to log
*/
void coil_log_set_level(coil_error_level_t level) {
  if (level <= COIL_LEVEL_FATAL) {
    coil_current_log_level = level;
  }
}

/**
* @brief Log a message with a specified level
*
* @param level Message level
* @param format Printf-style format string
* @param ... Format arguments
*/
void coil_log(coil_error_level_t level, const char *format, ...) {
  if (level < coil_current_log_level) {
    return;
  }

  FILE *output = (level >= COIL_LEVEL_ERROR) ? stderr : stdout;
  
  // Print timestamp
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
  
  // Print log level
  const char *level_name = (level <= COIL_LEVEL_FATAL) 
                          ? coil_level_names[level] 
                          : "UNKNOWN";
  
  fprintf(output, "[%s] [%s] ", timestamp, level_name);
  
  // Print message
  va_list args;
  va_start(args, format);
  vfprintf(output, format, args);
  va_end(args);
  
  fprintf(output, "\n");
  
  // For fatal errors, we exit the program
  if (level == COIL_LEVEL_FATAL) {
    exit(EXIT_FAILURE);
  }
}