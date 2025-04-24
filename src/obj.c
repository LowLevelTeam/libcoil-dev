#include <coil/obj.h>
#include <coil/err.h>
#include <coil/memory.h>
#include <coil/types.h>
#include <coil/section.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// Current COIL format version
#define COIL_CURRENT_VERSION 1

// Default magic bytes for COIL files
static const coil_u8_t COIL_MAGIC[] = COIL_MAGIC_BYTES;

/**
 * @brief Calculate a hash for section names
 * 
 * Simple FNV-1a hash algorithm for strings
 * 
 * @param name String to hash
 * @return coil_u64_t Hash value
 */
coil_u64_t coil_obj_hash_name(const char *name) {
    if (name == NULL) {
        return 0;
    }
    
    // FNV-1a hash parameters for 64-bit hash
    const coil_u64_t FNV_PRIME = 1099511628211ULL;
    const coil_u64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    
    coil_u64_t hash = FNV_OFFSET_BASIS;
    
    // Process each byte of the string
    for (const char *p = name; *p != '\0'; p++) {
        hash ^= (coil_u64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    
    return hash;
}

/**
 * @brief Initialize a COIL object
 * 
 * @param obj Pointer to object structure to initialize
 * @param flags Initialization flags
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_init(coil_object_t *obj, int flags) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    // Clear the object structure
    memset(obj, 0, sizeof(coil_object_t));
    
    // Set default values
    obj->fd = -1;
    obj->is_mapped = 0;
    
    // If creating a new object, set up the header
    if (flags & COIL_OBJ_INIT_EMPTY) {
        // Set magic number
        memcpy(obj->header.magic, COIL_MAGIC, sizeof(obj->header.magic));
        
        // Set version
        obj->header.version = COIL_CURRENT_VERSION;
        
        // Initial values
        obj->header.section_count = 0;
        obj->header.file_size = sizeof(coil_object_header_t);
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Clean up a COIL object and free associated resources
 * 
 * @param obj Object to clean up
 */
void coil_obj_cleanup(coil_object_t *obj) {
    if (obj == NULL) {
        return;
    }
    
    // Unmap memory if mapped
    if (obj->is_mapped && obj->memory != NULL) {
        munmap(obj->memory, obj->header.file_size);
    } else {
        // Otherwise free allocated memory
        if (obj->memory != NULL && !obj->is_mapped) {
            free(obj->memory);
        }
        
        if (obj->sectheaders != NULL) {
            free(obj->sectheaders);
        }
    }
    
    // Clean up loaded sections
    if (obj->sections != NULL) {
        for (coil_u16_t i = 0; i < obj->loaded_count; i++) {
            if (obj->sections[i].data != NULL) {
                coil_section_cleanup(&obj->sections[i]);
            }
        }
        free(obj->sections);
    }
    
    // Close file descriptor if open
    if (obj->fd >= 0) {
        close(obj->fd);
    }
    
    // Clear the object structure
    memset(obj, 0, sizeof(coil_object_t));
    obj->fd = -1;
}

/**
 * @brief Load object from file using normal file I/O
 * 
 * @param obj Object to populate
 * @param filepath Path to the file to load
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_load_file(coil_object_t *obj, const char *filepath) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    if (filepath == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "File path is NULL");
    }
    
    // Clean up any existing data
    coil_obj_cleanup(obj);
    
    // Initialize object
    coil_err_t err = coil_obj_init(obj, 0);
    if (err != COIL_ERR_GOOD) {
        return err;
    }
    
    // Open the file
    obj->fd = open(filepath, O_RDONLY);
    if (obj->fd < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to open file: %s", strerror(errno));
        return COIL_ERROR(COIL_ERR_IO, error_msg);
    }
    
    // Get file size
    struct stat st;
    if (fstat(obj->fd, &st) < 0) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_IO, "Failed to get file size");
    }
    
    // Check if file is too small to be a valid COIL file
    if (st.st_size < (off_t)sizeof(coil_object_header_t)) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_FORMAT, "File too small to be a valid COIL file");
    }
    
    // Read object header
    ssize_t bytes_read = read(obj->fd, &obj->header, sizeof(coil_object_header_t));
    if (bytes_read != sizeof(coil_object_header_t)) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_IO, "Failed to read object header");
    }
    
    // Verify magic number
    if (memcmp(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC)) != 0) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_FORMAT, "Invalid magic number");
    }
    
    // Verify file size
    if (obj->header.file_size != (coil_u64_t)st.st_size) {
        // Allow size mismatch but log warning
        COIL_WARNING(COIL_ERR_FORMAT, "File size mismatch");
    }
    
    // Allocate memory for section headers
    if (obj->header.section_count > 0) {
        size_t sectheaders_size = obj->header.section_count * sizeof(coil_section_header_t);
        obj->sectheaders = (coil_section_header_t*)malloc(sectheaders_size);
        if (obj->sectheaders == NULL) {
            close(obj->fd);
            obj->fd = -1;
            return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
        }
        
        // Read section headers
        bytes_read = read(obj->fd, obj->sectheaders, sectheaders_size);
        if (bytes_read != (ssize_t)sectheaders_size) {
            free(obj->sectheaders);
            obj->sectheaders = NULL;
            close(obj->fd);
            obj->fd = -1;
            return COIL_ERROR(COIL_ERR_IO, "Failed to read section headers");
        }
    }
    
    // Allocate memory for section array (to be lazily loaded)
    obj->sections = NULL;
    obj->loaded_count = 0;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Load object from file using memory mapping
 * 
 * @param obj Object to populate
 * @param filepath Path to the file to map
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_mmap(coil_object_t *obj, const char *filepath) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    if (filepath == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "File path is NULL");
    }
    
    // Initialize object
    coil_err_t err = coil_obj_init(obj, 0);
    if (err != COIL_ERR_GOOD) {
        return err;
    }
    
    // Open the file
    obj->fd = open(filepath, O_RDONLY);
    if (obj->fd < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to open file: %s", strerror(errno));
        return COIL_ERROR(COIL_ERR_IO, error_msg);
    }
    
    // Get file size
    struct stat st;
    if (fstat(obj->fd, &st) < 0) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_IO, "Failed to get file size");
    }
    
    // Check if file is too small to be a valid COIL file
    if (st.st_size < (off_t)sizeof(coil_object_header_t)) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_FORMAT, "File too small to be a valid COIL file");
    }
    
    // Memory map the entire file
    obj->memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, obj->fd, 0);
    if (obj->memory == MAP_FAILED) {
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_NOMEM, "Failed to memory map file");
    }
    
    obj->is_mapped = 1;
    
    // Set up header from mapped memory
    memcpy(&obj->header, obj->memory, sizeof(coil_object_header_t));
    
    // Verify magic number
    if (memcmp(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC)) != 0) {
        munmap(obj->memory, st.st_size);
        obj->memory = NULL;
        obj->is_mapped = 0;
        close(obj->fd);
        obj->fd = -1;
        return COIL_ERROR(COIL_ERR_FORMAT, "Invalid magic number");
    }
    
    // Point section headers to mapped memory
    if (obj->header.section_count > 0) {
        obj->sectheaders = (coil_section_header_t*)(obj->memory + sizeof(coil_object_header_t));
    }
    
    // Allocate memory for section array (to be lazily loaded)
    obj->sections = NULL;
    obj->loaded_count = 0;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Unmap a previously memory-mapped object
 * 
 * @param obj Object to unmap
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_munmap(coil_object_t *obj) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    // Check if object is memory mapped
    if (!obj->is_mapped || obj->memory == NULL) {
        return COIL_ERROR(COIL_ERR_BADSTATE, "Object is not memory mapped");
    }
    
    // Unmap the memory
    if (munmap(obj->memory, obj->header.file_size) != 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to unmap memory: %s", strerror(errno));
        return COIL_ERROR(COIL_ERR_IO, error_msg);
    }
    
    // Clear memory pointer and flags
    obj->memory = NULL;
    obj->is_mapped = 0;
    
    // Section headers are now invalid
    obj->sectheaders = NULL;
    
    // Close file descriptor if open
    if (obj->fd >= 0) {
        close(obj->fd);
        obj->fd = -1;
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Save object to file
 * 
 * @param obj Object to save
 * @param filepath Path to the file to create or overwrite
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_save_file(coil_object_t *obj, const char *filepath) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    if (filepath == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "File path is NULL");
    }
    
    // Open or create the file
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to create file: %s", strerror(errno));
        return COIL_ERROR(COIL_ERR_IO, error_msg);
    }
    
    // Calculate file size based on header, section headers, and section data
    coil_u64_t file_size = sizeof(coil_object_header_t);
    file_size += obj->header.section_count * sizeof(coil_section_header_t);
    
    // Update section offsets and add section sizes
    for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
        // Set section offset
        obj->sectheaders[i].offset = file_size;
        
        // Add section size to file size
        file_size += obj->sectheaders[i].size;
    }
    
    // Update file size in header
    obj->header.file_size = file_size;
    
    // Write object header
    ssize_t bytes_written = write(fd, &obj->header, sizeof(coil_object_header_t));
    if (bytes_written != sizeof(coil_object_header_t)) {
        close(fd);
        return COIL_ERROR(COIL_ERR_IO, "Failed to write object header");
    }
    
    // Write section headers if any
    if (obj->header.section_count > 0) {
        size_t sectheaders_size = obj->header.section_count * sizeof(coil_section_header_t);
        bytes_written = write(fd, obj->sectheaders, sectheaders_size);
        if (bytes_written != (ssize_t)sectheaders_size) {
            close(fd);
            return COIL_ERROR(COIL_ERR_IO, "Failed to write section headers");
        }
    }
    
    // Write section data for each loaded section
    for (coil_u16_t i = 0; i < obj->loaded_count; i++) {
        coil_section_t *sect = &obj->sections[i];
        if (sect->data != NULL && sect->size > 0) {
            // Seek to section offset
            if (lseek(fd, obj->sectheaders[i].offset, SEEK_SET) == -1) {
                close(fd);
                return COIL_ERROR(COIL_ERR_IO, "Failed to seek to section offset");
            }
            
            // Write section data
            bytes_written = write(fd, sect->data, sect->size);
            if (bytes_written != (ssize_t)sect->size) {
                close(fd);
                return COIL_ERROR(COIL_ERR_IO, "Failed to write section data");
            }
        }
    }
    
    // Close the file
    close(fd);
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Initialize the section array for lazy loading
 * 
 * @param obj Object to initialize
 * @return COIL_ERR_GOOD on success
 */
static coil_err_t coil_obj_init_sections_array(coil_object_t *obj) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    // Check if already initialized
    if (obj->sections != NULL) {
        return COIL_ERR_GOOD;
    }
    
    // No sections to load
    if (obj->header.section_count == 0) {
        return COIL_ERR_GOOD;
    }
    
    // Allocate memory for section array
    obj->sections = (coil_section_t*)calloc(obj->header.section_count, sizeof(coil_section_t));
    if (obj->sections == NULL) {
        return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section array");
    }
    
    obj->loaded_count = 0;
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Load a section by index
 * 
 * @param obj Object containing the section
 * @param index Section index
 * @param sect Pointer to section structure to populate
 * @param mode Section access mode and loading mode
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_load_section(coil_object_t *obj, coil_u16_t index, coil_section_t *sect, int mode) {
    // Validate parameters
    if (obj == NULL || sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    if (index >= obj->header.section_count) {
        return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
    }
    
    // Get section header
    coil_section_header_t *header = &obj->sectheaders[index];
    
    // Initialize section structure
    memset(sect, 0, sizeof(coil_section_t));
    
    // Copy section name
    sect->name = header->name;
    
    // For memory-mapped files, we can use view mode for read-only access
    if (obj->is_mapped && (mode & COIL_SLOAD_VIEW) && !(mode & COIL_SECT_MODE_W)) {
        // Use direct pointer to memory-mapped data
        return coil_section_init_view(sect, 
                                     obj->memory + header->offset, 
                                     header->size, 
                                     mode | COIL_SECT_MODE_R); // Force read mode
    }
    
    // For normal file I/O or when writing is needed, initialize with own memory
    coil_err_t err = coil_section_init(sect, header->size, mode);
    if (err != COIL_ERR_GOOD) {
        return err;
    }
    
    // Load section data if not a NOBITS section
    if (header->size > 0 && header->type != COIL_SECTION_NOBITS) {
        if (obj->is_mapped) {
            // Copy from memory-mapped file
            memcpy(sect->data, obj->memory + header->offset, header->size);
        } else if (obj->fd >= 0) {
            // Seek to section data
            if (lseek(obj->fd, header->offset, SEEK_SET) == -1) {
                coil_section_cleanup(sect);
                return COIL_ERROR(COIL_ERR_IO, "Failed to seek to section data");
            }
            
            // Read section data
            ssize_t bytes_read = read(obj->fd, sect->data, header->size);
            if (bytes_read < 0 || (coil_size_t)bytes_read != header->size) {
                coil_section_cleanup(sect);
                return COIL_ERROR(COIL_ERR_IO, "Failed to read section data");
            }
        } else {
            coil_section_cleanup(sect);
            return COIL_ERROR(COIL_ERR_BADSTATE, "No file descriptor or memory mapping available");
        }
        
        // Update section size
        sect->size = header->size;
    }
    
    // Initialize section array if needed
    if (obj->sections == NULL) {
        err = coil_obj_init_sections_array(obj);
        if (err != COIL_ERR_GOOD) {
            coil_section_cleanup(sect);
            return err;
        }
    }
    
    // Store in section array for tracking
    if (obj->sections != NULL && index < obj->header.section_count) {
        // If this section was already loaded, clean it up first
        if (obj->sections[index].data != NULL) {
            coil_section_cleanup(&obj->sections[index]);
        } else {
            obj->loaded_count++;
        }
        
        // Make a copy of the section
        memcpy(&obj->sections[index], sect, sizeof(coil_section_t));
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Create a new section in the object
 * 
 * @param obj Object to add section to
 * @param type Section type
 * @param name Section name
 * @param flags Section flags
 * @param sect Pre-initialized section to copy data from (can be NULL for empty section)
 * @param index Pointer to store the new section index
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_create_section(coil_object_t *obj, coil_u8_t type, const char *name, 
                                  coil_u16_t flags, coil_section_t *sect, coil_u16_t *index) {
    // Validate parameters
    if (obj == NULL || name == NULL || index == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    // Calculate name hash
    coil_u64_t name_hash = coil_obj_hash_name(name);
    
    // Check if section already exists
    coil_u16_t existing_index;
    if (coil_obj_find_section_by_hash(obj, name_hash, &existing_index) == COIL_ERR_GOOD) {
        return COIL_ERROR(COIL_ERR_EXISTS, "Section with this name already exists");
    }
    
    // Resize section headers array
    coil_section_header_t *new_headers = (coil_section_header_t*)realloc(
        obj->sectheaders, 
        (obj->header.section_count + 1) * sizeof(coil_section_header_t)
    );
    
    if (new_headers == NULL) {
        return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
    }
    
    obj->sectheaders = new_headers;
    
    // Initialize new section header
    coil_section_header_t *header = &obj->sectheaders[obj->header.section_count];
    header->name = name_hash;
    header->size = (sect != NULL) ? sect->size : 0;
    header->offset = 0; // Will be set during save
    header->flags = flags;
    header->type = type;
    
    // Store the new section index
    *index = obj->header.section_count;
    
    // Increment section count
    obj->header.section_count++;
    
    // If section data provided, add to sections array
    if (sect != NULL) {
        // Initialize section array if needed
        coil_err_t err = coil_obj_init_sections_array(obj);
        if (err != COIL_ERR_GOOD) {
            return err;
        }
        
        // Resize sections array
        coil_section_t *new_sections = (coil_section_t*)realloc(
            obj->sections, 
            obj->header.section_count * sizeof(coil_section_t)
        );
        
        if (new_sections == NULL) {
            return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for sections array");
        }
        
        obj->sections = new_sections;
        
        // Copy section data
        memcpy(&obj->sections[*index], sect, sizeof(coil_section_t));
        obj->loaded_count++;
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Delete a section from the object
 * 
 * @param obj Object containing the section
 * @param index Section index to delete
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_delete_section(coil_object_t *obj, coil_u16_t index) {
    // Validate parameters
    if (obj == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
    }
    
    if (index >= obj->header.section_count) {
        return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
    }
    
    // Clean up the section if it's loaded
    if (obj->sections != NULL && index < obj->loaded_count) {
        coil_section_cleanup(&obj->sections[index]);
    }
    
    // Shift section headers down
    if (index < obj->header.section_count - 1) {
        memmove(
            &obj->sectheaders[index], 
            &obj->sectheaders[index + 1], 
            (obj->header.section_count - index - 1) * sizeof(coil_section_header_t)
        );
        
        // Also shift loaded sections if they exist
        if (obj->sections != NULL) {
            memmove(
                &obj->sections[index], 
                &obj->sections[index + 1], 
                (obj->header.section_count - index - 1) * sizeof(coil_section_t)
            );
        }
    }
    
    // Decrement section count
    obj->header.section_count--;
    
    // Update loaded count
    if (obj->loaded_count > 0) {
        obj->loaded_count--;
    }
    
    return COIL_ERR_GOOD;
}

/**
 * @brief Find a section by name
 * 
 * @param obj Object to search
 * @param name Section name to find
 * @param index Pointer to store the found section index
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_find_section(coil_object_t *obj, const char *name, coil_u16_t *index) {
    // Validate parameters
    if (obj == NULL || name == NULL || index == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    // Calculate name hash
    coil_u64_t name_hash = coil_obj_hash_name(name);
    
    // Find the section by hash
    return coil_obj_find_section_by_hash(obj, name_hash, index);
}

/**
 * @brief Find a section by name hash
 * 
 * @param obj Object to search
 * @param name_hash Hash of the section name to find
 * @param index Pointer to store the found section index
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_find_section_by_hash(coil_object_t *obj, coil_u64_t name_hash, coil_u16_t *index) {
    // Validate parameters
    if (obj == NULL || index == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    // Search for section with matching name hash
    for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
        if (obj->sectheaders[i].name == name_hash) {
            *index = i;
            return COIL_ERR_GOOD;
        }
    }
    
    // Silent Fail
    return COIL_ERR_NOTFOUND;
}

/**
 * @brief Update a section in the object
 * 
 * @param obj Object containing the section
 * @param index Section index to update
 * @param sect Section data to update with
 * @return COIL_ERR_GOOD on success
 */
coil_err_t coil_obj_update_section(coil_object_t *obj, coil_u16_t index, coil_section_t *sect) {
    // Validate parameters
    if (obj == NULL || sect == NULL) {
        return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
    }
    
    if (index >= obj->header.section_count) {
        return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
    }
    
    // Update section header with new size
    obj->sectheaders[index].size = sect->size;
    
    // Initialize section array if needed
    if (obj->sections == NULL) {
        coil_err_t err = coil_obj_init_sections_array(obj);
        if (err != COIL_ERR_GOOD) {
            return err;
        }
    }
    
    // Update or add to sections array
    if (obj->sections != NULL) {
        // If this section was already loaded, clean it up first
        if (obj->sections[index].data != NULL) {
            coil_section_cleanup(&obj->sections[index]);
        } else {
            obj->loaded_count++;
        }
        
        // Make a copy of the section
        memcpy(&obj->sections[index], sect, sizeof(coil_section_t));
    }
    
    return COIL_ERR_GOOD;
}