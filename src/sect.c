/**
* @file sect.c
* @brief COIL Object Section functionality implementation for libcoil-dev
*/

#include <coil/base.h>
#include <coil/sect.h>
#include "srcdeps.h"

/**
* @brief Initial capacity for sections when none is specified
*/
#define COIL_SECTION_DEFAULT_CAPACITY 1024

// Forward declaration for functions not in public API
static coil_err_t coil_section_resize(coil_section_t *sect, coil_size_t new_capacity);

/**
* @brief Initialize coil section (COIL_SECT_MODE_CREATE)
*/
coil_err_t coil_section_init(coil_section_t *sect, coil_size_t capacity) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // Use default capacity if none provided
  if (capacity == 0) {
    capacity = COIL_SECTION_DEFAULT_CAPACITY;
  }
  
  // Initialize section
  coil_memset(sect, 0, sizeof(coil_section_t));
  sect->mode = COIL_SECT_MODE_CREATE;
  
  // Allocate memory for data
  sect->data = (coil_byte_t *)coil_malloc(capacity);
  if (sect->data == NULL) {
    return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section data");
  }
  
  sect->capacity = capacity;
  sect->size = 0;
  sect->rindex = 0;
  sect->windex = 0;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Clean up section resources
*/
void coil_section_cleanup(coil_section_t *sect) {
  if (sect == NULL) {
    return;
  }
  
  if (sect->data != NULL) {
    // Handling depends on section mode and mapping status
    if (sect->is_mapped && sect->map_base != NULL) {
      // Unmap memory-mapped section
      munmap(sect->map_base, sect->map_size);
      sect->map_base = NULL;
      sect->map_size = 0;
    } else if (sect->mode != COIL_SECT_MODE_VIEW) {
      // For CREATE or MODIFY modes, we own the memory and need to free it
      coil_free(sect->data);
    }
    
    // In all cases, set data to NULL to avoid double-free
    sect->data = NULL;
  }
  
  // Reset is_mapped flag
  sect->is_mapped = 0;
}

/**
* @brief Ensure section has at least specified capacity
*/
coil_err_t coil_section_ensure_capacity(coil_section_t *sect, coil_size_t min_capacity) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // If we're in VIEW mode, we can't resize
  if (sect->mode == COIL_SECT_MODE_VIEW) {
    return COIL_ERROR(COIL_ERR_BADSTATE, "Cannot resize section in VIEW mode");
  }
  
  // If we already have enough capacity, we're done
  if (sect->capacity >= min_capacity) {
    return COIL_ERR_GOOD;
  }
  
  // Calculate new capacity (double current, but at least min_capacity)
  coil_size_t new_capacity = sect->capacity * 2;
  if (new_capacity < min_capacity) {
    new_capacity = min_capacity;
  }
  
  return coil_section_resize(sect, new_capacity);
}

/**
* @brief Internal function to resize a section
*/
static coil_err_t coil_section_resize(coil_section_t *sect, coil_size_t new_capacity) {
  // Allocate new buffer
  coil_byte_t *new_data = (coil_byte_t *)coil_malloc(new_capacity);
  if (new_data == NULL) {
    return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section resize");
  }
  
  // Copy existing data
  if (sect->data != NULL && sect->size > 0) {
    coil_memcpy(new_data, sect->data, sect->size);
  }
  
  // Free old buffer if we own it
  if (sect->mode != COIL_SECT_MODE_VIEW && sect->data != NULL) {
    coil_free(sect->data);
  }
  
  // Update section
  sect->data = new_data;
  sect->capacity = new_capacity;
  sect->mode = COIL_SECT_MODE_MODIFY; // Now we own the data
  
  return COIL_ERR_GOOD;
}

