/**
 * @file test_coil.cpp
 * @brief Tests for the main COIL module
 */

#include <catch2/catch_test_macros.hpp>
#include "coil/coil.hpp"

// Basic tests for the COIL main library interface
TEST_CASE("COIL version information", "[coil]") {
  // Initialize the library
  coil::initialize();
  
  // Get the version
  const coil::Version& version = coil::getVersion();
  
  // Check that version is valid
  CHECK(version.major == 0);
  CHECK(version.minor == 1);
  CHECK(version.patch == 0);
  CHECK(version.string == "COIL 0.1.0");
  CHECK(!version.build.empty());
  
  // Get configuration
  const coil::Configuration& config = coil::getConfiguration();
  
  // Configuration should have valid values
  #ifdef NDEBUG
    CHECK(config.debug_enabled == false);
    CHECK(config.asserts_enabled == false);
  #else
    CHECK(config.debug_enabled == true);
    CHECK(config.asserts_enabled == true);
  #endif
  
  // Shutdown
  coil::shutdown();
}

TEST_CASE("COIL library initialization", "[coil]") {
  // Initialize the library
  coil::initialize();
  
  // Verify that it's initialized
  CHECK(coil::Library::instance().isInitialized() == true);
  
  // Calling initialize again should be safe
  coil::initialize();
  CHECK(coil::Library::instance().isInitialized() == true);
  
  // Shutdown
  coil::shutdown();
  CHECK(coil::Library::instance().isInitialized() == false);
  
  // Calling shutdown again should be safe
  coil::shutdown();
  CHECK(coil::Library::instance().isInitialized() == false);
}