#include <coil/section.h>
#include <coil/err.h>
#include <coil/memory.h>
#include <coil/obj.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Initialize coil section with its own memory
 *
 * @param sect Pointer to section to populate
 * @param capacity The beginning capacity
 * @param mode Define the operations on this section
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_init(coil_section_t *sect, coil_size_t capacity, int mode) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (capacity == 0) {
        capacity = 128; // Default initial capacity
    }
    
    // Clear the section structure
    memset(sect, 0, sizeof(coil_section_t));
    
    // Allocate memory for the section data
    if (capacity > 0) {
        sect->data = (coil_byte_t*)malloc(capacity);
        if (sect->data == NULL) {
            return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate section data buffer");
        }
    }
    
    // Initialize section properties
    sect->capacity = capacity;
    sect->size = 0;
    sect->rindex = 0;
    sect->windex = 0;
    sect->mode = mode;
    sect->ownership = COIL_SECT_OWN_SELF; // We own this memory
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Initialize coil section with object memory
 * 
 * @param sect Pointer to section to populate
 * @param byte Pointer to memory that can be used by the section
 * @param capacity The max memory allocated to this section
 * @param mode Define the operations on this section
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_init_view(coil_section_t *sect, coil_byte_t *byte, coil_size_t capacity, int mode) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (byte == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Memory pointer is NULL");
    }
    
    if (capacity == 0) {
        return COIL_ERROR(COIL_ERR_INVAL, "Capacity cannot be zero for view mode");
    }
    
    // Clear the section structure
    memset(sect, 0, sizeof(coil_section_t));
    
    // Initialize section with the provided memory
    sect->data = byte;
    sect->capacity = capacity;
    sect->size = capacity; // In view mode, size is the same as capacity
    sect->rindex = 0;
    sect->windex = 0;
    sect->mode = mode | COIL_SECT_MODE_O; // Always set the Owned bit
    sect->ownership = COIL_SECT_OWN_NONE; // We don't own this memory
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Clean up section resources
 * 
 * @param sect Pointer to section
 */
void coil_section_cleanup(coil_section_t *sect) {
    if (sect == NULL) {
        return;
    }
    
    // Free data if we own it
    if (sect->data != NULL && sect->ownership == COIL_SECT_OWN_SELF) {
        free(sect->data);
    }
    
    // Clear the section structure
    memset(sect, 0, sizeof(coil_section_t));
}

