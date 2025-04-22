#include <coil/err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Global error context to store the last error
static coil_error_context_t g_error_context;

// Error callback function and user data
static coil_error_callback_t g_error_callback = NULL;
static void* g_error_callback_user_data = NULL;

// Error strings for each error code
static const char* g_error_strings[] = {
    "No error",                  // COIL_ERR_GOOD
    "Memory allocation failure", // COIL_ERR_NOMEM
    "Invalid argument",          // COIL_ERR_INVAL
    "I/O error",                 // COIL_ERR_IO
    "Format error",              // COIL_ERR_FORMAT
    "Not found",                 // COIL_ERR_NOTFOUND
    "Not supported",             // COIL_ERR_NOTSUP
    "Bad state",                 // COIL_ERR_BADSTATE
    "Already exists",            // COIL_ERR_EXISTS
    "Unknown error"              // COIL_ERR_UNKNOWN
};

/**
 * @brief Initialize the error system
 * 
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_error_init(void) {
    // Clear the error context
    memset(&g_error_context, 0, sizeof(g_error_context));
    g_error_callback = NULL;
    g_error_callback_user_data = NULL;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Shutdown the error system
 */
void coil_error_shutdown(void) {
    // Reset callback
    g_error_callback = NULL;
    g_error_callback_user_data = NULL;
}

/**
 * @brief Set error callback function
 * 
 * @param callback Callback function to handle errors
 * @param user_data User data to pass to the callback
 */
void coil_error_set_callback(coil_error_callback_t callback, void* user_data) {
    g_error_callback = callback;
    g_error_callback_user_data = user_data;
}

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
) {
    // Store error in context
    g_error_context.code = code;
    g_error_context.level = level;
    
    // Copy message with truncation protection
    if (message) {
        strncpy(g_error_context.message, message, sizeof(g_error_context.message) - 1);
        g_error_context.message[sizeof(g_error_context.message) - 1] = '\0';
    } else {
        g_error_context.message[0] = '\0';
    }
    
    // Store position information
    g_error_context.position.file = file;
    g_error_context.position.line = line;
    g_error_context.position.index = index;
    
    // Call user callback if registered
    if (g_error_callback) {
        g_error_callback(level, message, &g_error_context.position, g_error_callback_user_data);
    }
    
    // Output to stderr for errors and fatal errors by default
    if (level >= COIL_LEVEL_ERROR) {
        const char* level_str = (level == COIL_LEVEL_ERROR) ? "ERROR" : "FATAL";
        if (file) {
            fprintf(stderr, "%s: %s (%s:%zu)\n", level_str, message, file, line);
        } else {
            fprintf(stderr, "%s: %s\n", level_str, message);
        }
    }
    
    return code;
}

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
) {
    return coil_error_report(level, code, message, file, line, index);
}

/**
 * @brief Get the last error context
 * 
 * @return const coil_error_context_t* Pointer to the last error context
 */
const coil_error_context_t* coil_error_get_last(void) {
    return &g_error_context;
}

/**
 * @brief Clear the last error
 */
void coil_error_clear(void) {
    memset(&g_error_context, 0, sizeof(g_error_context));
}

/**
 * @brief Get string representation of an error code
 * 
 * @param code Error code
 * @return const char* String representation
 */
const char* coil_error_string(coil_err_t code) {
    if (code >= 0 && code < (sizeof(g_error_strings) / sizeof(g_error_strings[0]))) {
        return g_error_strings[code];
    }
    return "Invalid error code";
}