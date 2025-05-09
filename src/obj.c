/**
* @file obj.c
* @brief COIL Object functionality implementation for libcoil-dev
*/

#include <coil/base.h>
#include <coil/obj.h>
#include <coil/sect.h>
#include "srcdeps.h"

/**
* @brief COIL magic bytes for object files
*/
static const coil_u8_t COIL_MAGIC[4] = COIL_MAGIC_BYTES;

/**
* @brief Current object format version
*/
#define COIL_CURRENT_VERSION 1

/**
* @brief Initialize a COIL object
*/
coil_err_t coil_obj_init(coil_object_t *obj, int flags) {
  if (obj == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
  }
  
  // Zero initialize the object
  coil_memset(obj, 0, sizeof(coil_object_t));
  
  // Set magic, version and flags
  coil_memcpy(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC));
  obj->header.version = COIL_CURRENT_VERSION;
  obj->header.section_count = 0;
  obj->header.file_size = sizeof(coil_object_header_t);
  
  // Set file descriptor to invalid
  obj->fd = -1;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Clean up a COIL object and free associated resources
*/
void coil_obj_cleanup(coil_object_t *obj) {
  if (obj == NULL) {
    return;
  }
  
  // Free sections and their data
  if (obj->sections != NULL) {
    for (coil_u16_t i = 0; i < obj->loaded_count; i++) {
      coil_section_cleanup(obj->sections + i);
    }
    coil_free(obj->sections);
  }
  
  // Free section headers
  if (obj->sectheaders != NULL) {
    coil_free(obj->sectheaders);
  }
  
  // Unmap memory if mapped
  if (obj->is_mapped && obj->memory != NULL) {
    // Use the file size from the header for unmapping
    coil_munmap(obj->memory, obj->header.file_size);
    obj->memory = NULL;
    obj->is_mapped = 0;
  }
  
  // Close file descriptor if open
  if (obj->fd >= 0) {
    coil_close(obj->fd);
    obj->fd = -1;
  }
  
  // Reset object
  coil_memset(obj, 0, sizeof(coil_object_t));
  obj->fd = -1;
}

