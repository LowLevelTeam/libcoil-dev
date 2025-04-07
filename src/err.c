/* src/err.c */
#include "coil/err.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Global default error manager */
coil_error_manager_t *coil_default_error_manager = NULL;

/* Error code messages */
static const char *error_messages[] = {
  "No error",                 /* COIL_ERR_NONE */
  "Memory allocation failure", /* COIL_ERR_MEMORY */
  "I/O error",                /* COIL_ERR_IO */
  "Invalid format",           /* COIL_ERR_FORMAT */
  "Syntax error",             /* COIL_ERR_SYNTAX */
  "Semantic error",           /* COIL_ERR_SEMANTIC */
  "Invalid reference",        /* COIL_ERR_REFERENCE */
  "Overflow",                 /* COIL_ERR_OVERFLOW */
  "Underflow",                /* COIL_ERR_UNDERFLOW */
  "Out of bounds",            /* COIL_ERR_BOUNDS */
  "Invalid state",            /* COIL_ERR_STATE */
  "Invalid argument",         /* COIL_ERR_ARGUMENT */
  "Internal error",           /* COIL_ERR_INTERNAL */
  "Unsupported operation",    /* COIL_ERR_UNSUPPORTED */
  "Custom error"              /* COIL_ERR_CUSTOM */
};

int coil_error_manager_init(coil_error_manager_t *manager, coil_logger_t *logger) {
  if (!manager) return -1;
  
  manager->head = NULL;
  manager->tail = NULL;
  manager->count = 0;
  manager->logger = logger ? logger : coil_default_logger;
  
  if (pthread_mutex_init(&manager->lock, NULL) != 0) {
      return -1;
  }
  
  manager->initialized = true;
  return 0;
}

coil_error_manager_t *coil_error_manager_create(coil_logger_t *logger) {
  coil_error_manager_t *manager = (coil_error_manager_t *)malloc(sizeof(coil_error_manager_t));
  if (!manager) return NULL;
  
  if (coil_error_manager_init(manager, logger) != 0) {
      free(manager);
      return NULL;
  }
  
  return manager;
}

void coil_error_add(coil_error_manager_t *manager, 
                  coil_error_code_t code, 
                  coil_error_severity_t severity,
                  const coil_stream_pos_t *position,
                  const char *fmt, ...) {
                  
  if (!manager || !manager->initialized) return;
  
  coil_error_entry_t *entry = (coil_error_entry_t *)malloc(sizeof(coil_error_entry_t));
  if (!entry) return;
  
  entry->code = code;
  entry->severity = severity;
  entry->next = NULL;
  
  if (position) {
      entry->position = *position;
  } else {
      entry->position.file_name = NULL;
      entry->position.line = 0;
      entry->position.column = 0;
      entry->position.offset = 0;
  }
  
  /* Format message */
  va_list args;
  va_start(args, fmt);
  vsnprintf(entry->message, sizeof(entry->message), fmt, args);
  va_end(args);
  
  /* Add to list */
  pthread_mutex_lock(&manager->lock);
  
  if (manager->tail) {
      manager->tail->next = entry;
      manager->tail = entry;
  } else {
      manager->head = manager->tail = entry;
  }
  
  manager->count++;
  
  /* Log the error immediately based on severity */
  if (manager->logger) {
      coil_log_level_t log_level;
      
      switch (severity) {
          case COIL_SEVERITY_INFO:
              log_level = COIL_LOG_INFO;
              break;
          case COIL_SEVERITY_WARNING:
              log_level = COIL_LOG_WARNING;
              break;
          case COIL_SEVERITY_ERROR:
              log_level = COIL_LOG_ERROR;
              break;
          case COIL_SEVERITY_FATAL:
              log_level = COIL_LOG_FATAL;
              break;
      }
      
      if (position && position->file_name) {
          coil_log(manager->logger, log_level, position->file_name, position->line, "", 
                    "%s: %s (at line %zu, column %zu, offset %zu)", 
                    coil_error_message(code), 
                    entry->message,
                    position->line,
                    position->column,
                    position->offset);
      } else {
          coil_log(manager->logger, log_level, "unknown", 0, "", 
                    "%s: %s", 
                    coil_error_message(code), 
                    entry->message);
      }
  }
  
  pthread_mutex_unlock(&manager->lock);
}

void coil_error_info(coil_error_manager_t *manager, 
                  coil_error_code_t code,
                  const coil_stream_pos_t *position,
                  const char *fmt, ...) {
  if (!manager || !manager->initialized) return;
  
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  
  coil_error_add(manager, code, COIL_SEVERITY_INFO, position, "%s", message);
}

void coil_error_warning(coil_error_manager_t *manager, 
                      coil_error_code_t code,
                      const coil_stream_pos_t *position,
                      const char *fmt, ...) {
  if (!manager || !manager->initialized) return;
  
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  
  coil_error_add(manager, code, COIL_SEVERITY_WARNING, position, "%s", message);
}