/**
* @brief Write into section data from user provided buffer
*/
coil_err_t coil_section_write(coil_section_t *sect, coil_byte_t *buf, 
                           coil_size_t bufsize, coil_size_t *byteswritten) {
  if (sect == NULL || buf == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Check if section is writable
  if (sect->mode == COIL_SECT_MODE_VIEW) {
    return COIL_ERROR(COIL_ERR_BADSTATE, "Cannot write to section in VIEW mode");
  }
  
  // Ensure we have enough capacity
  coil_err_t err = coil_section_ensure_capacity(sect, sect->windex + bufsize);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Write data
  coil_memcpy(sect->data + sect->windex, buf, bufsize);
  sect->windex += bufsize;
  
  // Update size if write position exceeds current size
  if (sect->windex > sect->size) {
    sect->size = sect->windex;
  }
  
  if (byteswritten != NULL) {
    *byteswritten = bufsize;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Read from section data into user provided buffer
*/
coil_err_t coil_section_read(coil_section_t *sect, coil_byte_t *buf, 
                          coil_size_t bufsize, coil_size_t *bytesread) {
  if (sect == NULL || buf == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Calculate how many bytes we can actually read
  coil_size_t readable = sect->size - sect->rindex;
  coil_size_t to_read = (readable < bufsize) ? readable : bufsize;
  
  // Read data
  if (to_read > 0) {
    coil_memcpy(buf, sect->data + sect->rindex, to_read);
    sect->rindex += to_read;
  }
  
  if (bytesread != NULL) {
    *bytesread = to_read;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Write a C-style string into section data
*/
coil_err_t coil_section_putstr(coil_section_t *sect, const char *str) {
  if (sect == NULL || str == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  coil_size_t len = strlen(str) + 1; // Include null terminator
  return coil_section_write(sect, (coil_byte_t *)str, len, NULL);
}

/**
* @brief Get a string from section data at a specific offset
*/
coil_err_t coil_section_getstr(coil_section_t *sect, coil_u64_t offset, const char **str) {
  if (sect == NULL || str == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Check if offset is valid
  if (offset >= sect->size) {
    return COIL_ERROR(COIL_ERR_INVAL, "Offset out of bounds");
  }
  
  // Set the string pointer
  *str = (const char *)(sect->data + offset);
  
  return COIL_ERR_GOOD;
}

/**
* @brief Shrink section buffer to fit current content
*/
coil_err_t coil_section_compact(coil_section_t *sect) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // If we're in VIEW mode or have no data, we can't/shouldn't compact
  if (sect->mode == COIL_SECT_MODE_VIEW || sect->data == NULL) {
    return COIL_ERROR(COIL_ERR_BADSTATE, "Cannot compact section in VIEW mode");
  }
  
  // If size matches capacity, nothing to do
  if (sect->size == sect->capacity) {
    return COIL_ERR_GOOD;
  }
  
  // Resize to exactly match content size
  return coil_section_resize(sect, sect->size);
}

/**
* @brief Reset section read/write indices
*/
coil_err_t coil_section_reset(coil_section_t *sect) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  sect->rindex = 0;
  sect->windex = 0;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Seek to a specific position for reading
*/
coil_err_t coil_section_seek_read(coil_section_t *sect, coil_size_t pos) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  if (pos > sect->size) {
    return COIL_ERROR(COIL_ERR_INVAL, "Position exceeds section size");
  }
  
  sect->rindex = pos;
  return COIL_ERR_GOOD;
}

/**
* @brief Seek to a specific position for writing
*/
coil_err_t coil_section_seek_write(coil_section_t *sect, coil_size_t pos) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  if (pos > sect->size) {
    return COIL_ERROR(COIL_ERR_INVAL, "Position exceeds section size");
  }
  
  sect->windex = pos;
  return COIL_ERR_GOOD;
}

/**
* @brief Load coil section from descriptor (copied)
*/
coil_err_t coil_section_load(coil_section_t *sect, coil_size_t capacity, coil_descriptor_t fd) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // Use a reasonable minimum capacity
  if (capacity == 0) {
    capacity = 1024;
  }
  
  // Initialize the section
  coil_err_t err = coil_section_init(sect, capacity);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Set mode to MODIFY since we're loading an existing section
  sect->mode = COIL_SECT_MODE_MODIFY;
  
  // Read data from file
  coil_size_t bytesread;
  err = coil_read(fd, sect->data, capacity, &bytesread);
  if (err != COIL_ERR_GOOD) {
    coil_section_cleanup(sect);
    return COIL_ERROR(COIL_ERR_IO, "Failed to read section data");
  }
  
  // Update section size
  sect->size = bytesread;
  
  // Reset read/write indices
  sect->rindex = 0;
  sect->windex = bytesread;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Load coil section view (memory mapped)
*/
coil_err_t coil_section_loadv(coil_section_t *sect, coil_size_t capacity, coil_descriptor_t fd) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // Get current file position
  off_t current_pos = lseek(fd, 0, SEEK_CUR);
  if (current_pos == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to get current file position");
  }
  
  // Get file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to determine file size");
  }
  
  // Calculate remaining bytes in file
  coil_size_t remaining = file_size - current_pos;
  
  // Use specified capacity or remaining bytes
  coil_size_t map_size = (capacity > 0 && capacity < remaining) ? capacity : remaining;
  
  // For mmap to work correctly with offsets, we need to align to page boundaries
  coil_size_t page_size = coil_get_page_size();
  off_t page_offset = current_pos % page_size;
  off_t aligned_offset = current_pos - page_offset;
  coil_size_t aligned_size = map_size + page_offset;
  
  // Return to original position
  if (lseek(fd, current_pos, SEEK_SET) == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to seek back to original position");
  }
  
  // Map the section data with page alignment
  void *mapped_data = mmap(NULL, aligned_size, PROT_READ, MAP_PRIVATE, fd, aligned_offset);
  if (mapped_data == MAP_FAILED) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to memory map section data");
  }
  
  // Initialize section
  coil_memset(sect, 0, sizeof(coil_section_t));
  sect->data = (coil_byte_t *)mapped_data + page_offset; // Adjust for page alignment
  sect->size = map_size;
  sect->capacity = map_size;
  sect->mode = COIL_SECT_MODE_VIEW;
  
  // Track memory mapping information
  sect->is_mapped = 1;
  sect->map_size = aligned_size;
  sect->map_base = mapped_data;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Serialize a section to an object file
*/
coil_err_t coil_section_serialize(coil_section_t *sect, coil_descriptor_t fd) {
  if (sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
  }
  
  // Handle empty sections gracefully
  if (sect->data == NULL || sect->size == 0) {
    return COIL_ERR_GOOD;  // Nothing to write
  }
  
  coil_size_t byteswritten;
  coil_err_t err = coil_write(fd, sect->data, sect->size, &byteswritten);
  
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  if (byteswritten != sect->size) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to write all section data");
  }
  
  return COIL_ERR_GOOD;
}