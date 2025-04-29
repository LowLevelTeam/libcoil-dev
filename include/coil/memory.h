/**
* @file memory.h
* @brief Memory management utilities for COIL
* 
* This file provides wrapper functions around libcoilt memory utilities
* to ensure compatibility while leveraging the standardized implementation.
*/

#ifndef __COIL_INCLUDE_GUARD_MEMORY_H
#define __COIL_INCLUDE_GUARD_MEMORY_H

#include <coilt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Allocate aligned memory using mmap
* 
* Wrapper around coilt_mmap_alloc for compatibility.
* 
* @param size Size of memory to allocate
* @param alignment Alignment requirement (must be page size multiple)
* @return void* Pointer to allocated memory or NULL on failure
* 
* @note For optimal performance, alignment should be a multiple of the system page size
*/
static inline void* coil_mmap_alloc(coil_size_t size, coil_size_t alignment) {
  return coilt_mmap_alloc(size, alignment);
}

/**
* @brief Free memory allocated with coil_mmap_alloc
* 
* Wrapper around coilt_mmap_free for compatibility.
* 
* @param ptr Pointer to allocated memory
* @param size Size of the allocated memory
* @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_INVAL for invalid pointer
* 
* @note Size must match the originally allocated size
*/
static inline coil_err_t coil_mmap_free(void *ptr, coil_size_t size) {
  return coilt_mmap_free(ptr, size);
}

/**
* @brief Get system page size
* 
* Wrapper around coilt_get_page_size for compatibility.
* 
* @return coil_size_t System page size in bytes
*/
static inline coil_size_t coil_get_page_size(void) {
  return coilt_get_page_size();
}

/**
* @brief Calculate the size of a section including alignment padding
* 
* Wrapper around coilt_aligned_size for compatibility.
* 
* @param size Base size
* @param alignment Required alignment (typically page size)
* @return coil_size_t Aligned size
*/
static inline coil_size_t coil_section_aligned_size(coil_size_t size, coil_size_t alignment) {
  return coilt_aligned_size(size, alignment);
}

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_MEMORY_H */