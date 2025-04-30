/**
* @file obj.h
* @brief COIL Object functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_OBJ_H
#define __COIL_INCLUDE_GUARD_OBJ_H

#include <coil/base.h>
#include <coil/sect.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief COIL Object file header
* 
* Fixed-size header at the beginning of every COIL object file
*/
typedef struct coil_object_header {
  coil_u8_t magic[4];          ///< Magic number (COIL_MAGIC)
  coil_u16_t version;          ///< Format version
  coil_u16_t section_count;    ///< Section Count
  coil_u64_t file_size;        ///< Complete object size
  coil_u8_t reserved[8];       ///< Reserved for future use
} coil_object_header_t;

/**
* @brief COIL Object
* 
* Primary structure for working with COIL object files
*/
typedef struct coil_object {
  // Object Format
  // [coil_object_header_t = header]
  // [coil_section_header_t...(header.section_count) = sectheaders]
  // [data]

  // Object Header
  coil_object_header_t header;

  // COIL Sections
  coil_section_header_t *sectheaders;  ///< Array of section headers
  coil_section_t *sections;            ///< Array of loaded sections (may be NULL if not loaded)
  coil_u16_t loaded_count;             ///< Number of currently loaded sections

  // Object Memory
  coil_byte_t *memory;                 ///< Memory for the object (may be memory mapped)
  coil_descriptor_t fd;                ///< File descriptor for memory mapping
  int is_mapped;                       ///< Flag indicating if memory is mapped
  
  // Default target metadata for new sections
  coil_pu_t default_pu;                ///< Default processing unit for target
  coil_u8_t default_arch;              ///< Default architecture for target
  coil_u64_t default_features;         ///< Default feature flags for target
} coil_object_t;

/**
* @brief Object initialization flags
*/
typedef enum coil_obj_init_flag_e {
  COIL_OBJ_INIT_DEFAULT = 0,      ///< Default initialization
  COIL_OBJ_INIT_EMPTY = 1 << 0,   ///< Initialize as empty object (for creation)
} coil_obj_init_flag_t;

/**
* @brief Section loading modes
*/
typedef enum coil_section_load_mode_e {
  COIL_SLOAD_DEFAULT = 0,         ///< Default loading (copy section data)
  COIL_SLOAD_VIEW = 1 << 0,       ///< View mode (direct pointer to object memory)
  COIL_SLOAD_MMAP = 1 << 1,       ///< Use memory mapping when possible
} coil_section_load_mode_t;

/**
* @brief Initialize a COIL object
* 
* Sets up an empty object structure ready for loading or creating sections.
* Does not allocate any memory for sections or data at this stage.
* 
* @param obj Pointer to object structure to initialize
* @param flags Initialization flags (COIL_OBJ_INIT_*)
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
* 
* @note This function must be called before any other operations on the object
*/
coil_err_t coil_obj_init(coil_object_t *obj, int flags);

/**
* @brief Clean up a COIL object and free associated resources
* 
* @param obj Object to clean up
*/
void coil_obj_cleanup(coil_object_t *obj);

/**
* @brief Set the default target metadata for sections in an object
*
* @param obj Object to configure
* @param pu Default processing unit
* @param arch Default architecture
* @param features Default feature flags
*
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
*/
coil_err_t coil_obj_set_target_defaults(coil_object_t *obj, coil_pu_t pu, coil_u8_t arch, 
                                        coil_u64_t features);

/**
* @brief Load object from file using normal file I/O
* 
* Loads the object header and section headers, but does not load section data
* until specifically requested via coil_obj_load_section.
* 
* @param obj Object to populate
* @param fd File descriptor for the file to load
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or fd is invalid
* @return COIL_ERR_IO if file cannot be read
* @return COIL_ERR_FORMAT if file format is invalid
*/
coil_err_t coil_obj_load_file(coil_object_t *obj, coil_descriptor_t fd);

/**
* @brief Load object from file using memory mapping
* 
* Maps the file directly into memory for read/write access.
* This can provide performance benefits for large files.
* 
* @param obj Object to populate
* @param fd File descriptor for the file to map
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or fd is invalid
* @return COIL_ERR_IO if file cannot be mapped
* @return COIL_ERR_FORMAT if file format is invalid
*/
coil_err_t coil_obj_mmap(coil_object_t *obj, coil_descriptor_t fd);

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
coil_err_t coil_obj_unmap(coil_object_t *obj);

/**
* @brief Save object to file
* 
* @param obj Object to save
* @param fd File descriptor for the file to create or overwrite
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or fd is invalid
* @return COIL_ERR_IO if file cannot be written
*/
coil_err_t coil_obj_save_file(coil_object_t *obj, coil_descriptor_t fd);

/**
* @brief Load a section by index
* 
* @param obj Object containing the section
* @param index Section index
* @param sect Pointer to section structure to populate
* @param mode Section access mode (COIL_SECT_MODE_*) and loading mode (COIL_SLOAD_*)
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if parameters are invalid
* @return COIL_ERR_NOTFOUND if section index is out of range
* @return COIL_ERR_IO if section data cannot be read
*/
coil_err_t coil_obj_load_section(coil_object_t *obj, coil_u16_t index, coil_section_t *sect, int mode);

/**
* @brief Create a new section in the object
* 
* @param obj Object to add section to
* @param type Section type (COIL_SECTION_*)
* @param name Section name
* @param flags Section flags (COIL_SECTION_FLAG_*)
* @param sect Pre-initialized section to copy data from (can be NULL for empty section)
* @param index Pointer to store the new section index
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL or parameters are invalid
* @return COIL_ERR_NOMEM if memory allocation fails
*
* @note This function transfers ownership of the section data to the object.
*       After this call, the 'sect' parameter will have its data field set to NULL,
*       and the object becomes responsible for freeing the data memory.
*/
coil_err_t coil_obj_create_section(coil_object_t *obj, coil_u8_t type, const char *name, coil_u16_t flags, coil_section_t *sect, coil_u16_t *index);

/**
* @brief Delete a section from the object
* 
* @param obj Object containing the section
* @param index Section index to delete
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
* @return COIL_ERR_NOTFOUND if section index is out of range
*/
coil_err_t coil_obj_delete_section(coil_object_t *obj, coil_u16_t index);

/**
* @brief Find a section by name
* 
* @param obj Object to search
* @param name Section name to find
* @param index Pointer to store the found section index
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or name is NULL
* @return COIL_ERR_NOTFOUND if section is not found
*/
coil_err_t coil_obj_find_section(coil_object_t *obj, const char *name, coil_u16_t *index);

/**
* @brief Find a section by name hash
* 
* More efficient than string comparison when name hash is already known.
* 
* @param obj Object to search
* @param name_hash Hash of the section name to find
* @param index Pointer to store the found section index
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
* @return COIL_ERR_NOTFOUND if section is not found
*/
coil_err_t coil_obj_find_section_by_hash(coil_object_t *obj, coil_u64_t name_hash, coil_u16_t *index);

/**
* @brief Update a section in the object
* 
* @param obj Object containing the section
* @param index Section index to update
* @param sect Section data to update with
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if parameters are invalid
* @return COIL_ERR_NOTFOUND if section index is out of range
*
* @note This function transfers ownership of the section data to the object,
*       similar to coil_obj_create_section.
*/
coil_err_t coil_obj_update_section(coil_object_t *obj, coil_u16_t index, coil_section_t *sect);

/**
* @brief Calculate a hash for section names
* 
* @param name String to hash
* 
* @return coil_u64_t Hash value
*/
coil_u64_t coil_obj_hash_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_OBJ_H