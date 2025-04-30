/**
* @file obj.c
* @brief COIL Object functionality implementation for libcoil-dev
*/

#include <coil/base.h>
#include <coil/obj.h>
#include <coil/sect.h>

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
    return COIL_ERR_INVAL;
  }
  
  // Zero initialize the object
  coil_memset(obj, 0, sizeof(coil_object_t));
  
  // Set magic, version and flags
  coil_memcpy(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC));
  obj->header.version = COIL_CURRENT_VERSION;
  obj->header.section_count = 0;
  obj->header.file_size = sizeof(coil_object_header_t);
  
  // Set native code flag if requested
  if (flags & COIL_OBJ_INIT_NATIVE) {
    obj->header.has_native = 1;
  }
  
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
      coil_section_cleanup(&obj->sections[i]);
    }
    coil_free(obj->sections);
  }
  
  // Free section headers
  if (obj->sectheaders != NULL) {
    coil_free(obj->sectheaders);
  }
  
  // Unmap memory if mapped
  if (obj->is_mapped && obj->memory != NULL) {
    // Calculate total size for unmapping
    coil_munmap(obj->memory, obj->header.file_size);
  }
  
  // Close file descriptor if open
  if (obj->fd >= 0) {
    coil_close(obj->fd);
  }
  
  // Reset object
  coil_memset(obj, 0, sizeof(coil_object_t));
  obj->fd = -1;
}

/**
* @brief Set the default native code settings for an object
*/
coil_err_t coil_obj_set_native_defaults(coil_object_t *obj, coil_pu_t pu, coil_u8_t arch, 
                                      coil_u32_t features) {
  if (obj == NULL) {
    return COIL_ERR_INVAL;
  }
  
  obj->default_pu = pu;
  obj->default_arch = arch;
  obj->default_features = features;
  obj->header.has_native = 1;
  
  return COIL_ERR_GOOD;
}

/**
* @brief Load object from file using normal file I/O
*/
coil_err_t coil_obj_load_file(coil_object_t *obj, coil_descriptor_t fd) {
  if (obj == NULL || fd < 0) {
    return COIL_ERR_INVAL;
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
    return COIL_ERR_IO;
  }
  
  // Verify magic
  if (coil_memcmp(obj->header.magic, COIL_MAGIC, sizeof(COIL_MAGIC)) != 0) {
    return COIL_ERR_FORMAT;
  }
  
  // Read section headers if present
  if (obj->header.section_count > 0) {
    // Allocate memory for section headers
    coil_size_t headers_size = obj->header.section_count * sizeof(coil_section_header_t);
    obj->sectheaders = (coil_section_header_t *)coil_malloc(headers_size);
    
    if (obj->sectheaders == NULL) {
      return COIL_ERR_NOMEM;
    }
    
    // Read section headers
    err = coil_read(fd, (coil_byte_t *)obj->sectheaders, headers_size, &bytesread);
    
    if (err != COIL_ERR_GOOD || bytesread != headers_size) {
      coil_free(obj->sectheaders);
      obj->sectheaders = NULL;
      return COIL_ERR_IO;
    }
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
    return COIL_ERR_INVAL;
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
    return COIL_ERR_INVAL;
  }
  
  // Search through section headers
  for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
    if (obj->sectheaders[i].name == name_hash) {
      *index = i;
      return COIL_ERR_GOOD;
    }
  }
  
  return COIL_ERR_NOTFOUND;
}

