/* coil/err.h */
#ifndef COIL_ERROR_H
#define COIL_ERROR_H

#include "coil/log.h"
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

/* Error codes */
typedef enum {
  COIL_ERR_NONE = 0,
  COIL_ERR_MEMORY,           /* Memory allocation failure */
  COIL_ERR_IO,               /* I/O error */
  COIL_ERR_FORMAT,           /* Invalid format */
  COIL_ERR_SYNTAX,           /* Syntax error */
  COIL_ERR_SEMANTIC,         /* Semantic error */
  COIL_ERR_REFERENCE,        /* Invalid reference */
  COIL_ERR_OVERFLOW,         /* Overflow */
  COIL_ERR_UNDERFLOW,        /* Underflow */
  COIL_ERR_BOUNDS,           /* Out of bounds */
  COIL_ERR_STATE,            /* Invalid state */
  COIL_ERR_ARGUMENT,         /* Invalid argument */
  COIL_ERR_INTERNAL,         /* Internal error */
  COIL_ERR_UNSUPPORTED,      /* Unsupported operation */
  COIL_ERR_CUSTOM,           /* Custom error */
  COIL_ERR_MAX               /* Maximum error code */
} coil_error_code_t;

/* Error severity */
typedef enum {
  COIL_SEVERITY_INFO,        /* Informational */
  COIL_SEVERITY_WARNING,     /* Warning */
  COIL_SEVERITY_ERROR,       /* Error */
  COIL_SEVERITY_FATAL        /* Fatal error */
} coil_error_severity_t;

/* Stream position */
typedef struct {
  const char *file_name;     /* Source file name */
  size_t line;               /* Line number */
  size_t column;             /* Column number */
  size_t offset;             /* Byte offset from start */
} coil_stream_pos_t;

/* Error entry */
typedef struct coil_error_entry {
  coil_error_code_t code;          /* Error code */
  coil_error_severity_t severity;  /* Error severity */
  coil_stream_pos_t position;      /* Stream position */
  char message[256];               /* Error message */
  struct coil_error_entry *next;   /* Next error in list */
} coil_error_entry_t;

/* Error manager */
typedef struct {
  coil_error_entry_t *head;        /* Head of error list */
  coil_error_entry_t *tail;        /* Tail of error list */
  size_t count;                    /* Number of errors */
  pthread_mutex_t lock;            /* Mutex for thread-safety */
  coil_logger_t *logger;           /* Logger */
  bool initialized;                /* Whether the manager is initialized */
} coil_error_manager_t;

/* Global default error manager */
extern coil_error_manager_t *coil_default_error_manager;

/* Initialize an error manager */
int coil_error_manager_init(coil_error_manager_t *manager, coil_logger_t *logger);

/* Create a new error manager */
coil_error_manager_t *coil_error_manager_create(coil_logger_t *logger);

/* Add an error to the manager */
void coil_error_add(coil_error_manager_t *manager, 
                  coil_error_code_t code, 
                  coil_error_severity_t severity,
                  const coil_stream_pos_t *position,
                  const char *fmt, ...);

/* Convenience functions for different error severities */
void coil_error_info(coil_error_manager_t *manager, 
                  coil_error_code_t code,
                  const coil_stream_pos_t *position,
                  const char *fmt, ...);

void coil_error_warning(coil_error_manager_t *manager, 
                      coil_error_code_t code,
                      const coil_stream_pos_t *position,
                      const char *fmt, ...);

void coil_error_error(coil_error_manager_t *manager, 
                    coil_error_code_t code,
                    const coil_stream_pos_t *position,
                    const char *fmt, ...);

void coil_error_fatal(coil_error_manager_t *manager, 
                    coil_error_code_t code,
                    const coil_stream_pos_t *position,
                    const char *fmt, ...);

/* Default error manager convenience macros */
#define COIL_DEFAULT_ERROR_INFO(code, position, ...) \
  coil_error_info(coil_default_error_manager, code, position, __VA_ARGS__)

#define COIL_DEFAULT_ERROR_WARNING(code, position, ...) \
  coil_error_warning(coil_default_error_manager, code, position, __VA_ARGS__)

#define COIL_DEFAULT_ERROR_ERROR(code, position, ...) \
  coil_error_error(coil_default_error_manager, code, position, __VA_ARGS__)

#define COIL_DEFAULT_ERROR_FATAL(code, position, ...) \
  coil_error_fatal(coil_default_error_manager, code, position, __VA_ARGS__)

/* Check if there are any errors of given severity or higher */
bool coil_error_has_errors(coil_error_manager_t *manager, coil_error_severity_t min_severity);

/* Dump all errors to the logger */
void coil_error_dump(coil_error_manager_t *manager);

/* Clear all errors */
void coil_error_clear(coil_error_manager_t *manager);

/* Create a stream position */
coil_stream_pos_t coil_stream_pos_create(const char *file_name, size_t line, size_t column, size_t offset);

/* Cleanup */
void coil_error_manager_cleanup(coil_error_manager_t *manager);
void coil_error_manager_destroy(coil_error_manager_t *manager);

/* Initialize library error management */
void coil_error_init(void);

/* Cleanup library error management */
void coil_error_cleanup(void);

/* Get an error message for a given error code */
const char *coil_error_message(coil_error_code_t code);

#endif /* COIL_ERROR_H */