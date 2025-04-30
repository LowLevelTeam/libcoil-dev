/**
* @file deps.h
* @brief Simple include for all external includes in the program
*/

#ifndef __COIL_INCLUDE_GUARD_DEPS_H
#define __COIL_INCLUDE_GUARD_DEPS_H

// Header Only (libc)
#include <stdint.h> // header only
#include <stddef.h> // header only
#include <stdarg.h> // header only

// libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Linux
#include <unistd.h> // universal
#include <sys/mman.h> // mmap, munmap, MAP_*, PROT_*

#endif // __COIL_INCLUDE_GUARD_DEPS_H