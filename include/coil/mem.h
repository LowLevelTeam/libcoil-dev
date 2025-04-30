/**
* @file mem.h
* @brief Memory management and operation functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_MEM_H
#define __COIL_INCLUDE_GUARD_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Get system page size
* 
* @return coil_size_t System page size in bytes
*/
coil_size_t coil_get_page_size(void);

/**
* @brief Allocate aligned memory using mmap
* 
* Uses mmap to allocate memory with specific alignment requirements.
* This is particularly useful for ensuring page-aligned allocations.
* 
* @param size Size of memory to allocate
* @param alignment Alignment requirement (must be page size multiple)
* @return void* Pointer to allocated memory or NULL on failure
* 
* @note For optimal performance, alignment should be a multiple of the system page size
*/
void* coil_mmap(coil_size_t size, coil_size_t alignment);

/**
* @brief Free memory allocated with coil_mmap
* 
* @param ptr Pointer to allocated memory
* @param size Size of the allocated memory
* @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_INVAL for invalid pointer
* 
* @note Size must match the originally allocated size
*/
coil_err_t coil_munmap(void *ptr, coil_size_t size);

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
static inline coil_size_t coil_aligned_size(coil_size_t size, coil_size_t alignment) {
  return coil_align_up(size, alignment);
}

/**
* @brief Allocate memory with standard malloc
*
* This function provides error reporting integration with the coilt error system.
*
* @param size Size of memory to allocate
* @return void* Pointer to allocated memory or NULL on failure
*/
void* coil_malloc(coil_size_t size);

/**
* @brief Allocate zero-initialized memory
*
* @param nmemb Number of elements
* @param size Size of each element
* @return void* Pointer to allocated memory or NULL on failure
*/
void* coil_calloc(coil_size_t nmemb, coil_size_t size);

/**
* @brief Reallocate memory block
*
* @param ptr Pointer to existing memory block
* @param size New size for the memory block
* @return void* Pointer to reallocated memory or NULL on failure
*/
void* coil_realloc(void *ptr, coil_size_t size);

/**
* @brief Free memory allocated with coil_malloc, coil_calloc, or coil_realloc
*
* @param ptr Pointer to allocated memory
*/
void coil_free(void *ptr);

/**
* @brief Copy memory from one location to another
*
* @param dest Destination buffer
* @param src Source buffer
* @param n Number of bytes to copy
* @return void* Pointer to destination
*/
void* coil_memcpy(void *dest, const void *src, coil_size_t n);

/**
* @brief Copy memory from one location to another (handling overlapping)
*
* @param dest Destination buffer
* @param src Source buffer
* @param n Number of bytes to copy
* @return void* Pointer to destination
*/
void* coil_memmove(void *dest, const void *src, coil_size_t n);

/**
* @brief Set memory to a specific value
*
* @param s Memory to set
* @param c Value to set (converted to unsigned char)
* @param n Number of bytes to set
* @return void* Pointer to memory
*/
void* coil_memset(void *s, int c, coil_size_t n);

/**
* @brief Compare memory blocks
* 
* @param s1 First block
* @param s2 Second block
* @param n Number of bytes to compare
* @return int <0 if s1<s2, 0 if s1=s2, >0 if s1>s2
*/
int coil_memcmp(const void *s1, const void *s2, coil_size_t n);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_MEM_H