/**
* @brief Load a section by index
*/
coil_err_t coil_obj_load_section(coil_object_t *obj, coil_u16_t index, 
                              coil_section_t *sect, int mode) {
  if (obj == NULL || sect == NULL) {
    return COIL_ERR_INVAL;
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERR_NOTFOUND;
  }
  
  // Get section header
  coil_section_header_t *header = &obj->sectheaders[index];
  
  // Allocate sections array if needed
  if (obj->sections == NULL) {
    obj->sections = (coil_section_t *)coil_calloc(
        obj->header.section_count, sizeof(coil_section_t));
    
    if (obj->sections == NULL) {
      return COIL_ERR_NOMEM;
    }
  }
  
  // Seek to section data
  coil_err_t err = coil_seek(obj->fd, header->offset, SEEK_SET);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Load the section
  if (mode & COIL_SLOAD_VIEW) {
    // TODO: Implement view mode (would need memory mapping)
    return COIL_ERR_NOTSUP;
  } else {
    // Copy mode
    err = coil_section_load(sect, header->size, obj->fd);
    if (err != COIL_ERR_GOOD) {
      return err;
    }
    
    // Copy section information
    sect->name = header->name;
    
    // Copy native code metadata if present
    if (header->has_native) {
      sect->has_native = 1;
      coil_memcpy(&sect->native, &header->native, sizeof(coil_native_meta_t));
    }
  }
  
  // Store in sections array
  if (index >= obj->loaded_count) {
    obj->loaded_count = index + 1;
  }
  coil_memcpy(&obj->sections[index], sect, sizeof(coil_section_t));
  
  return COIL_ERR_GOOD;
}

