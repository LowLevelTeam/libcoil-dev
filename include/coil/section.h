/**
* @file section.h
* @brief Define the standard interaction with COIL Object Sections
*/

#ifndef __COIL_INCLUDE_GUARD_SECTION_H
#define __COIL_INCLUDE_GUARD_SECTION_H

#include <coil/types.h>

/**
* @brief Section header
*/
typedef struct coil_section_header {
  coil_u64_t name;             // Offset into string table for name
  coil_u64_t size;             // Section size in bytes
  coil_u64_t offset;           // Data location
  coil_u16_t flags;            // Section flags
  coil_u8_t type;              // Section type
} coil_section_header_t;

/**
* @brief Mutli facet coil section optimized for R_ONLY and RW
*/
typedef struct coil_section {
  coil_u64_t name;

  coil_byte_t *data;
  coil_size_t capacity;
  coil_size_t size;
  coil_size_t rindex; // offset into data for read operations
  coil_size_t windex; // offset into data for write operations
  int mode;
} coil_section_t;

/**
* @brief Initalize coil section with its own memory
*
* @param sect pointer to section to populate
* @param capacity The beginning capacity
* @param mode define the operations on this section
*/
coil_err_t coil_section_init(coil_section_t *sect, coil_size_t capacity, int mode);

/**
* @brief Initalize coil section with object memory
* 
* automatically addes COIL_SECT_MODE_O
*
* @param sect pointer to section to populate
* @param byte pointer to memory that can be used by the section provided by the object
* @param capacity the max memory allocated to this section
* @param mode define the operations on this section
*/
coil_err_t coil_section_init_view(coil_section_t *sect, coil_byte_t *byte, coil_size_t capacity, int mode);

/**
* @brief Write into section data from user provided buffer
*
* @param sect pointer to section
* @param buf pointer to buffer
* @param bufsize size of user buffer
* @param byteswritten pointer to size type to be populated with bytes read
* @return COIL Error Code
*/
coil_err_t coil_section_write(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *byteswritten);

/**
* @brief Read from section data into user provided buffer
*
* @param sect pointer to section
* @param buf pointer to buffer
* @param bufsize size of user buffer
* @param bytesread pointer to size type to be populated with bytes read
* @return COIL Error Code
*/
coil_err_t coil_section_read(coil_section_t *sect, coil_byte_t *buf, coil_size_t bufsize, coil_size_t *bytesread);

/**
* @brief Write into section data from user provided buffer
*
* @param sect pointer to section
* @param str C style string to append
* @return COIL Error Code
*/
coil_err_t coil_section_putstr(coil_section_t *sect, coil_byte_t *str);

/**
* @brief Read from section data into user provided buffer
*
* warning the string pointer could be invalidated on calls to write which may modify the size resulting in reallocation.
* it is recommended to use the string before any more calls to section related functions or copy the string.
*
* @param sect pointer to section
* @param offset offset into section the string is located at
* @param str pointer to a pointer to be set to a location in the section buffer
* @return COIL Error Code
*/
coil_err_t coil_section_getstr(coil_section_t *sect, u64 offset, coil_byte_t **str);

// Serialization and De-Serialization
coil_err_t coil_section_deserialize(coil_object_t *obj, FILE *fileptr);
coil_err_t coil_section_serialize(coil_object_t *obj, FILE *fileptr);

#endif // __COIL_INCLUDE_GUARD_SECTION_H