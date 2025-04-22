/**
 * @file section.h
 * @brief Define the standard interaction with COIL Object Sections
 * 
 * This file provides the interface for working with COIL object sections,
 * including initialization, reading, writing, and serialization operations.
 */

#ifndef __COIL_INCLUDE_GUARD_SECTION_H
#define __COIL_INCLUDE_GUARD_SECTION_H

#include <coil/types.h>
#include <coil/err.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for circular dependency
struct coil_object;
typedef struct coil_object coil_object_t;

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
  
  int mode;                    ///< Access mode (COIL_SECT_MODE_*)
  coil_section_ownership_t ownership;  ///< Memory ownership status
} coil_section_t;

/**
 * @brief Initialize coil section with its own memory
 *
 * @param sect Pointer to section to populate
 * @param capacity The beginning capacity
 * @param mode Define the operations on this section (COIL_SECT_MODE_*)
 * 
 * @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_NOMEM if memory allocation fails
 * 
 * @note Sets ownership to COIL_SECT_OWN_SELF
 */
coil_err_t coil_section_init(coil_section_t *sect, coil_size_t capacity, int mode);

/**
 * @brief Initialize coil section with object memory
 * 
 * Creates a view of existing memory without taking ownership.
 * Automatically adds COIL_SECT_MODE_O flag.
 *
 * @param sect Pointer to section to populate
 * @param byte Pointer to memory that can be used by the section provided by the object
 * @param capacity The max memory allocated to this section
 * @param mode Define the operations on this section
 * 
 * @return coil_err_t COIL_ERR_GOOD on success, COIL_ERR_INVAL for invalid parameters
 * 
 * @note Sets ownership to COIL_SECT_OWN_NONE
 */
coil_err_t coil_section_init_view(coil_section_t *sect, coil_byte_t *byte, coil_size_t capacity, int mode);

/**
 * @brief Clean up section resources
 * 
 * Frees memory if the section owns its buffer.
 * 
 * @param sect Pointer to section
 */
void coil_section_cleanup(coil_section_t *sect);

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

/**
 * @brief Deserialize a section from an object file
 * 
 * @param obj Object containing section header information
 * @param fd File descriptor for reading
 * @param index Section index
 * @param sect Section structure to populate
 * 
 * @return coil_err_t COIL_ERR_GOOD on success
 * @return coil_err_t COIL_ERR_IO on file read errors
 * @return coil_err_t COIL_ERR_INVAL for invalid parameters
 */
coil_err_t coil_section_deserialize(coil_object_t *obj, int fd, coil_u16_t index, coil_section_t *sect);

/**
 * @brief Serialize a section to an object file
 * 
 * @param obj Object to update with section information
 * @param fd File descriptor for writing
 * @param sect Section to serialize
 * @param index Section index to use
 * 
 * @return coil_err_t COIL_ERR_GOOD on success
 * @return coil_err_t COIL_ERR_IO on file write errors
 * @return coil_err_t COIL_ERR_INVAL for invalid parameters
 */
coil_err_t coil_section_serialize(coil_object_t *obj, int fd, coil_section_t *sect, coil_u16_t index);

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_SECTION_H */