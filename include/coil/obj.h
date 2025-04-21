/**
 * @file obj.h
 * @brief COIL object format definition
 * 
 * Defines a compact binary format for storing COIL code, inspired by ELF
 * but specialized for the needs of the Computer Oriented Intermediate Language (COIL).
 * Designed for optimal storage and linking of COIL code.
 */

#ifndef __COIL_INCLUDE_GUARD_OBJ_H
#define __COIL_INCLUDE_GUARD_OBJ_H

#include <ccoil/err.h>
#include <ccoil/arena.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Basic types used in object format
 */
typedef uint8_t coil_u8_t;
typedef uint16_t coil_u16_t;
typedef uint32_t coil_u32_t;
typedef uint64_t coil_u64_t;

/**
 * @brief Magic number for COIL object files
 * "COIL" in ASCII
 */
#define COIL_MAGIC_BYTES {'C', 'O', 'I', 'L'}

/**
 * @brief Current format version
 */
#define COIL_VERSION 0x0100 /* Version 1.0 */

/**
 * @brief Section types
 */
typedef enum coil_section_type_e {
  COIL_SECTION_NULL = 0,        // Null section
  COIL_SECTION_PROGBITS = 1,    // Program space with data
  COIL_SECTION_SYMTAB = 2,      // Symbol table
  COIL_SECTION_STRTAB = 3,      // String table
  COIL_SECTION_RELTAB = 4,      // Relocation entries
  COIL_SECTION_NOBITS = 5,      // Program space with no data (bss)
  COIL_SECTION_DEBUG = 6        // Debug information
} coil_section_type_t;

/**
 * @brief Section flags
 */
typedef enum coil_section_flag_e {
  COIL_SECTION_FLAG_NONE = 0,          // No flags
  COIL_SECTION_FLAG_WRITE = 1 << 0,    // Writable
  COIL_SECTION_FLAG_CODE = 1 << 1,     // Compile this section as COIL
  COIL_SECTION_FLAG_MERGE = 1 << 2,    // Might be merged
  COIL_SECTION_FLAG_ALLOC = 1 << 3,    // Occupies memory during execution
  COIL_SECTION_FLAG_TLS = 1 << 4       // Thread-local storage
} coil_section_flag_t;

/**
 * @brief Symbol types
 */
typedef enum coil_symbol_type_e {
  COIL_SYMBOL_NOTYPE = 0,      // Type not specified
  COIL_SYMBOL_OBJECT = 1,      // Data object
  COIL_SYMBOL_FUNC = 2,        // Function
  COIL_SYMBOL_SECTION = 3,     // Section
  COIL_SYMBOL_FILE = 4         // File
} coil_symbol_type_t;

/**
 * @brief Symbol binding
 */
typedef enum coil_symbol_binding_e {
  COIL_SYMBOL_LOCAL = 0,       // Local symbol
  COIL_SYMBOL_GLOBAL = 1,      // Global symbol
  COIL_SYMBOL_WEAK = 2         // Weak symbol
} coil_symbol_binding_t;

/**
 * @brief COIL Object file header
 * 
 * Fixed-size header at the beginning of every COIL object file
 */
typedef struct coil_object_header_s {
  coil_u8_t magic[4];          // Magic number (COIL_MAGIC)
  coil_u16_t version;          // Format version
  coil_u16_t section_count;    // Section Count
  coil_u64_t file_size;        // Complete object size
} coil_object_header_t;

/**
 * @brief Section header
 */
typedef struct coil_section_header_s {
  coil_u64_t name;             // Offset into string table for name
  coil_u64_t size;             // Section size in bytes
  coil_u16_t flags;            // Section flags
  coil_u8_t type;              // Section type
} coil_section_header_t;

/**
 * @brief Symbol table entry
 */
typedef struct coil_symbol_s {
  coil_u64_t name;             // Symbol name (string table offset)
  coil_u32_t value;            // Symbol value
  coil_u16_t section_index;    // Section index
  coil_u8_t type;              // Type information
  coil_u8_t binding;           // Binding information
} coil_symbol_t;

/**
 * @brief Opaque object type
 */
typedef struct coil_object_s coil_object_t;

/**
 * @brief Create a new COIL object
 * 
 * @param arena Arena to allocate object from (or NULL to use malloc)
 * @return coil_object_t* Pointer to new object or NULL on failure
 */
coil_object_t* coil_object_create(coil_arena_t* arena);

/**
 * @brief Destroy a COIL object and free resources
 * 
 * @param obj Pointer to the object to destroy
 * @param arena Arena the object was allocated from (or NULL if malloc was used)
 */
void coil_object_destroy(coil_object_t* obj, coil_arena_t* arena);

/**
 * @brief Load an object from a memory buffer
 * 
 * @param obj Pointer to the object
 * @param data Pointer to the data buffer
 * @param size Size of the data buffer
 * @param arena Arena to allocate internal data (can be NULL to use malloc)
 * @return coil_err_t Error code
 */
coil_err_t coil_object_load_from_memory(
  coil_object_t* obj,
  const void* data,
  size_t size,
  coil_arena_t* arena
);

/**
 * @brief Load an object from a file
 * 
 * @param obj Pointer to the object
 * @param filename Path to the file
 * @param arena Arena to allocate internal data (can be NULL to use malloc)
 * @return coil_err_t Error code
 */
coil_err_t coil_object_load_from_file(
  coil_object_t* obj,
  const char* filename,
  coil_arena_t* arena
);

