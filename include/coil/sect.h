/**
* @file sect.h
* @brief COIL Object Section functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_SECT_OBJ_H
#define __COIL_INCLUDE_GUARD_SECT_OBJ_H

#include <coil/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Native code metadata structure
*
* Contains information about the native machine code in a section
*/
typedef struct coil_native_meta {
  coil_pu_t pu;              ///< Processing unit type (CPU, GPU, etc.)
  union {
    coil_cpu_t cpu_arch;     ///< CPU architecture when pu is COIL_PU_CPU
    coil_gpu_t gpu_arch;     ///< GPU architecture when pu is COIL_PU_GPU
    coil_u8_t raw_arch;      ///< Raw architecture value for direct access
  };
  coil_u32_t features;       ///< Feature flags for the specific architecture
  coil_u64_t native_size;    ///< Size of native code in bytes
  coil_u64_t native_offset;  ///< Offset to native code within section data
} coil_native_meta_t;

/**
* @brief Section header
* 
* Contains metadata about a section stored in an object file
*/
typedef struct coil_section_header {
  coil_u64_t name;             ///< Offset into string table for name
  coil_u64_t size;             ///< Section size in bytes
  coil_u64_t offset;           ///< Data location
  coil_u16_t flags;            ///< Section flags
  coil_u8_t type;              ///< Section type
  coil_u8_t has_native;        ///< Flag indicating if section contains native code
  coil_native_meta_t native;   ///< Native code metadata (valid if has_native is non-zero)
} coil_section_header_t;

/**
* @brief Multi-facet coil section optimized for read-only and read-write access
* 
* This structure provides a unified interface for section data, with different 
* optimizations based on the access mode.
*/
typedef struct coil_section {
  coil_u64_t name;             ///< Section name or offset in string table
  
  coil_byte_t *data;           ///< Section data buffer
  coil_size_t capacity;        ///< Total allocated capacity
  coil_size_t size;            ///< Current used size
  coil_size_t rindex;          ///< Read index (offset for next read operation)
  coil_size_t windex;          ///< Write index (offset for next write operation)

  coil_section_mode_t mode;    ///< Section access mode
  
  coil_native_meta_t native;   ///< Native code metadata
  int has_native;              ///< Flag indicating if section contains native code
  
  int is_mapped;               ///< Flag indicating if section is memory mapped
  coil_size_t map_size;        ///< Original size of mapped memory (may differ from size due to alignment)
  void *map_base;              ///< Base address of mapped memory (may differ from data due to alignment)
} coil_section_t;

// -------------------------------- Section Operations -------------------------------- //

/**
* @brief Initialize coil section (COIL_SECT_MODE_CREATE)
*
* @param sect Pointer to section to populate
* @param capacity The beginning capacity
* 
* @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_NOMEM if memory allocation fails
*/
coil_err_t coil_section_init(coil_section_t *sect, coil_size_t capacity);

/**
* @brief Clean up section resources
* 
* Frees memory if the section owns its buffer.
* 
* @param sect Pointer to section
*/
void coil_section_cleanup(coil_section_t *sect);

/**
* @brief Set native code metadata for a section
*
* @param sect Pointer to section
* @param pu Processing unit type (COIL_PU_*)
* @param arch Architecture value (depends on PU type)
* @param features Feature flags for the specific architecture
* @param offset Offset to native code within section data
* @param size Size of native code in bytes
*
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if sect is NULL or parameters are invalid
*/
coil_err_t coil_section_set_native(coil_section_t *sect, coil_pu_t pu, coil_u8_t arch,
                                   coil_u32_t features, coil_u64_t offset, coil_u64_t size);

/**
* @brief Get native code data from a section
*
* @param sect Pointer to section
* @param data Pointer to receive native code data
* @param size Pointer to receive native code size
*
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if parameters are invalid
* @return coil_err_t COIL_ERR_NOTFOUND if section doesn't contain native code
*/
coil_err_t coil_section_get_native_data(coil_section_t *sect, coil_byte_t **data, coil_size_t *size);

/**
* @brief Clear native code metadata from a section
*
* @param sect Pointer to section
*
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if sect is NULL
*/
coil_err_t coil_section_clear_native(coil_section_t *sect);

/**
* @brief Write into section data from user provided buffer
*
* @param sect Pointer to section
* @param buf Pointer to buffer
* @param bufsize Size of user buffer
* @param byteswritten Pointer to size type to be populated with bytes written
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if parameters are invalid
* @return coil_err_t COIL_ERR_BADSTATE if section mode doesn't allow writing
* @return coil_err_t COIL_ERR_NOMEM if section can't be expanded and buffer is full
*/
coil_err_t coil_section_write(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *byteswritten);