/**
 * @brief Ensure section has at least specified capacity
 * 
 * @param sect Section to resize
 * @param min_capacity Minimum capacity needed
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_ensure_capacity(coil_section_t *sect, coil_size_t min_capacity) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    // Check if we already have enough capacity
    if (sect->capacity >= min_capacity) {
        return COIL_ERR_GOOD;
    }
    
    // Check if we're allowed to resize
    if (sect->mode & COIL_SECT_MODE_O) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Cannot resize externally owned section");
    }
    
    // Calculate new capacity (double current size or use min_capacity if larger)
    coil_size_t new_capacity = sect->capacity * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    // Allocate new buffer
    coil_byte_t *new_data = (coil_byte_t*)realloc(sect->data, new_capacity);
    if (new_data == NULL) {
        return COIL_ERROR(COIL_ERR_NOMEM, "Failed to resize section data buffer");
    }
    
    // Update section properties
    sect->data = new_data;
    sect->capacity = new_capacity;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Shrink section buffer to fit current content
 * 
 * @param sect Section to compact
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_compact(coil_section_t *sect) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    // Check if we're allowed to resize
    if (sect->ownership != COIL_SECT_OWN_SELF) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Cannot compact non-owned section");
    }
    
    // Check if compaction is needed
    if (sect->capacity == sect->size || sect->size == 0) {
        return COIL_ERR_GOOD;
    }
    
    // Resize buffer to exactly fit the data
    coil_byte_t *new_data = (coil_byte_t*)realloc(sect->data, sect->size);
    if (new_data == NULL) {
        // This is not a critical error - we can continue using the larger buffer
        return COIL_WARNING(COIL_ERR_NOMEM, "Failed to compact section buffer");
    }
    
    // Update section properties
    sect->data = new_data;
    sect->capacity = sect->size;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Reset section read/write indices
 * 
 * @param sect Section to reset
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_reset(coil_section_t *sect) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    sect->rindex = 0;
    sect->windex = 0;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Seek to a specific position for reading
 * 
 * @param sect Section to operate on
 * @param pos Position to seek to
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_seek_read(coil_section_t *sect, coil_size_t pos) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (pos > sect->size) {
        return COIL_ERROR(COIL_ERR_INVAL, "Read position beyond section size");
    }
    
    sect->rindex = pos;
    return COIL_ERR_GOOD;
}

/**
 * @brief Seek to a specific position for writing
 * 
 * @param sect Section to operate on
 * @param pos Position to seek to
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_seek_write(coil_section_t *sect, coil_size_t pos) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (pos > sect->size) {
        return COIL_ERROR(COIL_ERR_INVAL, "Write position beyond section size");
    }
    
    sect->windex = pos;
    return COIL_ERR_GOOD;
}

/**
 * @brief Write into section data from user provided buffer
 *
 * @param sect Pointer to section
 * @param buf Pointer to buffer
 * @param bufsize Size of user buffer
 * @param byteswritten Pointer to size type to be populated with bytes written
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_write(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *byteswritten) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (buf == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Buffer pointer is NULL");
    }
    
    if (!(sect->mode & COIL_SECT_MODE_W)) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Section not opened for writing");
    }
    
    // Initialize bytes written
    if (byteswritten) {
        *byteswritten = 0;
    }
    
    // Check if we have enough capacity
    if (sect->windex + bufsize > sect->capacity) {
        // Try to resize if we own the memory
        if (sect->ownership == COIL_SECT_OWN_SELF) {
            coil_err_t err = coil_section_ensure_capacity(sect, sect->windex + bufsize);
            if (err != COIL_ERR_GOOD) {
                return err;
            }
        } else {
            // Can't resize, so just write what we can
            bufsize = sect->capacity - sect->windex;
            if (bufsize == 0) {
                return COIL_ERROR(COIL_ERR_NOMEM, "Section buffer full and cannot resize");
            }
        }
    }
    
    // Copy data to section buffer
    memcpy(sect->data + sect->windex, buf, bufsize);
    sect->windex += bufsize;
    
    // Update section size if needed
    if (sect->windex > sect->size) {
        sect->size = sect->windex;
    }
    
    // Update bytes written
    if (byteswritten) {
        *byteswritten = bufsize;
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Read from section data into user provided buffer
 *
 * @param sect Pointer to section
 * @param buf Pointer to buffer
 * @param bufsize Size of user buffer
 * @param bytesread Pointer to size type to be populated with bytes read
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_read(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *bytesread) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (buf == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Buffer pointer is NULL");
    }
    
    if (!(sect->mode & COIL_SECT_MODE_R)) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Section not opened for reading");
    }
    
    // Initialize bytes read
    if (bytesread) {
        *bytesread = 0;
    }
    
    // Calculate how much we can read
    coil_size_t bytes_available = sect->size - sect->rindex;
    coil_size_t bytes_to_read = (bufsize < bytes_available) ? bufsize : bytes_available;
    
    // Copy data from section buffer
    if (bytes_to_read > 0) {
        memcpy(buf, sect->data + sect->rindex, bytes_to_read);
        sect->rindex += bytes_to_read;
        
        // Update bytes read
        if (bytesread) {
            *bytesread = bytes_to_read;
        }
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Write a C-style string into section data
 *
 * @param sect Pointer to section
 * @param str Null-terminated string to append
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_putstr(coil_section_t *sect, const char *str) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (str == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "String pointer is NULL");
    }
    
    // Calculate string length (including null terminator)
    coil_size_t len = strlen(str) + 1;
    
    // Write string to section
    coil_size_t bytes_written;
    return coil_section_write(sect, (coil_byte_t*)str, len, &bytes_written);
}

/**
 * @brief Get a string from section data at a specific offset
 *
 * @param sect Pointer to section
 * @param offset Offset into section the string is located at
 * @param str Pointer to a pointer to be set to a location in the section buffer
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_getstr(coil_section_t *sect, coil_u64_t offset, const char **str) {
    // Validate parameters
    if (sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Section pointer is NULL");
    }
    
    if (str == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "String pointer is NULL");
    }
    
    if (!(sect->mode & COIL_SECT_MODE_R)) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Section not opened for reading");
    }
    
    // Check if offset is valid
    if (offset >= sect->size) {
        return COIL_ERROR(COIL_ERR_INVAL, "Offset beyond section size");
    }
    
    // Get string from section buffer
    *str = (const char*)(sect->data + offset);
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Deserialize a section from an object file
 * 
 * @param obj Object containing section header information
 * @param fd File descriptor for reading
 * @param index Section index
 * @param sect Section structure to populate
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_deserialize(coil_object_t *obj, int fd, coil_u16_t index, coil_section_t *sect) {
    // Validate parameters
    if (obj == NULL || sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    if (fd < 0) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor");
    }
    
    if (index >= obj->header.section_count) {
        return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
    }
    
    // Get section header
    coil_section_header_t *header = &obj->sectheaders[index];
    
    // Initialize section
    coil_err_t err = coil_section_init(sect, header->size, COIL_SECT_MODE_R | COIL_SECT_MODE_W);
    if (err != COIL_ERR_GOOD) {
        return err;
    }
    
    // Copy the section name
    sect->name = header->name;
    
    // If section has data, read it from file
    if (header->size > 0 && header->type != COIL_SECTION_NOBITS) {
        // Seek to section data
        if (lseek(fd, header->offset, SEEK_SET) == -1) {
            coil_section_cleanup(sect);
            return COIL_ERROR(COIL_ERR_IO, "Failed to seek to section data");
        }
        
        // Read section data
        ssize_t bytes_read = read(fd, sect->data, header->size);
        if (bytes_read < 0 || (coil_size_t)bytes_read != header->size) {
            coil_section_cleanup(sect);
            return COIL_ERROR(COIL_ERR_IO, "Failed to read section data");
        }
        
        // Update section size
        sect->size = header->size;
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Serialize a section to an object file
 * 
 * @param obj Object to update with section information
 * @param fd File descriptor for writing
 * @param sect Section to serialize
 * @param index Section index to use
 * @return coil_err_t COIL_ERR_GOOD on success
 */
coil_err_t coil_section_serialize(coil_object_t *obj, int fd, coil_section_t *sect, coil_u16_t index) {
    // Validate parameters
    if (obj == NULL || sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    if (fd < 0) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor");
    }
    
    if (index >= obj->header.section_count) {
        return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
    }
    
    // Get section header
    coil_section_header_t *header = &obj->sectheaders[index];
    
    // Update header with section information
    header->size = sect->size;
    header->name = sect->name;
    
    // If section has data, write it to file
    if (sect->size > 0 && header->type != COIL_SECTION_NOBITS) {
        // Seek to section data offset
        if (lseek(fd, header->offset, SEEK_SET) == -1) {
            return COIL_ERROR(COIL_ERR_IO, "Failed to seek to section data offset");
        }
        
        // Write section data
        ssize_t bytes_written = write(fd, sect->data, sect->size);
        if (bytes_written < 0 || (coil_size_t)bytes_written != sect->size) {
            return COIL_ERROR(COIL_ERR_IO, "Failed to write section data");
        }
    }
    
    return COIL_ERR_GOOD;
}