/**
* @brief Create a new section in the object
*/
coil_err_t coil_obj_create_section(coil_object_t *obj, coil_u8_t type, const char *name, 
                                coil_u16_t flags, coil_section_t *sect, coil_u16_t *index) {
  if (obj == NULL || name == NULL) {
    return COIL_ERR_INVAL;
  }
  
  // Calculate name hash
  coil_u64_t name_hash = coil_obj_hash_name(name);
  
  // Check if section already exists
  coil_u16_t existing_index;
  if (coil_obj_find_section_by_hash(obj, name_hash, &existing_index) == COIL_ERR_GOOD) {
    return COIL_ERR_EXISTS;
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
    return COIL_ERR_NOMEM;
  }
  
  obj->sectheaders = new_headers;
  
  // Initialize new section header
  coil_section_header_t *header = &obj->sectheaders[new_index];
  coil_memset(header, 0, sizeof(coil_section_header_t));
  
  header->name = name_hash;
  header->type = type;
  header->flags = flags;
  
  // If section data is provided, copy it
  if (sect != NULL) {
    // Set size and offset (will be updated during save)
    header->size = sect->size;
    header->offset = 0; // Will be calculated during save
    
    // Copy native code metadata if present
    if (sect->has_native) {
      header->has_native = 1;
      coil_memcpy(&header->native, &sect->native, sizeof(coil_native_meta_t));
    }
    
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
      return COIL_ERR_NOMEM;
    }
    
    obj->sections = new_sections;
    
    // Copy section data
    coil_section_t *new_sect = &obj->sections[new_index];
    coil_memcpy(new_sect, sect, sizeof(coil_section_t));
    
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
    return COIL_ERR_INVAL;
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERR_NOTFOUND;
  }
  
  // Update section header
  coil_section_header_t *header = &obj->sectheaders[index];
  header->size = sect->size;
  
  // Update native code metadata if present
  if (sect->has_native) {
    header->has_native = 1;
    coil_memcpy(&header->native, &sect->native, sizeof(coil_native_meta_t));
  } else {
    header->has_native = 0;
  }
  
  // Update section data if loaded
  if (obj->sections != NULL && index < obj->loaded_count) {
    coil_section_t *dest_sect = &obj->sections[index];
    
    // Clean up existing section
    coil_section_cleanup(dest_sect);
    
    // Copy new section
    coil_memcpy(dest_sect, sect, sizeof(coil_section_t));
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Save object to file
*/
coil_err_t coil_obj_save_file(coil_object_t *obj, coil_descriptor_t fd) {
  if (obj == NULL || fd < 0) {
    return COIL_ERR_INVAL;
  }
  
  // Calculate file layout
  coil_size_t header_size = sizeof(coil_object_header_t);
  coil_size_t sectheaders_size = obj->header.section_count * sizeof(coil_section_header_t);
  
  // Calculate section offsets
  coil_u64_t data_offset = header_size + sectheaders_size;
  for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
    obj->sectheaders[i].offset = data_offset;
    data_offset += obj->sectheaders[i].size;
  }
  
  // Update total file size
  obj->header.file_size = data_offset;
  
  // Write header
  coil_size_t written;
  coil_err_t err = coil_write(fd, (coil_byte_t *)&obj->header, header_size, &written);
  if (err != COIL_ERR_GOOD || written != header_size) {
    return COIL_ERR_IO;
  }
  
  // Write section headers
  if (obj->header.section_count > 0) {
    err = coil_write(fd, (coil_byte_t *)obj->sectheaders, sectheaders_size, &written);
    if (err != COIL_ERR_GOOD || written != sectheaders_size) {
      return COIL_ERR_IO;
    }
  }
  
  // Write section data
  for (coil_u16_t i = 0; i < obj->loaded_count; i++) {
    if (i < obj->header.section_count) {
      // Seek to section offset
      err = coil_seek(fd, obj->sectheaders[i].offset, SEEK_SET);
      if (err != COIL_ERR_GOOD) {
        return err;
      }
      
      // Write section data
      err = coil_section_serialize(&obj->sections[i], fd);
      if (err != COIL_ERR_GOOD) {
        return err;
      }
    }
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Create a new native code section in the object
*/
coil_err_t coil_obj_create_native_section(coil_object_t *obj, const char *name,
                                       coil_byte_t *data, coil_size_t size,
                                       coil_pu_t pu, coil_u8_t arch, coil_u32_t features,
                                       coil_u16_t *index) {
  if (obj == NULL || name == NULL || data == NULL || size == 0) {
    return COIL_ERR_INVAL;
  }
  
  // Create a temporary section
  coil_section_t sect;
  coil_err_t err = coil_section_init(&sect, size);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Write native code to section
  coil_size_t written;
  err = coil_section_write(&sect, data, size, &written);
  if (err != COIL_ERR_GOOD) {
    coil_section_cleanup(&sect);
    return err;
  }
  
  // Set native code metadata
  err = coil_section_set_native(&sect, pu, arch, features, 0, size);
  if (err != COIL_ERR_GOOD) {
    coil_section_cleanup(&sect);
    return err;
  }
  
  // Create section in object
  err = coil_obj_create_section(obj, COIL_SECTION_NATIVE, name, 
                             COIL_SECTION_FLAG_NATIVE, &sect, index);
  
  // Clean up temporary section
  coil_section_cleanup(&sect);
  
  return err;
}

/**
* @brief Load native code from a section
*/
coil_err_t coil_obj_load_native(coil_object_t *obj, coil_u16_t index, 
                              coil_byte_t **data, coil_size_t *size, 
                              coil_native_meta_t *meta) {
  if (obj == NULL || data == NULL || size == NULL) {
    return COIL_ERR_INVAL;
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERR_NOTFOUND;
  }
  
  // Check if section has native code
  if (!obj->sectheaders[index].has_native) {
    return COIL_ERR_NOTFOUND;
  }
  
  // Load section if not already loaded
  if (obj->sections == NULL || index >= obj->loaded_count || 
      obj->sections[index].data == NULL) {
    
    coil_section_t sect;
    coil_err_t err = coil_obj_load_section(obj, index, &sect, COIL_SLOAD_DEFAULT);
    if (err != COIL_ERR_GOOD) {
      return err;
    }
  }
  
  // Get native code data
  coil_err_t err = coil_section_get_native_data(&obj->sections[index], data, size);
  if (err != COIL_ERR_GOOD) {
    return err;
  }
  
  // Copy metadata if requested
  if (meta != NULL) {
    coil_memcpy(meta, &obj->sections[index].native, sizeof(coil_native_meta_t));
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Delete a section from the object
*/
coil_err_t coil_obj_delete_section(coil_object_t *obj, coil_u16_t index) {
  if (obj == NULL) {
    return COIL_ERR_INVAL;
  }
  
  // Check if index is valid
  if (index >= obj->header.section_count) {
    return COIL_ERR_NOTFOUND;
  }
  
  // Free section data if loaded
  if (obj->sections != NULL && index < obj->loaded_count) {
    coil_section_cleanup(&obj->sections[index]);
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