/**
* @brief Read from section data into user provided buffer
*
* @param sect Pointer to section
* @param buf Pointer to buffer
* @param bufsize Size of user buffer
* @param bytesread Pointer to size type to be populated with bytes read
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if parameters are invalid
* @return coil_err_t COIL_ERR_BADSTATE if section mode doesn't allow reading
*/
coil_err_t coil_section_read(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *bytesread);

/**
* @brief Write a C-style string into section data
*
* @param sect Pointer to section
* @param str Null-terminated string to append
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if parameters are invalid
* @return coil_err_t COIL_ERR_BADSTATE if section mode doesn't allow writing
* @return coil_err_t COIL_ERR_NOMEM if section can't be expanded and buffer is full
*/
coil_err_t coil_section_putstr(coil_section_t *sect, const char *str);

/**
* @brief Get a string from section data at a specific offset
*
* Warning: The string pointer could be invalidated on calls to write which may modify 
* the size resulting in reallocation. It is recommended to use the string before any 
* more calls to section related functions or copy the string.
*
* @param sect Pointer to section
* @param offset Offset into section the string is located at
* @param str Pointer to a pointer to be set to a location in the section buffer
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if parameters are invalid
* @return coil_err_t COIL_ERR_BADSTATE if section mode doesn't allow reading
*/
coil_err_t coil_section_getstr(coil_section_t *sect, coil_u64_t offset, const char **str);

/**
* @brief Ensure section has at least specified capacity
* 
* If the section doesn't own its memory and needs to grow, it will copy to new 
* memory and take ownership.
* 
* @param sect Section to resize
* @param min_capacity Minimum capacity needed
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_NOMEM on allocation failure
* @return coil_err_t COIL_ERR_BADSTATE if section has a mode that forbids resizing
*/
coil_err_t coil_section_ensure_capacity(coil_section_t *sect, coil_size_t min_capacity);

/**
* @brief Shrink section buffer to fit current content
* 
* Only applies if section owns its memory.
* 
* @param sect Section to compact
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_BADSTATE if section doesn't own memory
*/
coil_err_t coil_section_compact(coil_section_t *sect);

/**
* @brief Reset section read/write indices
* 
* @param sect Section to reset
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if sect is NULL
*/
coil_err_t coil_section_reset(coil_section_t *sect);

/**
* @brief Seek to a specific position for reading
* 
* @param sect Section to operate on
* @param pos Position to seek to
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if position is beyond section size
*/
coil_err_t coil_section_seek_read(coil_section_t *sect, coil_size_t pos);

/**
* @brief Seek to a specific position for writing
* 
* @param sect Section to operate on
* @param pos Position to seek to
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_INVAL if position is beyond section size
*/
coil_err_t coil_section_seek_write(coil_section_t *sect, coil_size_t pos);

// -------------------------------- Serialization -------------------------------- //

/**
* @brief Serialize a section to an object file
* 
* @param sect Section to serialize
* @param fd File descriptor for writing
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_IO on file write errors
* @return coil_err_t COIL_ERR_INVAL for invalid parameters
*/
coil_err_t coil_section_serialize(coil_section_t *sect, coil_descriptor_t fd);

// -------------------------------- De-Serialization -------------------------------- //

/**
* @brief Initialize coil section from descriptor (copied) (COIL_SECT_MODE_MODIF)
*
* @param sect Pointer to section to populate
* @param capacity The beginning capacity
* @param fd File descriptor to read from
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_NOMEM if memory allocation fails
* @return coil_err_t COIL_ERR_IO if reading fails
*/
coil_err_t coil_section_load(coil_section_t *sect, coil_size_t capacity, coil_descriptor_t fd);

/**
* @brief Load coil section view (memory mapped) (COIL_SECT_MODE_VIEW)
*
* Creates a view of a file segment using memory mapping. This provides direct
* read-only access to file data without copying it to memory.
*
* @param sect Pointer to section to populate
* @param capacity Number of bytes to map (0 for all remaining bytes in file)
* @param fd File descriptor to map
* 
* @return coil_err_t COIL_ERR_GOOD on success
* @return coil_err_t COIL_ERR_IO if memory mapping fails
* @return coil_err_t COIL_ERR_INVAL for invalid parameters
*/
coil_err_t coil_section_loadv(coil_section_t *sect, coil_size_t capacity, coil_descriptor_t fd);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_SECT_OBJ_H