/**
 * @brief Save an object to a memory buffer
 * 
 * @param obj Pointer to the object
 * @param arena Arena to allocate output buffer
 * @param data Pointer to store allocated data buffer (must be freed or owned by arena)
 * @param size Pointer to store size of data buffer
 * @return coil_err_t Error code
 */
coil_err_t coil_object_save_to_memory(
  const coil_object_t* obj,
  coil_arena_t* arena,
  void** data,
  size_t* size
);

/**
 * @brief Save an object to a file
 * 
 * @param obj Pointer to the object
 * @param filename Path to the file
 * @return coil_err_t Error code
 */
coil_err_t coil_object_save_to_file(
  const coil_object_t* obj,
  const char* filename
);

/**
 * @brief Get object header
 * 
 * @param obj Pointer to the object
 * @return const coil_object_header_t* Pointer to the header
 */
const coil_object_header_t* coil_object_get_header(const coil_object_t* obj);

/**
 * @brief Get the number of sections in an object
 * 
 * @param obj Pointer to the object
 * @return coil_u16_t Number of sections
 */
coil_u16_t coil_object_get_section_count(const coil_object_t* obj);

/**
 * @brief Initialize string table if not already present
 * 
 * @param obj Pointer to the object
 * @param arena Arena to allocate memory from (or NULL to use malloc)
 * @return coil_err_t Error code
 */
coil_err_t coil_object_init_string_table(coil_object_t* obj, coil_arena_t* arena);

/**
 * @brief Initialize symbol table if not already present
 * 
 * @param obj Pointer to the object
 * @param arena Arena to allocate memory from (or NULL to use malloc)
 * @return coil_err_t Error code
 */
coil_err_t coil_object_init_symbol_table(coil_object_t* obj, coil_arena_t* arena);

/**
 * @brief Add a string to the string table
 * 
 * @param obj Pointer to the object
 * @param str String to add
 * @param arena Arena to allocate memory from (or NULL to use malloc)
 * @return coil_u64_t Offset of the string in table, or 0 on error
 */
coil_u64_t coil_object_add_string(
  coil_object_t* obj,
  const char* str,
  coil_arena_t* arena
);

/**
 * @brief Get a string from the string table
 * 
 * @param obj Pointer to the object
 * @param offset Offset of the string in the table
 * @param buffer Buffer to store the string
 * @param buffer_size Size of the buffer
 * @return coil_err_t Error code
 */
coil_err_t coil_object_get_string(
  const coil_object_t* obj,
  coil_u64_t offset,
  char* buffer,
  size_t buffer_size
);

/**
 * @brief Add a section to the object
 * 
 * @param obj Pointer to the object
 * @param name_offset Offset of section name in string table
 * @param flags Section flags
 * @param type Section type
 * @param data Section data
 * @param data_size Size of section data
 * @param arena Arena to allocate memory from (or NULL to use malloc)
 * @return coil_u16_t Index of new section (1-based, 0 indicates error)
 */
coil_u16_t coil_object_add_section(
  coil_object_t* obj,
  coil_u64_t name_offset,
  coil_u16_t flags,
  coil_u8_t type,
  const void* data,
  coil_u64_t data_size,
  coil_arena_t* arena
);

/**
 * @brief Get a section by index
 * 
 * @param obj Pointer to the object
 * @param index Section index (1-based)
 * @param header Pointer to store section header
 * @param data Pointer to store section data pointer (can be NULL)
 * @param data_size Pointer to store section data size (can be NULL)
 * @return coil_err_t Error code
 */
coil_err_t coil_object_get_section(
  const coil_object_t* obj,
  coil_u16_t index,
  coil_section_header_t* header,
  const void** data,
  coil_u64_t* data_size
);

/**
 * @brief Get a section index by name
 * 
 * @param obj Pointer to the object
 * @param name Section name
 * @return coil_u16_t Section index (1-based, 0 indicates not found)
 */
coil_u16_t coil_object_get_section_index(
  const coil_object_t* obj,
  const char* name
);

/**
 * @brief Add a symbol to the object
 * 
 * @param obj Pointer to the object
 * @param name Offset of symbol name in string table
 * @param value Symbol value
 * @param section_index Symbol section index
 * @param type Symbol type
 * @param binding Symbol binding
 * @param arena Arena to allocate memory from (or NULL to use malloc)
 * @return coil_u16_t Index of new symbol (1-based, 0 indicates error)
 */
coil_u16_t coil_object_add_symbol(
  coil_object_t* obj,
  coil_u64_t name,
  coil_u32_t value,
  coil_u16_t section_index,
  coil_u8_t type,
  coil_u8_t binding,
  coil_arena_t* arena
);

/**
 * @brief Get a symbol by index
 * 
 * @param obj Pointer to the object
 * @param index Symbol index (1-based)
 * @param symbol Pointer to store symbol
 * @return coil_err_t Error code
 */
coil_err_t coil_object_get_symbol(
  const coil_object_t* obj,
  coil_u16_t index,
  coil_symbol_t* symbol
);

/**
 * @brief Get a symbol index by name
 * 
 * @param obj Pointer to the object
 * @param name Symbol name
 * @return coil_u16_t Symbol index (1-based, 0 indicates not found)
 */
coil_u16_t coil_object_get_symbol_index(
  const coil_object_t* obj,
  const char* name
);

#ifdef __cplusplus
}
#endif

#endif /* __COIL_INCLUDE_GUARD_OBJ_H */