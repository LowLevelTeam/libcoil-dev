/**
* @file test_coil.cpp
* @brief Tests for the main COIL module
*/

#include <catch2/catch_test_macros.hpp>
#include "coil/coil.hpp"

// Basic tests for the COIL main library interface
TEST_CASE("COIL version information", "[coil]") {
  // Get the version
  coil::Version version = coil::getVersion();
  
  // Check that version is valid
  CHECK(version.major == 0);
  CHECK(version.minor == 1);
  CHECK(version.patch == 0);
  CHECK(version.string != nullptr);
  CHECK(std::string(version.string) == "COIL 0.1.0");
}