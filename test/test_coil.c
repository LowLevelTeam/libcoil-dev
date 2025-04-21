/**
 * @file test_coil.c
 * @brief Tests for the main COIL library functionality
 */

#include "test_framework.h"
#include <coil/coil.h>
#include <string.h>

// Test library initialization and shutdown
void test_coil_init_shutdown(void) {
  // Should not be initialized initially
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should start uninitialized");
  
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should be initialized after init");
  
  // Try to initialize again (should succeed but be a no-op)
  err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Double initialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should still be initialized");
  
  // Shutdown
  coil_shutdown();
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should be uninitialized after shutdown");
  
  // Shutdown again (should be a no-op)
  coil_shutdown();
  TEST_ASSERT_EQUAL_INT(0, coil_is_initialized(), "Library should still be uninitialized");
  
  // Initialize again to ensure we can reinitialize
  err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Reinitialization should succeed");
  TEST_ASSERT_EQUAL_INT(1, coil_is_initialized(), "Library should be initialized after reinit");
  
  // Clean up
  coil_shutdown();
}

// Test version information retrieval
void test_coil_version(void) {
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  
  // Get version
  coil_version_t version;
  err = coil_get_version(&version);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Get version should succeed");
  
  // Check version information
  TEST_ASSERT(version.major >= 0, "Major version should be valid");
  TEST_ASSERT(version.minor >= 0, "Minor version should be valid");
  TEST_ASSERT(version.patch >= 0, "Patch version should be valid");
  
  // String should contain version numbers
  char expected_prefix[32];
  sprintf(expected_prefix, "COIL %d.%d.%d", version.major, version.minor, version.patch);
  TEST_ASSERT(strstr(version.string, expected_prefix) != NULL, "Version string should contain version numbers");
  
  // Build string should not be empty
  TEST_ASSERT(version.build != NULL && strlen(version.build) > 0, "Build string should not be empty");
  
  // Test with NULL pointer
  err = coil_get_version(NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "NULL pointer should fail");
  
  // Clean up
  coil_shutdown();
}

// Test configuration information retrieval
void test_coil_configuration(void) {
  // Initialize
  coil_err_t err = coil_initialize();
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Initialization should succeed");
  
  // Get configuration
  coil_configuration_t config;
  err = coil_get_configuration(&config);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_GOOD, err, "Get configuration should succeed");
  
  // Check configuration validity (debug and asserts can be either 0 or 1)
  TEST_ASSERT(config.debug_enabled == 0 || config.debug_enabled == 1, 
              "Debug enabled should be valid boolean");
  TEST_ASSERT(config.asserts_enabled == 0 || config.asserts_enabled == 1, 
              "Asserts enabled should be valid boolean");
              
  // Test with NULL pointer
  err = coil_get_configuration(NULL);
  TEST_ASSERT_EQUAL_INT(COIL_ERR_INVAL, err, "NULL pointer should fail");
  
  // Clean up
  coil_shutdown();
}

// Test end-to-end integration
void test_coil_integration(void) {
  // TODO
}

// Define test array
test_t coil_tests[] = {
  {"COIL Init/Shutdown", test_coil_init_shutdown},
  {"COIL Version", test_coil_version},
  {"COIL Configuration", test_coil_configuration},
  {"COIL Integration", test_coil_integration}
};

// Run COIL tests
void run_coil_tests(void) {
  run_tests(coil_tests, sizeof(coil_tests) / sizeof(coil_tests[0]));
}