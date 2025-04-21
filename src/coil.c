/*
 * Main implementation file for the COIL library
 */

#include <coil/coil.h>
#include <coil/err.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Library state
static struct {
	int initialized;
	coil_version_t version;
	coil_configuration_t config;
	char version_string[32];
	char build_string[64];
} g_coil_state = {0};

// Default error callback
static void coil_default_error_callback(
	coil_error_level_t level,
	const char* message,
	const coil_error_position_t* position,
	void* user_data
) {
	(void)user_data; // Unused
	
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
		fprintf(stderr, "COIL %s: %s:%zu: %s\n", 
						level_str, position->file, position->line, message);
	} else {
		fprintf(stderr, "COIL %s: %s\n", level_str, message);
	}
}

coil_err_t coil_initialize(void) {
	if (g_coil_state.initialized) {
		return COIL_ERR_GOOD; // Already initialized
	}
	
	// Initialize error system
	coil_err_t err = coil_error_init();
	if (err != COIL_ERR_GOOD) {
		return err;
	}
	
	// Set default error callback
	coil_error_set_callback(coil_default_error_callback, NULL);
	
	// Set up version information
	g_coil_state.version.major = COIL_VERSION_MAJOR;
	g_coil_state.version.minor = COIL_VERSION_MINOR;
	g_coil_state.version.patch = COIL_VERSION_PATCH;
	
	// Format version string
	snprintf(g_coil_state.version_string, sizeof(g_coil_state.version_string), 
					 "COIL %d.%d.%d", 
					 COIL_VERSION_MAJOR, COIL_VERSION_MINOR, COIL_VERSION_PATCH);
	
	g_coil_state.version.string = g_coil_state.version_string;
	
	// Get build timestamp
	time_t build_time = time(NULL);
	struct tm* time_info = localtime(&build_time);
	strftime(g_coil_state.build_string, sizeof(g_coil_state.build_string),
					 "%Y-%m-%d %H:%M:%S", time_info);
	
	g_coil_state.version.build = g_coil_state.build_string;
	
	// Set up configuration
#ifdef NDEBUG
	g_coil_state.config.debug_enabled = 0;
	g_coil_state.config.asserts_enabled = 0;
#else
	g_coil_state.config.debug_enabled = 1;
	g_coil_state.config.asserts_enabled = 1;
#endif
	
	// Report initialization
	COIL_INFO(COIL_ERR_GOOD, g_coil_state.version_string);
	
	g_coil_state.initialized = 1;
	return COIL_ERR_GOOD;
}

void coil_shutdown(void) {
	if (!g_coil_state.initialized) {
		return;
	}
	
	// Report shutdown
	COIL_INFO(COIL_ERR_GOOD, "COIL Library shutdown");
	
	// Shutdown error system
	coil_error_shutdown();
	
	g_coil_state.initialized = 0;
}

coil_err_t coil_get_version(coil_version_t* version) {
	if (!version) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid version pointer");
	}
	
	// Copy version info
	memcpy(version, &g_coil_state.version, sizeof(coil_version_t));
	return COIL_ERR_GOOD;
}

coil_err_t coil_get_configuration(coil_configuration_t* config) {
	if (!config) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid configuration pointer");
	}
	
	// Copy configuration
	memcpy(config, &g_coil_state.config, sizeof(coil_configuration_t));
	return COIL_ERR_GOOD;
}

int coil_is_initialized(void) {
	return g_coil_state.initialized;
}