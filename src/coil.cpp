/**
* @file coil.cpp
* @brief Main implementation file for the COIL library
*/

#include "coil/coil.hpp"
#include "coil/err.hpp"
#include "coil/obj.hpp"
#include <ctime>
#include <cstdlib>

namespace coil {

// Current library version information
static const Version CURRENT_VERSION = {
  0,                   // major
  1,                   // minor
  0,                   // patch
  "COIL 0.1.0",        // string
  __DATE__ " " __TIME__ // build timestamp
};

// Library configuration
static const Configuration LIBRARY_CONFIG = {
#ifdef NDEBUG
  false,              // debug_enabled
  false,              // asserts_enabled
#else
  true,               // debug_enabled
  true,               // asserts_enabled
#endif
  MAX_SECTIONS,       // max_sections
  MAX_SYMBOLS,        // max_symbols
  MAX_RELOCATIONS     // max_relocations
};

// Library initialization state
static bool g_initialized = false;

Version getVersion() {
  return CURRENT_VERSION;
}

Configuration getConfiguration() {
  return LIBRARY_CONFIG;
}

Result initialize() {
  if (g_initialized) {
    return Result::Success;
  }
  
  // Seed random number generator if needed
  srand(static_cast<unsigned int>(time(nullptr)));
  
  // Setup default error handling
  setErrorCallback(nullptr, nullptr);
  
  // Report initialization
  reportError(ErrorLevel::Info, "COIL Library %s initialized", CURRENT_VERSION.string);
  
  g_initialized = true;
  return Result::Success;
}

void shutdown() {
  if (!g_initialized) {
    return;
  }
  
  // Report shutdown
  reportError(ErrorLevel::Info, "COIL Library %s shutdown", CURRENT_VERSION.string);
  
  g_initialized = false;
}

} // namespace coil