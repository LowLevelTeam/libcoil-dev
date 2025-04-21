/**
* @file err.hpp
* @brief Common error interface
*/

#ifndef __COIL_INCLUDE_GUARD_ERR_H
#define __COIL_INCLUDE_GUARD_ERR_H

#include <stdint.h>

/**
* @brief COIL Error Codes
*/
typedef enum coil_err_e {
  COIL_ERR_GOOD = 0,
  COIL_ERR_NOMEM = 1,
} coil_err_t;

/**
* @brief COIL error manager
*/
// TODO: For error management there should be a way to track errors throughout the programs with dedicated messages
//       this way warnings can be tracked as well as errors and fatals with helpful messages for debugging by developers
//       and users

#endif // __COIL_INCLUDE_GUARD_ERR_H