void coil_error_error(coil_error_manager_t *manager, 
                    coil_error_code_t code,
                    const coil_stream_pos_t *position,
                    const char *fmt, ...) {
  if (!manager || !manager->initialized) return;
  
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  
  coil_error_add(manager, code, COIL_SEVERITY_ERROR, position, "%s", message);
}

void coil_error_fatal(coil_error_manager_t *manager, 
                    coil_error_code_t code,
                    const coil_stream_pos_t *position,
                    const char *fmt, ...) {
  if (!manager || !manager->initialized) return;
  
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  
  coil_error_add(manager, code, COIL_SEVERITY_FATAL, position, "%s", message);
}

bool coil_error_has_errors(coil_error_manager_t *manager, coil_error_severity_t min_severity) {
  if (!manager || !manager->initialized) return false;
  
  bool has_errors = false;
  
  pthread_mutex_lock(&manager->lock);
  
  coil_error_entry_t *entry = manager->head;
  while (entry) {
      if (entry->severity >= min_severity) {
          has_errors = true;
          break;
      }
      entry = entry->next;
  }
  
  pthread_mutex_unlock(&manager->lock);
  
  return has_errors;
}

void coil_error_dump(coil_error_manager_t *manager) {
  if (!manager || !manager->initialized || !manager->logger) return;
  
  pthread_mutex_lock(&manager->lock);
  
  if (manager->count == 0) {
      coil_log(manager->logger, COIL_LOG_INFO, "", 0, "", "No errors reported");
  } else {
      coil_log(manager->logger, COIL_LOG_INFO, "", 0, "", "Error summary (%zu errors):", manager->count);
      
      coil_error_entry_t *entry = manager->head;
      size_t i = 0;
      
      while (entry) {
          coil_log_level_t log_level;
          
          switch (entry->severity) {
              case COIL_SEVERITY_INFO:
                  log_level = COIL_LOG_INFO;
                  break;
              case COIL_SEVERITY_WARNING:
                  log_level = COIL_LOG_WARNING;
                  break;
              case COIL_SEVERITY_ERROR:
                  log_level = COIL_LOG_ERROR;
                  break;
              case COIL_SEVERITY_FATAL:
                  log_level = COIL_LOG_FATAL;
                  break;
          }
          
          if (entry->position.file_name) {
              coil_log(manager->logger, log_level, entry->position.file_name, entry->position.line, "", 
                        "[%zu] %s: %s (at line %zu, column %zu, offset %zu)", 
                        i++,
                        coil_error_message(entry->code), 
                        entry->message,
                        entry->position.line,
                        entry->position.column,
                        entry->position.offset);
          } else {
              coil_log(manager->logger, log_level, "unknown", 0, "", 
                        "[%zu] %s: %s", 
                        i++,
                        coil_error_message(entry->code), 
                        entry->message);
          }
          
          entry = entry->next;
      }
  }
  
  pthread_mutex_unlock(&manager->lock);
}

void coil_error_clear(coil_error_manager_t *manager) {
  if (!manager || !manager->initialized) return;
  
  pthread_mutex_lock(&manager->lock);
  
  coil_error_entry_t *entry = manager->head;
  while (entry) {
      coil_error_entry_t *next = entry->next;
      free(entry);
      entry = next;
  }
  
  manager->head = NULL;
  manager->tail = NULL;
  manager->count = 0;
  
  pthread_mutex_unlock(&manager->lock);
}

coil_stream_pos_t coil_stream_pos_create(const char *file_name, size_t line, size_t column, size_t offset) {
  coil_stream_pos_t pos;
  pos.file_name = file_name;
  pos.line = line;
  pos.column = column;
  pos.offset = offset;
  return pos;
}

void coil_error_manager_cleanup(coil_error_manager_t *manager) {
  if (!manager || !manager->initialized) return;
  
  coil_error_clear(manager);
  pthread_mutex_destroy(&manager->lock);
  manager->initialized = false;
}

void coil_error_manager_destroy(coil_error_manager_t *manager) {
  if (!manager) return;
  
  coil_error_manager_cleanup(manager);
  free(manager);
}

void coil_error_init(void) {
  if (!coil_default_logger) {
      coil_log_init();
  }
  
  if (!coil_default_error_manager) {
      coil_default_error_manager = coil_error_manager_create(coil_default_logger);
  }
}

void coil_error_cleanup(void) {
  if (coil_default_error_manager) {
      coil_error_manager_destroy(coil_default_error_manager);
      coil_default_error_manager = NULL;
  }
}

const char *coil_error_message(coil_error_code_t code) {
  if (code >= 0 && code < COIL_ERR_MAX) {
      return error_messages[code];
  }
  return "Unknown error";
}