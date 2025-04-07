/* coil/log.h */
#ifndef COIL_LOG_H
#define COIL_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
  COIL_LOG_TRACE = 0,
  COIL_LOG_DEBUG,
  COIL_LOG_INFO,
  COIL_LOG_WARNING,
  COIL_LOG_ERROR,
  COIL_LOG_FATAL,
  COIL_LOG_NONE
} coil_log_level_t;

typedef struct coil_logger {
  FILE *stream;                 /* Output stream (stdout, file, etc.) */
  coil_log_level_t level;       /* Current log level */
  bool colored_output;          /* Whether to use ANSI colors in output */
  pthread_mutex_t lock;         /* Mutex for thread-safety */
  bool initialized;             /* Whether the logger is initialized */
  char prefix[64];              /* Logger name/prefix */
} coil_logger_t;

/* Global default logger */
extern coil_logger_t *coil_default_logger;

/* Initialize a logger with a specific stream */
int coil_logger_init(coil_logger_t *logger, const char *prefix, FILE *stream, coil_log_level_t level);

/* Create a new logger with a specific stream */
coil_logger_t *coil_logger_create(const char *prefix, FILE *stream, coil_log_level_t level);

/* Set the log level */
void coil_logger_set_level(coil_logger_t *logger, coil_log_level_t level);

/* Enable or disable colored output */
void coil_logger_set_colored_output(coil_logger_t *logger, bool enabled);

/* Log a message with the given level */
void coil_log(coil_logger_t *logger, coil_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

/* Convenience macros for different log levels */
#define COIL_TRACE(logger, ...) \
  coil_log(logger, COIL_LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_DEBUG(logger, ...) \
  coil_log(logger, COIL_LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_INFO(logger, ...) \
  coil_log(logger, COIL_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_WARNING(logger, ...) \
  coil_log(logger, COIL_LOG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_ERROR(logger, ...) \
  coil_log(logger, COIL_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define COIL_FATAL(logger, ...) \
  coil_log(logger, COIL_LOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

/* Default logger convenience macros */
#define COIL_DEFAULT_TRACE(...) COIL_TRACE(coil_default_logger, __VA_ARGS__)
#define COIL_DEFAULT_DEBUG(...) COIL_DEBUG(coil_default_logger, __VA_ARGS__)
#define COIL_DEFAULT_INFO(...) COIL_INFO(coil_default_logger, __VA_ARGS__)
#define COIL_DEFAULT_WARNING(...) COIL_WARNING(coil_default_logger, __VA_ARGS__)
#define COIL_DEFAULT_ERROR(...) COIL_ERROR(coil_default_logger, __VA_ARGS__)
#define COIL_DEFAULT_FATAL(...) COIL_FATAL(coil_default_logger, __VA_ARGS__)

/* Cleanup */
void coil_logger_destroy(coil_logger_t *logger);
void coil_logger_cleanup(coil_logger_t *logger);

/* Initialize library logging */
void coil_log_init(void);

/* Cleanup library logging */
void coil_log_cleanup(void);

#endif /* COIL_LOG_H */