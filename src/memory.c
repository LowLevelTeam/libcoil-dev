#include <coil/memory.h>
#include <coil/err.h>

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


/**
 * @brief Get system page size
 * 
 * @return coil_size_t System page size in bytes
 */
coil_size_t coil_get_page_size(void) {
    static coil_size_t page_size = 0;
    
    if (page_size == 0) {
        // Cache the page size since it won't change during program execution
        long result = sysconf(_SC_PAGESIZE);
        if (result > 0) {
            page_size = (coil_size_t)result;
        } else {
            // Default to 4K if we can't get the actual page size
            page_size = 4096;
        }
    }
    
    return page_size;
}

/**
 * @brief Allocate aligned memory using mmap
 * 
 * @param size Size of memory to allocate
 * @param alignment Alignment requirement (must be page size multiple)
 * @return void* Pointer to allocated memory or NULL on failure
 */
void* coil_mmap_alloc(coil_size_t size, coil_size_t alignment) {
    // Validate parameters
    if (size == 0) {
        COIL_ERROR(COIL_ERR_INVAL, "Cannot allocate zero bytes");
        return NULL;
    }
    
    coil_size_t page_size = coil_get_page_size();
    
    // Ensure alignment is at least page size and a power of 2
    if (alignment < page_size) {
        alignment = page_size;
    } else if ((alignment & (alignment - 1)) != 0) {
        // Round up to nearest power of 2
        coil_size_t temp = alignment;
        alignment = 1;
        while (temp > 0) {
            temp >>= 1;
            alignment <<= 1;
        }
    }
    
    // Round size up to page size
    coil_size_t aligned_size = coil_align_up(size, page_size);
    
    // Use mmap to allocate memory
    void *ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                     
    if (ptr == MAP_FAILED) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "mmap allocation failed: %s", strerror(errno));
        COIL_ERROR(COIL_ERR_NOMEM, error_msg);
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief Free memory allocated with coil_mmap_alloc
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of the allocated memory
 * @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_INVAL for invalid pointer
 */
coil_err_t coil_mmap_free(void *ptr, coil_size_t size) {
    // Validate parameters
    if (ptr == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Cannot free NULL pointer");
    }
    
    if (size == 0) {
        return COIL_ERROR(COIL_ERR_INVAL, "Cannot free zero bytes");
    }
    
    // Round size up to page size
    coil_size_t page_size = coil_get_page_size();
    coil_size_t aligned_size = coil_align_up(size, page_size);
    
    // Unmap the memory
    if (munmap(ptr, aligned_size) != 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "munmap failed: %s", strerror(errno));
        return COIL_ERROR(COIL_ERR_INVAL, error_msg);
    }
    
    return COIL_ERR_GOOD;
}