/**
* @brief Set the default target metadata for an object
*/
coil_err_t coil_obj_set_target_defaults(coil_object_t *obj, coil_pu_t pu, coil_u8_t arch, 
                                      coil_u64_t features) {
  if (obj == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
  }
  
  obj->default_pu = pu;
  obj->default_arch = arch;
  obj->default_features = features;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Load object from file using normal file I/O
*/
coil_err_t coil_obj_load_file(coil_object_t *obj, coil_descriptor_t fd) {
  if (obj == NULL || fd < 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid object pointer or file descriptor");
  }
  
  // Initialize object
  coil_obj_init(obj, COIL_OBJ_INIT_DEFAULT);
  
  // Save file descriptor
  obj->fd = fd;
  
  // Read header
  coil_size_t bytesread;
  coil_err_t err = coil_read(fd, (coil_byte_t *)&obj->header, 
                          sizeof(coil_object_header_t), &bytesread);
  
  if (err != COIL_ERR_GOOD || bytesread != sizeof(coil_object_header_t)) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to read object header");
  }
  
  // Verify magic
  if (coil_memcmp(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC)) != 0) {
    return COIL_ERROR(COIL_ERR_FORMAT, "Invalid object format (magic mismatch)");
  }
  
  // Read section headers if present
  if (obj->header.section_count > 0) {
    // Allocate memory for section headers
    coil_size_t headers_size = obj->header.section_count * sizeof(coil_section_header_t);
    obj->sectheaders = (coil_section_header_t *)coil_malloc(headers_size);
    
    if (obj->sectheaders == NULL) {
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
    }
    
    // Read section headers
    err = coil_read(fd, (coil_byte_t *)obj->sectheaders, headers_size, &bytesread);
    
    if (err != COIL_ERR_GOOD || bytesread != headers_size) {
      coil_free(obj->sectheaders);
      obj->sectheaders = NULL;
      return COIL_ERROR(COIL_ERR_IO, "Failed to read section headers");
    }
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Load object from file using memory mapping
*/
coil_err_t coil_obj_mmap(coil_object_t *obj, coil_descriptor_t fd) {
  if (obj == NULL || fd < 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid object pointer or file descriptor");
  }
  
  // Initialize object
  coil_obj_init(obj, COIL_OBJ_INIT_DEFAULT);
  
  // Save file descriptor
  obj->fd = fd;
  
  // Get file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to determine file size");
  }
  
  // Seek back to the beginning
  if (lseek(fd, 0, SEEK_SET) == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to seek to beginning of file");
  }
  
  // Map the entire file into memory
  void *mapped_memory = mmap(NULL, file_size, PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE, fd, 0);
  
  if (mapped_memory == MAP_FAILED) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to memory map file");
  }
  
  // Set mapping flag
  obj->is_mapped = 1;
  obj->memory = mapped_memory;
  
  // Read header from mapped memory
  coil_memcpy(&obj->header, obj->memory, sizeof(coil_object_header_t));
  
  // Verify magic
  if (coil_memcmp(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC)) != 0) {
    coil_munmap(obj->memory, file_size);
    obj->memory = NULL;
    obj->is_mapped = 0;
    return COIL_ERROR(COIL_ERR_FORMAT, "Invalid object format (magic mismatch)");
  }
  
  // Verify file size
  if (obj->header.file_size != (coil_u64_t)file_size) {
    coil_log(COIL_LEVEL_WARNING, "Actual file size (%lld) doesn't match header size (%lld)",
            (long long)file_size, (long long)obj->header.file_size);
    
    // Update file size in header to match actual size
    obj->header.file_size = file_size;
  }
  
  // Load section headers
  if (obj->header.section_count > 0) {
    coil_size_t headers_size = obj->header.section_count * sizeof(coil_section_header_t);
    coil_byte_t *headers_ptr = (coil_byte_t *)obj->memory + sizeof(coil_object_header_t);
    
    // Allocate memory for section headers (we'll copy them rather than using the mapped memory directly)
    obj->sectheaders = (coil_section_header_t *)coil_malloc(headers_size);
    if (obj->sectheaders == NULL) {
      coil_munmap(obj->memory, file_size);
      obj->memory = NULL;
      obj->is_mapped = 0;
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
    }
    
    // Copy section headers
    coil_memcpy(obj->sectheaders, headers_ptr, headers_size);
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Convert a memory-mapped object to a regular object
*
* This function copies all data from the memory-mapped object to a regular
* object with standard memory allocation. After conversion, the object
* is no longer tied to the file mapping.
*
* @param obj Object to convert
*
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
* @return COIL_ERR_NOMEM if memory allocation fails
*/
coil_err_t coil_obj_unmap(coil_object_t *obj) {
  if (obj == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
  }
  
  // If the object is not memory-mapped, there's nothing to do
  if (!obj->is_mapped || obj->memory == NULL) {
    return COIL_ERR_GOOD;
  }
  
  // Create a temporary object to hold the converted data
  coil_object_t temp_obj;
  coil_err_t err = coil_obj_init(&temp_obj, COIL_OBJ_INIT_DEFAULT);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Copy the header
  temp_obj.header = obj->header;
  
  // Copy section headers
  if (obj->header.section_count > 0) {
    temp_obj.sectheaders = (coil_section_header_t *)coil_malloc(
        obj->header.section_count * sizeof(coil_section_header_t));
    
    if (temp_obj.sectheaders == NULL) {
      coil_obj_cleanup(&temp_obj);
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
    }
    
    coil_memcpy(temp_obj.sectheaders, obj->sectheaders, 
               obj->header.section_count * sizeof(coil_section_header_t));
  }
  
  // Copy loaded sections
  if (obj->sections != NULL && obj->loaded_count > 0) {
    temp_obj.sections = (coil_section_t *)coil_calloc(
        obj->loaded_count, sizeof(coil_section_t));
    
    if (temp_obj.sections == NULL) {
      coil_obj_cleanup(&temp_obj);
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for sections");
    }
    
    temp_obj.loaded_count = obj->loaded_count;
    
    // Copy each section
    for (coil_u16_t i = 0; i < obj->loaded_count; i++) {
      coil_section_t *src = &obj->sections[i];
      coil_section_t *dst = &temp_obj.sections[i];
      
      if (src->data != NULL && src->size > 0) {
        err = coil_section_init(dst, src->size);
        if (err != COIL_ERR_GOOD) {
          coil_obj_cleanup(&temp_obj);
          return err;
        }
        
        // Copy section data
        coil_memcpy(dst->data, src->data, src->size);
        dst->size = src->size;

        // Copy metadata
        dst->name = src->name;
        dst->mode = COIL_SECT_MODE_MODIFY;  // Switch to modify mode
      }
    }
  }
  
  // Copy target defaults
  temp_obj.default_pu = obj->default_pu;
  temp_obj.default_arch = obj->default_arch;
  temp_obj.default_features = obj->default_features;
  
  // Preserve file descriptor
  temp_obj.fd = obj->fd;
  
  // Unmap the original object's memory
  coil_munmap(obj->memory, obj->header.file_size);
  
  // Replace the original object with the temporary one
  *obj = temp_obj;
  obj->is_mapped = 0;
  obj->memory = NULL;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Save object to file
*/
coil_err_t coil_obj_save_file(coil_object_t *obj, coil_descriptor_t fd) {
  if (obj == NULL || fd < 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // If the object is memory-mapped, convert it to a regular object first
  if (obj->is_mapped) {
    coil_err_t err = coil_obj_unmap(obj);
    if (err != COIL_ERR_GOOD) {
      return err;
    }
  }
  
  // Calculate file layout
  coil_size_t header_size = sizeof(coil_object_header_t);
  coil_size_t sectheaders_size = obj->header.section_count * sizeof(coil_section_header_t);
  
  // Calculate section offsets
  coil_u64_t data_offset = header_size + sectheaders_size;
  for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
    obj->sectheaders[i].offset = data_offset;
    
    // Get section size (either from section or header)
    coil_size_t size = obj->sectheaders[i].size;
    if (obj->sections != NULL && i < obj->loaded_count && obj->sections[i].data != NULL) {
      size = obj->sections[i].size;
      obj->sectheaders[i].size = size; // Update header with actual size
    }
    
    data_offset += size;
  }
  
  // Update total file size
  obj->header.file_size = data_offset;
  
  // Write header
  coil_size_t written;
  coil_err_t err = coil_write(fd, (coil_byte_t *)&obj->header, header_size, &written);
  if (err != COIL_ERR_GOOD || written != header_size) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to write object header");
  }
  
  // Write section headers
  if (obj->header.section_count > 0) {
    err = coil_write(fd, (coil_byte_t *)obj->sectheaders, sectheaders_size, &written);
    if (err != COIL_ERR_GOOD || written != sectheaders_size) {
      return COIL_ERROR(COIL_ERR_IO, "Failed to write section headers");
    }
  }
  
  // Write section data
  for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
    // Only write sections that have data
    if (obj->sections != NULL && i < obj->loaded_count && obj->sections[i].data != NULL && obj->sections[i].size > 0) {
      // Seek to section offset
      err = coil_seek(fd, obj->sectheaders[i].offset, SEEK_SET);
      if (err != COIL_ERR_GOOD) {
        return err;
      }
      
      // Write section data
      err = coil_section_serialize(&obj->sections[i], fd);
      if (err != COIL_ERR_GOOD) {
        return COIL_ERROR(COIL_ERR_IO, "Failed to write section data");
      }
    }
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Delete a section from the object
*/
coil_err_t coil_obj_delete_section(coil_object_t *obj, coil_u16_t index) {
  if (obj == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Object pointer is NULL");
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
  }
  
  // Free section data if loaded
  if (obj->sections != NULL && index < obj->loaded_count) {
    // Free memory but don't call section_cleanup (which would double-free in some cases)
    if (obj->sections[index].mode != COIL_SECT_MODE_VIEW && obj->sections[index].data != NULL) {
      coil_free(obj->sections[index].data);
      obj->sections[index].data = NULL;
    }
  }
  
  // Shift section headers down
  if (index < obj->header.section_count - 1) {
    coil_memmove(&obj->sectheaders[index], &obj->sectheaders[index + 1],
               (obj->header.section_count - index - 1) * sizeof(coil_section_header_t));
    
    // Shift loaded sections down
    if (obj->sections != NULL && index < obj->loaded_count - 1) {
      coil_memmove(&obj->sections[index], &obj->sections[index + 1],
                 (obj->loaded_count - index - 1) * sizeof(coil_section_t));
    }
  }
  
  // Update section count
  obj->header.section_count--;
  
  // Update loaded count
  if (obj->loaded_count > 0) {
    obj->loaded_count--;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Calculate a hash for section names
*/
coil_u64_t coil_obj_hash_name(const char *name) {
  if (name == NULL) {
    return 0;
  }
  
  // FNV-1a hash algorithm (simple and effective)
  coil_u64_t hash = 0xcbf29ce484222325ULL; // FNV offset basis
  const unsigned char *str = (const unsigned char *)name;
  
  while (*str) {
    hash ^= (coil_u64_t)*str++;
    hash *= 0x100000001b3ULL; // FNV prime
  }
  
  return hash;
}

/**
* @brief Find a section by name
*/
coil_err_t coil_obj_find_section(coil_object_t *obj, const char *name, coil_u16_t *index) {
  if (obj == NULL || name == NULL || index == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Calculate name hash
  coil_u64_t name_hash = coil_obj_hash_name(name);
  
  return coil_obj_find_section_by_hash(obj, name_hash, index);
}

/**
* @brief Find a section by name hash
*/
coil_err_t coil_obj_find_section_by_hash(coil_object_t *obj, coil_u64_t name_hash, coil_u16_t *index) {
  if (obj == NULL || index == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Search through section headers
  for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
    if (obj->sectheaders[i].name == name_hash) {
      *index = i;
      return COIL_ERR_GOOD;
    }
  }
  
  return COIL_ERROR(COIL_ERR_NOTFOUND, "Section not found");
}

/**
* @brief Load a section by index
*/
coil_err_t coil_obj_load_section(coil_object_t *obj, coil_u16_t index, 
                              coil_section_t *sect, int mode) {
  if (obj == NULL || sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
  }
  
  // Get section header
  coil_section_header_t *header = &obj->sectheaders[index];
  
  // Determine section mode and load mode
  coil_section_mode_t section_mode = COIL_SECT_MODE_MODIFY; // Default mode
  int load_flags = mode;
  
  // If COIL_SLOAD_VIEW is set, use VIEW mode
  if (load_flags & COIL_SLOAD_VIEW) {
    section_mode = COIL_SECT_MODE_VIEW;
  }
  
  // Check if section is already loaded in memory
  if (obj->sections != NULL && index < obj->loaded_count && obj->sections[index].data != NULL) {
    // Section is already loaded, create a copy
    coil_section_t *src_sect = &obj->sections[index];
    
    // Initialize the destination section
    coil_err_t err = coil_section_init(sect, src_sect->size > 0 ? src_sect->size : 1024);
    if (err != COIL_ERR_GOOD) {
      return err;
    }
    
    // Copy the data
    if (src_sect->size > 0) {
      coil_size_t bytes_written;
      err = coil_section_write(sect, src_sect->data, src_sect->size, &bytes_written);
      if (err != COIL_ERR_GOOD) {
        coil_section_cleanup(sect);
        return err;
      }
    }
    
    // Copy metadata
    sect->name = header->name;
    sect->mode = section_mode;
    
    return COIL_ERR_GOOD;
  }
  
  // Allocate sections array if needed
  if (obj->sections == NULL) {
    obj->sections = (coil_section_t *)coil_calloc(
        obj->header.section_count, sizeof(coil_section_t));
    
    if (obj->sections == NULL) {
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for sections");
    }
  }
  
  // If the object is memory mapped and we're using VIEW mode
  if (obj->is_mapped && obj->memory != NULL && (load_flags & COIL_SLOAD_VIEW)) {
    // Initialize section with view of mapped memory
    coil_memset(sect, 0, sizeof(coil_section_t));
    sect->data = (coil_byte_t *)obj->memory + header->offset;
    sect->size = header->size;
    sect->capacity = header->size;
    sect->mode = COIL_SECT_MODE_VIEW;  // Explicitly set to VIEW mode
    sect->name = header->name;
    
    // Create a copy for the object's internal sections array if needed
    if (obj->sections) {
      coil_section_t *obj_sect = &obj->sections[index];
      *obj_sect = *sect;
    
      if (index >= obj->loaded_count) {
        obj->loaded_count = index + 1;
      }
    }
    
    return COIL_ERR_GOOD;
  }
  
  // If we have a valid file descriptor, load from file
  if (obj->fd >= 0) {
    // Get section size
    coil_size_t section_size = header->size;
    if (section_size == 0) {
      // Create an empty section if size is zero
      coil_err_t err = coil_section_init(sect, 1024);
      if (err != COIL_ERR_GOOD) {
        return err;
      }
      sect->mode = section_mode;
    } else {
      // Seek to section data
      coil_err_t err = coil_seek(obj->fd, header->offset, SEEK_SET);
      if (err != COIL_ERR_GOOD) {
        return err;
      }
      
      // If MMAP is requested, use memory mapping
      if (load_flags & COIL_SLOAD_MMAP) {
        err = coil_section_loadv(sect, section_size, obj->fd);
        if (err == COIL_ERR_GOOD) {
          // Make sure the mode is VIEW for mapped sections
          sect->mode = COIL_SECT_MODE_VIEW;
        }
      } else {
        // Initialize the section with enough space
        err = coil_section_init(sect, section_size);
        if (err != COIL_ERR_GOOD) {
          return err;
        }
        sect->mode = section_mode;
        
        // Read the section data
        coil_size_t bytes_read;
        err = coil_read(obj->fd, sect->data, section_size, &bytes_read);
        if (err != COIL_ERR_GOOD) {
          coil_section_cleanup(sect);
          return COIL_ERROR(COIL_ERR_IO, "Failed to read section data");
        }
        
        // Update section size based on actual bytes read
        sect->size = bytes_read;
        
        // If we didn't read the expected amount of data, that's a warning but not fatal
        if (bytes_read != section_size) {
          coil_log(COIL_LEVEL_WARNING, "Section data incomplete: expected %zu bytes, got %zu", 
                  section_size, bytes_read);
        }
      }
      
      if (err != COIL_ERR_GOOD) {
        return err;
      }
    }
  } else {
    // No file descriptor, initialize an empty section
    coil_err_t err = coil_section_init(sect, 1024);
    if (err != COIL_ERR_GOOD) {
      return err;
    }
    sect->mode = section_mode;
  }
  
  // Copy section information
  sect->name = header->name;
  
  // Store in sections array
  if (index >= obj->loaded_count) {
    obj->loaded_count = index + 1;
  }
  
  // Make a copy for the object's internal sections array
  coil_section_t *obj_sect = &obj->sections[index];
  if (obj_sect->data == NULL) {
    if (sect->mode == COIL_SECT_MODE_VIEW) {
      // For view mode, just copy the pointer
      *obj_sect = *sect;
    } else {
      // For modify mode, make a true copy
      coil_err_t err = coil_section_init(obj_sect, sect->capacity);
      if (err != COIL_ERR_GOOD) {
        // Non-fatal error, just log it
        coil_log(COIL_LEVEL_WARNING, "Failed to initialize internal section copy");
      } else {
        // Copy data if any
        if (sect->size > 0) {
          coil_memcpy(obj_sect->data, sect->data, sect->size);
          obj_sect->size = sect->size;
        }
        
        // Copy metadata
        obj_sect->name = sect->name;
        obj_sect->mode = sect->mode;
      }
    }
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Create a new section in the object
*/
coil_err_t coil_obj_create_section(coil_object_t *obj, coil_u8_t type, const char *name, 
                                 coil_u16_t flags, coil_section_t *sect, coil_u16_t *index) {
  if (obj == NULL || name == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Calculate name hash
  coil_u64_t name_hash = coil_obj_hash_name(name);
  
  // Check if section already exists
  coil_u16_t existing_index;
  if (coil_obj_find_section_by_hash(obj, name_hash, &existing_index) == COIL_ERR_GOOD) {
    return COIL_ERROR(COIL_ERR_EXISTS, "Section already exists");
  }
  
  // Grow section headers array
  coil_u16_t new_index = obj->header.section_count;
  coil_size_t new_size = (new_index + 1) * sizeof(coil_section_header_t);
  
  coil_section_header_t *new_headers;
  if (obj->sectheaders == NULL) {
    new_headers = (coil_section_header_t *)coil_malloc(new_size);
  } else {
    new_headers = (coil_section_header_t *)coil_realloc(
        obj->sectheaders, new_size);
  }
  
  if (new_headers == NULL) {
    return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for section headers");
  }
  
  obj->sectheaders = new_headers;
  
  // Initialize new section header
  coil_section_header_t *header = &obj->sectheaders[new_index];
  coil_memset(header, 0, sizeof(coil_section_header_t));
  
  header->name = name_hash;
  header->type = type;
  header->flags = flags;
  
  // Set target metadata from defaults
  header->pu = obj->default_pu;
  header->raw_arch = obj->default_arch;
  header->features = obj->default_features;
  
  // If section data is provided, copy it
  if (sect != NULL) {
    // Set size and offset (will be updated during save)
    header->size = sect->size;
    header->offset = 0; // Will be calculated during save
    
    // Grow sections array
    coil_section_t *new_sections;
    if (obj->sections == NULL) {
      new_sections = (coil_section_t *)coil_calloc(
          new_index + 1, sizeof(coil_section_t));
    } else {
      new_sections = (coil_section_t *)coil_realloc(
          obj->sections, (new_index + 1) * sizeof(coil_section_t));
      if (new_sections != NULL && new_index > obj->loaded_count) {
        // Zero-initialize new sections
        coil_memset(&new_sections[obj->loaded_count], 0, 
                   (new_index - obj->loaded_count) * sizeof(coil_section_t));
      }
    }
    
    if (new_sections == NULL) {
      return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for sections");
    }
    
    obj->sections = new_sections;
    
    // Copy section data with ownership transfer
    coil_section_t *new_sect = &obj->sections[new_index];
    
    // Do a shallow copy first
    coil_memcpy(new_sect, sect, sizeof(coil_section_t));
    
    // Mark the original section as no longer owning the data
    // to avoid double-free issues
    sect->data = NULL;
    sect->capacity = 0;
    sect->size = 0;
    
    if (new_index >= obj->loaded_count) {
      obj->loaded_count = new_index + 1;
    }
  }
  
  // Update section count
  obj->header.section_count++;
  
  // Return new section index
  if (index != NULL) {
    *index = new_index;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Update a section in the object
*/
coil_err_t coil_obj_update_section(coil_object_t *obj, coil_u16_t index, coil_section_t *sect) {
  if (obj == NULL || sect == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid parameters");
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERROR(COIL_ERR_NOTFOUND, "Section index out of range");
  }
  
  // Update section header
  coil_section_header_t *header = &obj->sectheaders[index];
  header->size = sect->size;
  
  // Update section data if loaded
  if (obj->sections != NULL && index < obj->loaded_count) {
    coil_section_t *dest_sect = &obj->sections[index];
    
    // Clean up existing section's data (not using coil_section_cleanup to avoid NULL pointer issues)
    if (dest_sect->mode != COIL_SECT_MODE_VIEW && dest_sect->data != NULL) {
      coil_free(dest_sect->data);
      dest_sect->data = NULL;
    }
    
    // Copy new section
    coil_memcpy(dest_sect, sect, sizeof(coil_section_t));
    
    // Mark the original section as no longer owning the data
    sect->data = NULL;
    sect->capacity = 0;
    sect->size = 0;
  }
  
  return COIL_ERR_GOOD;
}