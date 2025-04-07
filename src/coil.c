/* src/coil.c */
#include "coil/coil.h"
#include <stdlib.h>
#include <string.h>

/* Library version */
#define COIL_VERSION_MAJOR 0
#define COIL_VERSION_MINOR 1
#define COIL_VERSION_PATCH 0
#define COIL_VERSION_STRING "0.1.0"

/* Error handler */
static coil_error_handler_fn error_handler = NULL;
static void *error_handler_user_data = NULL;

/* Last error message */
static char last_error[512] = {0};

/* Error handler function */
static void coil_default_error_handler(coil_error_code_t code,
                                      coil_error_severity_t severity,
                                      const coil_stream_pos_t *position,
                                      const char *message,
                                      void *user_data) {
    (void)user_data;
    
    /* Store the error message */
    memset(last_error, 0, sizeof(last_error));
    
    if (position && position->file_name) {
        snprintf(last_error, sizeof(last_error) - 1,
                "%s: %s (at %s line %zu, column %zu, offset %zu)",
                coil_error_message(code),
                message,
                position->file_name,
                position->line,
                position->column,
                position->offset);
    } else {
        snprintf(last_error, sizeof(last_error) - 1,
                "%s: %s",
                coil_error_message(code),
                message);
    }
    
    /* Log the error */
    if (coil_default_logger) {
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
            default:
                log_level = COIL_LOG_ERROR;
                break;
        }
        
        coil_log(coil_default_logger, log_level, position ? position->file_name : "unknown",
                position ? position->line : 0, "", "%s", last_error);
    }
    
    /* Call the custom error handler if set */
    if (error_handler) {
        error_handler(code, severity, position, message, error_handler_user_data);
    }
}

coil_version_t coil_get_version(void) {
    static coil_version_t version = {
        .major = COIL_VERSION_MAJOR,
        .minor = COIL_VERSION_MINOR,
        .patch = COIL_VERSION_PATCH,
        .version_string = COIL_VERSION_STRING
    };
    
    return version;
}

int coil_init(void) {
    /* Initialize in the correct order to handle dependencies */
    
    /* Step 1: Initialize logging */
    coil_log_init();
    if (!coil_default_logger) {
        return -1;
    }
    
    /* Step 2: Initialize error handling */
    coil_error_init();
    if (!coil_default_error_manager) {
        coil_log_cleanup();
        return -1;
    }
    
    /* Step 3: Initialize memory management */
    coil_memory_init();
    if (!coil_global_arena) {
        coil_error_cleanup();
        coil_log_cleanup();
        return -1;
    }
    
    /* Step 4: Initialize thread management */
    if (coil_thread_init() != 0) {
        coil_memory_cleanup();
        coil_error_cleanup();
        coil_log_cleanup();
        return -1;
    }
    
    /* Log initialization */
    coil_version_t version = coil_get_version();
    COIL_DEFAULT_INFO("libcoil-dev v%s initialized", version.version_string);
    
    return 0;
}

void coil_cleanup(void) {
    /* Clean up in the reverse order of initialization */
    
    /* Log cleanup */
    COIL_DEFAULT_INFO("libcoil-dev shutting down");
    
    /* Step 1: Clean up thread management */
    coil_thread_cleanup();
    
    /* Step 2: Clean up memory management */
    coil_memory_cleanup();
    
    /* Step 3: Clean up error handling */
    coil_error_cleanup();
    
    /* Step 4: Clean up logging */
    coil_log_cleanup();
}

const char *coil_get_last_error(void) {
    return last_error[0] ? last_error : NULL;
}

void coil_set_log_level(coil_log_level_t level) {
    if (coil_default_logger) {
        coil_logger_set_level(coil_default_logger, level);
    }
}

void coil_set_error_handler(coil_error_handler_fn handler, void *user_data) {
    error_handler = handler;
    error_handler_user_data = user_data;
}