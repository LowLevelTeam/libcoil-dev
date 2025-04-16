/**
* @file coil.cpp
* @brief Main implementation file for the COIL library
*/

#include "coil/coil.hpp"

namespace coil {

// Current library version information
static const Version CURRENT_VERSION = {
  0,                   // major
  1,                   // minor
  0,                   // patch
  "COIL 0.1.0"         // string
};

Version getVersion() {
  return CURRENT_VERSION;
}

} // namespace coil