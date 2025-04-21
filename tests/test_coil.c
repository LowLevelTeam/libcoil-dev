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

// For combined test mode
#ifndef RUN_INDIVIDUAL
extern int test_verbosity;
#endif

/**
 * @brief Print library version and configuration information
 */
static void debug_print_coil_info(void) {
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (!verbosity) return;
  
  coil_version_t version;
  coil_get_version(&version);
  
  coil_configuration_t config;
  coil_get_configuration(&config);
  
  printf("COIL Library Info:\n");
  printf("  ├─ Version: %d.%d.%d\n", version.major, version.minor, version.patch);
  printf("  ├─ Version string: %s\n", version.string);
  printf("  ├─ Build: %s\n", version.build);
  printf("  ├─ Debug enabled: %s\n", config.debug_enabled ? "Yes" : "No");
  printf("  └─ Asserts enabled: %s\n", config.asserts_enabled ? "Yes" : "No");
}

/* Test library initialization and shutdown */
static void test_initialize_shutdown(void **state) {
  (void)state; /* unused */
  
  /* Initialize library */
  coil_err_t err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  /* Print library info */
  debug_print_coil_info();
  
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
  
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (verbosity) {
    printf("\nVersion details:\n");
    printf("  ├─ Major: %d\n", version.major);
    printf("  ├─ Minor: %d\n", version.minor);
    printf("  ├─ Patch: %d\n", version.patch);
    printf("  ├─ String: %s\n", version.string);
    printf("  └─ Build: %s\n", version.build);
  }
  
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
  
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (verbosity) {
    printf("\nConfiguration details:\n");
    printf("  ├─ Debug enabled: %s\n", config.debug_enabled ? "Yes" : "No");
    printf("  └─ Asserts enabled: %s\n", config.asserts_enabled ? "Yes" : "No");
  }
  
  /* Test with NULL parameter */
  err = coil_get_configuration(NULL);
  assert_int_equal(err, COIL_ERR_INVAL);
  
  /* Shutdown library */
  coil_shutdown();
}

/* Test initialization in different orders */
static void test_initialization_ordering(void **state) {
  (void)state; /* unused */
  
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (verbosity) {
    printf("\nTesting initialization ordering:\n");
  }
  
  /* Test initializing error system directly first */
  coil_err_t err = coil_error_init();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  if (verbosity) {
    printf("  ├─ Initialized error system directly: OK\n");
  }
  
  /* Then initialize library */
  err = coil_initialize();
  assert_int_equal(err, COIL_ERR_GOOD);
  
  if (verbosity) {
    printf("  ├─ Then initialized library: OK\n");
  }
  
  /* Verify initialized state */
  assert_true(coil_is_initialized());
  
  /* Shutdown library */
  coil_shutdown();
  
  if (verbosity) {
    printf("  ├─ Shutdown library: OK\n");
  }
  
  /* Error system should still be initialized since we initialized it separately */
  
  /* Shutdown error system */
  coil_error_shutdown();
  
  if (verbosity) {
    printf("  └─ Shutdown error system: OK\n");
  }
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
  
#ifdef RUN_INDIVIDUAL
  static int verbosity = 1; // Always verbose in individual mode
#else
  int verbosity = test_verbosity;
#endif

  if (verbosity) {
    const coil_object_header_t *header = coil_object_get_header(obj);
    
    printf("\nObject created with arena:\n");
    printf("  ├─ Section count: %d\n", header->section_count);
    printf("  ├─ Section index: %d\n", section_index);
    printf("  ├─ Arena capacity: %zu bytes\n", arena_capacity(arena));
    printf("  └─ Arena used: %zu bytes\n", arena_used(arena));
  }
  
  /* Cleanup */
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
  
  /* Shutdown library */
  coil_shutdown();
}

/* Get COIL tests for combined testing */
struct CMUnitTest *get_coil_tests(int *count) {
  static struct CMUnitTest coil_tests[] = {
    cmocka_unit_test(test_initialize_shutdown),
    cmocka_unit_test(test_version),
    cmocka_unit_test(test_configuration),
    cmocka_unit_test(test_initialization_ordering),
    cmocka_unit_test(test_with_arena),
  };
  
  *count = sizeof(coil_tests) / sizeof(coil_tests[0]);
  return coil_tests;
}

/* Individual test main function */
#ifdef RUN_INDIVIDUAL
int main(void) {
  printf("Running COIL library tests individually\n");
  
  const struct CMUnitTest *tests;
  int count;
  
  tests = get_coil_tests(&count);
  return cmocka_run_group_tests(tests, NULL, NULL);
}
#endif