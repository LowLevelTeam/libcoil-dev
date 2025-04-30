/**
* @file mem.c
* @brief Memory management implementation for libcoil-dev
*/

#include <coil/base.h>
#include "srcdeps.h"

/**
* @brief Get system page size
*/
coil_size_t coil_get_page_size(void) {
  long page_size = sysconf(_SC_PAGESIZE);
  return (page_size > 0) ? (coil_size_t)page_size : 4096; // Default to 4k if failed
}

/**
* @brief Allocate aligned memory using mmap
*/
void* coil_mmap(coil_size_t size, coil_size_t alignment) {
  // Validate inputs
  if (size == 0) {
    COIL_ERROR(COIL_ERR_INVAL, "Size cannot be zero");
    return NULL;
  }
  
  // Ensure alignment is at least page size
  coil_size_t page_size = coil_get_page_size();
  if (alignment < page_size) {
    alignment = page_size;
  }
  
  // Align size to page size
  coil_size_t aligned_size = coil_aligned_size(size, page_size);
  
  // Create memory mapping
  void *ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, 
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  
  if (ptr == MAP_FAILED) {
    COIL_ERROR(COIL_ERR_NOMEM, "Memory mapping failed");
    return NULL;
  }
  
  return ptr;
}

/**
* @brief Free memory allocated with coil_mmap
*/
coil_err_t coil_munmap(void *ptr, coil_size_t size) {
  if (ptr == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Pointer is NULL");
  }
  
  coil_size_t page_size = coil_get_page_size();
  coil_size_t aligned_size = coil_aligned_size(size, page_size);
  
  if (munmap(ptr, aligned_size) != 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Failed to unmap memory");
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Allocate memory with standard malloc
*/
void* coil_malloc(coil_size_t size) {
  if (size == 0) {
    COIL_ERROR(COIL_ERR_INVAL, "Size cannot be zero");
    return NULL;
  }
  
  void *ptr = malloc(size);
  if (ptr == NULL) {
    COIL_ERROR(COIL_ERR_NOMEM, "Memory allocation failed");
  }
  
  return ptr;
}

/**
* @brief Allocate zero-initialized memory
*/
void* coil_calloc(coil_size_t nmemb, coil_size_t size) {
  if (nmemb == 0 || size == 0) {
    COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters for calloc");
    return NULL;
  }
  
  void *ptr = calloc(nmemb, size);
  if (ptr == NULL) {
    COIL_ERROR(COIL_ERR_NOMEM, "Memory allocation failed");
  }
  
  return ptr;
}

/**
* @brief Reallocate memory block
*/
void* coil_realloc(void *ptr, coil_size_t size) {
  if (size == 0) {
    coil_free(ptr);
    return NULL;
  }
  
  void *new_ptr = realloc(ptr, size);
  if (new_ptr == NULL && size > 0) {
    COIL_ERROR(COIL_ERR_NOMEM, "Memory reallocation failed");
  }
  
  return new_ptr;
}

/**
* @brief Free memory allocated with coil_malloc, coil_calloc, or coil_realloc
*/
void coil_free(void *ptr) {
  free(ptr);
}

/**
* @brief Copy memory from one location to another
*/
void* coil_memcpy(void *dest, const void *src, coil_size_t n) {
  if (dest == NULL || src == NULL) {
    COIL_ERROR(COIL_ERR_INVAL, "Invalid source or destination pointer");
    return NULL;
  }
  
  return memcpy(dest, src, n);
}

/**
* @brief Copy memory from one location to another (handling overlapping)
*/
void* coil_memmove(void *dest, const void *src, coil_size_t n) {
  if (dest == NULL || src == NULL) {
    COIL_ERROR(COIL_ERR_INVAL, "Invalid source or destination pointer");
    return NULL;
  }
  
  return memmove(dest, src, n);
}

/**
* @brief Set memory to a specific value
*/
void* coil_memset(void *s, int c, coil_size_t n) {
  if (s == NULL) {
    COIL_ERROR(COIL_ERR_INVAL, "Invalid pointer");
    return NULL;
  }
  
  return memset(s, c, n);
}

/**
* @brief Compare memory blocks
*/
int coil_memcmp(const void *s1, const void *s2, coil_size_t n) {
  if (s1 == NULL || s2 == NULL) {
    COIL_ERROR(COIL_ERR_INVAL, "Invalid pointers for comparison");
    return 0;
  }
  
  return memcmp(s1, s2, n);
}