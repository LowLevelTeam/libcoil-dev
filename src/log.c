/* src/log.c */
#include "coil/log.h"
#include <stdlib.h>
#include <string.h>

/* Global default logger */
coil_logger_t *coil_default_logger = NULL;

/* ANSI color codes */
static const char *level_colors[] = {
  "\x1b[90m",  /* TRACE: Bright Black */
  "\x1b[36m",  /* DEBUG: Cyan */
  "\x1b[32m",  /* INFO: Green */
  "\x1b[33m",  /* WARNING: Yellow */
  "\x1b[31m",  /* ERROR: Red */
  "\x1b[35m",  /* FATAL: Magenta */
  ""           /* NONE: No color */
};

static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"
};

static const char *reset_color = "\x1b[0m";

int coil_logger_init(coil_logger_t *logger, const char *prefix, FILE *stream, coil_log_level_t level) {
  if (!logger || !stream) return -1;
  
  logger->stream = stream;
  logger->level = level;
  logger->colored_output = isatty(fileno(stream));
  
  if (pthread_mutex_init(&logger->lock, NULL) != 0) {
    return -1;
  }
  
  logger->initialized = true;
  
  if (prefix) {
    strncpy(logger->prefix, prefix, sizeof(logger->prefix) - 1);
    logger->prefix[sizeof(logger->prefix) - 1] = '\0';
  } else {
    logger->prefix[0] = '\0';
  }
  
  return 0;
}

coil_logger_t *coil_logger_create(const char *prefix, FILE *stream, coil_log_level_t level) {
  coil_logger_t *logger = (coil_logger_t *)malloc(sizeof(coil_logger_t));
  if (!logger) return NULL;
  
  if (coil_logger_init(logger, prefix, stream, level) != 0) {
    free(logger);
    return NULL;
  }
  
  return logger;
}

void coil_logger_set_level(coil_logger_t *logger, coil_log_level_t level) {
  if (!logger || !logger->initialized) return;
  
  pthread_mutex_lock(&logger->lock);
  logger->level = level;
  pthread_mutex_unlock(&logger->lock);
}

void coil_logger_set_colored_output(coil_logger_t *logger, bool enabled) {
  if (!logger || !logger->initialized) return;
  
  pthread_mutex_lock(&logger->lock);
  logger->colored_output = enabled;
  pthread_mutex_unlock(&logger->lock);
}

void coil_log(coil_logger_t *logger, coil_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...) {
  if (!logger || !logger->initialized || level < logger->level) return;
  
  char timestamp[32];
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
  
  pthread_mutex_lock(&logger->lock);
  
  /* Print timestamp, level, and thread id */
  if (logger->colored_output) {
    fprintf(logger->stream, "%s [%s%s%s] [%s] [%lu] ", 
            timestamp, 
            level_colors[level], level_names[level], reset_color,
            logger->prefix[0] ? logger->prefix : "COIL",
            (unsigned long)pthread_self());
  } else {
    fprintf(logger->stream, "%s [%s] [%s] [%lu] ", 
            timestamp, 
            level_names[level],
            logger->prefix[0] ? logger->prefix : "COIL",
            (unsigned long)pthread_self());
  }
  
  /* Print source location for debug and higher */
  if (level >= COIL_LOG_DEBUG) {
      fprintf(logger->stream, "(%s:%d:%s) ", file, line, func);
  }
  
  /* Print the actual message */
  va_list args;
  va_start(args, fmt);
  vfprintf(logger->stream, fmt, args);
  va_end(args);
  
  fprintf(logger->stream, "\n");
  fflush(logger->stream);
  
  pthread_mutex_unlock(&logger->lock);
}

void coil_logger_cleanup(coil_logger_t *logger) {
  if (!logger || !logger->initialized) return;
  
  pthread_mutex_destroy(&logger->lock);
  logger->initialized = false;
}

void coil_logger_destroy(coil_logger_t *logger) {
  if (!logger) return;
  
  coil_logger_cleanup(logger);
  free(logger);
}

void coil_log_init(void) {
  if (!coil_default_logger) {
    coil_default_logger = coil_logger_create("COIL", stdout, COIL_LOG_INFO);
  }
}

void coil_log_cleanup(void) {
  if (coil_default_logger) {
    coil_logger_destroy(coil_default_logger);
    coil_default_logger = NULL;
  }
}