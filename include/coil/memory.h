/**
* @file memory.h
* @brief Memory management utilities for COIL
* 
* This file provides optimized memory allocation functions for the COIL library,
* using Linux-specific memory allocation strategies for better performance.
*/

#ifndef __COIL_INCLUDE_GUARD_MEMORY_H
#define __COIL_INCLUDE_GUARD_MEMORY_H

#include <coil/types.h>
#include <coil/err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Allocate aligned memory using mmap
* 
* Uses Linux's mmap to allocate memory with specific alignment requirements.
* This is particularly useful for ensuring page-aligned allocations.
* 
* @param size Size of memory to allocate
* @param alignment Alignment requirement (must be page size multiple)
* @return void* Pointer to allocated memory or NULL on failure
* 
* @note For optimal performance, alignment should be a multiple of the system page size
*/
void* coil_mmap_alloc(coil_size_t size, coil_size_t alignment);

/**
* @brief Free memory allocated with coil_mmap_alloc
* 
* @param ptr Pointer to allocated memory
* @param size Size of the allocated memory
* @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_INVAL for invalid pointer
* 
* @note Size must match the originally allocated size
*/
coil_err_t coil_mmap_free(void *ptr, coil_size_t size);

/**
* @brief Get system page size
* 
* @return coil_size_t System page size in bytes
*/
coil_size_t coil_get_page_size(void);

/**
* @brief Align a value up to the nearest multiple of alignment
* 
* @param value Value to align
* @param alignment Alignment value (must be power of 2)
* @return coil_size_t Aligned value
*/
static inline coil_size_t coil_align_up(coil_size_t value, coil_size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
* @brief Align a value down to the nearest multiple of alignment
* 
* @param value Value to align
* @param alignment Alignment value (must be power of 2)
* @return coil_size_t Aligned value
*/
static inline coil_size_t coil_align_down(coil_size_t value, coil_size_t alignment) {
    return value & ~(alignment - 1);
}

/**
* @brief Calculate the size of a section including alignment padding
* 
* @param size Base size
* @param alignment Required alignment (typically page size)
* @return coil_size_t Aligned size
*/
static inline coil_size_t coil_section_aligned_size(coil_size_t size, coil_size_t alignment) {
    return coil_align_up(size, alignment);
}

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_MEMORY_H */