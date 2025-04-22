/**
* @file obj.h
* @brief Define the standard instruction interface for serialization and deserialization
*/

#ifndef __COIL_INCLUDE_GUARD_OBJECT_H
#define __COIL_INCLUDE_GUARD_OBJECT_H

#include <coil/section.h>
#include <coil/types.h>

/**
* @brief COIL Object file header
* 
* Fixed-size header at the beginning of every COIL object file
*/
typedef struct coil_object_header {
  coil_u8_t magic[4];          // Magic number (COIL_MAGIC)
  coil_u16_t version;          // Format version
  coil_u16_t section_count;    // Section Count
  coil_u64_t file_size;        // Complete object size
} coil_object_header_t;

/**
* @brief COIL Object
*/
typedef struct coil_object {
  // Object Format
  // [coil_object_header_t = header]
  // [coil_section_header_t...(header.section_count) = sectheaders]
  // [data]

  // the data of each section is only loaded on request and can be done with specific modes and optimizations 

  // Object Header
  coil_object_header_t header;

  // COIL Sections
  coil_section_header_t *sectheaders;
  coil_section_t *sections // size stored in header section_count

  // Object Memory
  coil_byte_t *memory;
  int fd; // for memory mapping
} coil_object_t;

// Object Initialization
coil_err_t coil_obj_init(coil_object_t *obj);

// Serialization and De-Serialization
coil_err_t coil_obj_load_file(coil_object_t *obj, int fd);
coil_err_t coil_obj_save_file(coil_object_t *obj, int fd);

// Section manipulation
coil_err_t coil_obj_sload(coil_object_t *obj, u16 index, coil_section_t *sect, int mode); // get section
coil_err_t coil_obj_screate(coil_object_t *obj, u16 index, coil_section_t *sect); // create section
coil_err_t coil_obj_sdelete(coil_object_t *obj, u16 index); // delete section
coil_err_t coil_obj_sfind(coil_object_t *obj, u64 name, u16 *index); // find section

#endif // __COIL_INCLUDE_GUARD_OBJECT_H