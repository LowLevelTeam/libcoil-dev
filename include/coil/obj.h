/**
* @file obj.h
* @brief COIL Object functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_OBJ_H
#define __COIL_INCLUDE_GUARD_OBJ_H

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
  coil_u8_t has_native;        ///< Flag indicating if object contains native code sections
  coil_u8_t reserved[7];       ///< Reserved for future use
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
  
  // Native code metadata
  coil_native_format_t native_format;  ///< Native format when outputting to executable
  coil_pu_t default_pu;                ///< Default processing unit for native code
  coil_u8_t default_arch;              ///< Default architecture for native code
  coil_u32_t default_features;         ///< Default feature flags for native code
} coil_object_t;

/**
* @brief Object initialization flags
*/
typedef enum coil_obj_init_flag_e {
  COIL_OBJ_INIT_DEFAULT = 0,      ///< Default initialization
  COIL_OBJ_INIT_EMPTY = 1 << 0,   ///< Initialize as empty object (for creation)
  COIL_OBJ_INIT_NATIVE = 1 << 1,  ///< Initialize as native object (.coilo)
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
* @brief Set the default native code settings for an object
*
* @param obj Object to configure
* @param pu Default processing unit
* @param arch Default architecture
* @param features Default feature flags
* @param format Output format for native code
*
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj is NULL
*/
coil_err_t coil_obj_set_native_defaults(coil_object_t *obj, coil_pu_t pu, coil_u8_t arch, 
                                        coil_u32_t features, coil_native_format_t format);

/**
* @brief Load object from file using normal file I/O
* 
* Loads the object header and section headers, but does not load section data
* until specifically requested via coil_obj_load_section.
* 
* @param obj Object to populate
* @param filepath Path to the file to load
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or filepath is NULL
* @return COIL_ERR_IO if file cannot be opened or read
* @return COIL_ERR_FORMAT if file format is invalid
*/
coil_err_t coil_obj_load_file(coil_object_t *obj, coil_descriptor_t fd);

/**
* @brief Load object from file using memory mapping
* 
* Maps the file directly into memory for read-only access.
* Any modifications will require copying the data first.
* 
* @param obj Object to populate
* @param filepath Path to the file to map
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or filepath is NULL
* @return COIL_ERR_IO if file cannot be opened or mapped
* @return COIL_ERR_FORMAT if file format is invalid
*/
coil_err_t coil_obj_mmap(coil_object_t *obj, coil_descriptor_t fd);

/**
* @brief Save object to file
* 
* @param obj Object to save
* @param filepath Path to the file to create or overwrite
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if obj or filepath is NULL
* @return COIL_ERR_IO if file cannot be created or written
*/
coil_err_t coil_obj_save_file(coil_object_t *obj, coil_descriptor_t fd);

/**
* @brief Export object to native executable format
* 
* @param obj Object to export
* @param filepath Path to the file to create or overwrite
* @param format Output format (COIL_NATIVE_FORMAT_*)
* @param pu Target processing unit (COIL_PU_*)
* @param arch Target architecture (depends on PU)
* 
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if parameters are invalid
* @return COIL_ERR_IO if file cannot be created or written
* @return COIL_ERR_NOTSUP if format, PU or arch is not supported
*/
coil_err_t coil_obj_export_native(coil_object_t *obj, coil_descriptor_t fd, 
                                  coil_native_format_t format, coil_pu_t pu, coil_u8_t arch);

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
* @brief Load native code from a section
*
* @param obj Object containing the section
* @param index Section index
* @param data Pointer to receive native code data
* @param size Pointer to receive native code size
* @param meta Pointer to receive native metadata (can be NULL)
*
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if parameters are invalid
* @return COIL_ERR_NOTFOUND if section index is out of range or doesn't contain native code
* @return COIL_ERR_IO if native code data cannot be read
*/
coil_err_t coil_obj_load_native(coil_object_t *obj, coil_u16_t index, 
                               coil_byte_t **data, coil_size_t *size, 
                               coil_native_meta_t *meta);

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
*/
coil_err_t coil_obj_create_section(coil_object_t *obj, coil_u8_t type, const char *name, 
                                 coil_u16_t flags, coil_section_t *sect, coil_u16_t *index);

/**
* @brief Create a new native code section in the object
*
* @param obj Object to add section to
* @param name Section name
* @param data Native code data
* @param size Size of native code data
* @param pu Processing unit type (COIL_PU_*)
* @param arch Architecture (depends on PU type)
* @param features Feature flags for the specific architecture
* @param index Pointer to store the new section index
*
* @return COIL_ERR_GOOD on success
* @return COIL_ERR_INVAL if parameters are invalid
* @return COIL_ERR_NOMEM if memory allocation fails
*/
coil_err_t coil_obj_create_native_section(coil_object_t *obj, const char *name,
                                        coil_byte_t *data, coil_size_t size,
                                        coil_pu_t pu, coil_u8_t arch, coil_u32_t features,
                                        coil_u16_t *index);

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