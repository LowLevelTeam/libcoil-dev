/**
 * @file test_coil.c
 * @brief Tests for the main COIL library interface
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>
#include <stdio.h>

#include <coil/coil.h>

/* Test library initialization and shutdown */
static void test_initialize_shutdown(void **state) {
  (void)state; /* unused */
  
  /* Initialize library */
  coil_err_t err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Check initialization status */
  assert_true(coil_is_initialized());
  
  /* Test double initialization (should be harmless) */
  err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Shutdown library */
  coil_shutdown();
  
  /* Check initialization status */
  assert_false(coil_is_initialized());
  
  /* Test double shutdown (should be harmless) */
  coil_shutdown();
  assert_false(coil_is_initialized());
}

/* Test version information */
static void test_version(void **state) {
  (void)state; /* unused */
  
  /* Initialize library */
  coil_initialize();
  
  /* Get version information */
  coil_version_t version;
  coil_err_t err = coil_get_version(&version);
  
  /* Verify success */
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Check version fields */
  assert_int_equal(version.major, COIL_VERSION_MAJOR);
  assert_int_equal(version.minor, COIL_VERSION_MINOR);
  assert_int_equal(version.patch, COIL_VERSION_PATCH);
  
  /* Check version string format */
  assert_non_null(version.string);
  
  /* Version string should contain the version numbers */
  char expected_prefix[16];
  snprintf(expected_prefix, sizeof(expected_prefix), "COIL %d.%d.%d", 
           version.major, version.minor, version.patch);
  
  assert_true(strstr(version.string, expected_prefix) != NULL);
  
  /* Check build string */
  assert_non_null(version.build);
  
  /* Test with NULL parameter */
  err = coil_get_version(NULL);
  assert_int_equal(err, COIL_ERR_INVAL);
  
  /* Shutdown library */
  coil_shutdown();
}

/* Test configuration information */
static void test_configuration(void **state) {
  (void)state; /* unused */
  
  /* Initialize library */
  coil_initialize();
  
  /* Get configuration information */
  coil_configuration_t config;
  coil_err_t err = coil_get_configuration(&config);
  
  /* Verify success */
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Check configuration fields - these depend on build flags */
#ifdef NDEBUG
  assert_int_equal(config.debug_enabled, 0);
  assert_int_equal(config.asserts_enabled, 0);
#else
  assert_int_equal(config.debug_enabled, 1);
  assert_int_equal(config.asserts_enabled, 1);
#endif
  
  /* Test with NULL parameter */
  err = coil_get_configuration(NULL);
  assert_int_equal(err, COIL_ERR_INVAL);
  
  /* Shutdown library */
  coil_shutdown();
}

/* Test initialization in different orders */
static void test_initialization_ordering(void **state) {
  (void)state; /* unused */
  
  /* Test initializing error system directly first */
  coil_err_t err = coil_error_init();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Then initialize library */
  err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Verify initialized state */
  assert_true(coil_is_initialized());
  
  /* Shutdown library */
  coil_shutdown();
  
  /* Error system should still be initialized since we initialized it separately */
  
  /* Shutdown error system */
  coil_error_shutdown();
}

/* Test usage with arena allocator */
static void test_with_arena(void **state) {
  (void)state; /* unused */
  
  /* Initialize library */
  coil_initialize();
  
  /* Create arena */
  coil_arena_t *arena = arena_init(4096, 0);
  assert_non_null(arena);
  
  /* Create COIL object */
  coil_object_t *obj = coil_object_create(arena);
  assert_non_null(obj);
  
  /* Initialize string table */
  coil_err_t err = coil_object_init_string_table(obj, arena);
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Add a sample section */
  coil_u64_t name_offset = coil_object_add_string(obj, ".test", arena);
  assert_true(name_offset > 0);
  
  uint8_t code[] = {0x10, 0x02, 0x01, 0x01, 0x00, 0x04, 0x00, 0x2A};
  
  coil_u16_t section_index = coil_object_add_section(
    obj,
    name_offset,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    code,
    sizeof(code),
    arena
  );
  
  assert_true(section_index > 0);
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
  
  /* Shutdown library */
  coil_shutdown();
}

/* Main function running all tests */
int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_initialize_shutdown),
    cmocka_unit_test(test_version),
    cmocka_unit_test(test_configuration),
    cmocka_unit_test(test_initialization_ordering),
    cmocka_unit_test(test_with_arena),
  };
  
  return cmocka_run_group_tests(tests, NULL, NULL);
}