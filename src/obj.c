/*
 * Implementation of COIL object format
 */

#include <coil/obj.h>
#include <coil/arena.h>
#include <coil/err.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/**
 * Section information structure
 */
typedef struct coil_section_s {
	coil_section_header_t header;  // Section header
	void* data;                    // Section data
	size_t data_capacity;          // Capacity of data buffer
} coil_section_t;

/**
 * COIL object structure
 */
struct coil_object_s {
	coil_object_header_t header;     // Object header
	coil_section_t* sections;        // Array of sections
	size_t sections_capacity;        // Capacity of sections array
	coil_u16_t strtab_index;         // String table index (1-based, 0 if not present)
	coil_u16_t symtab_index;         // Symbol table index (1-based, 0 if not present)
	int uses_arena;                  // Whether this object uses arena allocation
};

/**
 * Initialize a default object header
 */
static void init_object_header(coil_object_header_t* header) {
	const coil_u8_t magic[] = COIL_MAGIC_BYTES;
	
	header->magic[0] = magic[0];
	header->magic[1] = magic[1];
	header->magic[2] = magic[2];
	header->magic[3] = magic[3];
	header->version = COIL_VERSION;
	header->section_count = 0;
	header->file_size = 0;
}

/**
 * Allocate memory either from arena or heap
 */
static void* alloc_mem(coil_arena_t* arena, size_t size) {
	if (arena) {
		return arena_alloc_default(arena, size);
	} else {
		return malloc(size);
	}
}

/**
 * Push data to arena or copy to heap
 */
static void* push_data(coil_arena_t* arena, const void* data, size_t size) {
	if (!data || size == 0) {
		return NULL;
	}
	
	void* mem = NULL;
	
	if (arena) {
		mem = arena_alloc_default(arena, size);
		if (mem) {
			memcpy(mem, data, size);
		}
	} else {
		mem = malloc(size);
		if (mem) {
			memcpy(mem, data, size);
		}
	}
	
	return mem;
}

/**
 * Find a section by name
 */
static coil_u16_t find_section_by_name(const coil_object_t* obj, const char* name) {
	// Cannot find a section without a string table
	if (!obj || !name || obj->strtab_index == 0) {
		return 0;
	}
	
	// Compare each section name
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		const coil_section_t* section = &obj->sections[i];
		
		// Get section name from string table
		char section_name[256] = {0};
		if (coil_object_get_string(obj, section->header.name, section_name, sizeof(section_name)) != COIL_ERR_GOOD) {
			continue;
		}
		
		// Compare names
		if (strcmp(section_name, name) == 0) {
			return i + 1; // 1-based index
		}
	}
	
	return 0; // Not found
}

/**
 * Add a string to the string table section
 */
static coil_u64_t add_string_to_table(coil_object_t* obj, const char* str, coil_arena_t* arena) {
	if (!obj || !str || obj->strtab_index == 0) {
		return 0;
	}
	
	// Get the string table section
	coil_section_t* strtab = &obj->sections[obj->strtab_index - 1];
	
	// Check if string already exists in table
	coil_u8_t* data = (coil_u8_t*)strtab->data;
	coil_u64_t size = strtab->header.size;
	
	// Empty string table should have at least one null byte
	if (size == 0) {
		coil_u8_t null_byte = 0;
		if (obj->uses_arena) {
			strtab->data = arena_alloc_default(arena, 1);
		} else {
			strtab->data = malloc(1);
		}
		
		if (!strtab->data) {
			COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate string table");
			return 0;
		}
		
		data = (coil_u8_t*)strtab->data;
		data[0] = null_byte;
		strtab->header.size = 1;
		strtab->data_capacity = 1;
		size = 1;
	}
	
	for (coil_u64_t i = 0; i < size; ) {
		// Check if we've hit the end of the table
		if (i >= size) break;
		
		// Compare strings
		const char* existing = (const char*)(data + i);
		size_t len = strlen(existing);
		
		// Make sure we don't go past the end
		if (i + len >= size) break;
		
		// Check if strings match
		if (strcmp(existing, str) == 0) {
			return i; // Return offset of existing string
		}
		
		// Move to next string
		i += len + 1;
	}
	
	// String not found, add it
	size_t len = strlen(str) + 1; // Include null terminator
	coil_u64_t offset = size;
	
	// If we're using an arena, we need to allocate a new string table
	if (obj->uses_arena) {
		// Create a new buffer with both existing content and new string
		void* new_data = alloc_mem(arena, size + len);
		if (!new_data) {
			COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for string table");
			return 0;
		}
		
		// Copy existing data
		if (size > 0 && strtab->data) {
			memcpy(new_data, strtab->data, size);
		}
		
		// Add new string
		memcpy((coil_u8_t*)new_data + size, str, len);
		
		// Replace old buffer
		strtab->data = new_data;
		strtab->data_capacity = size + len;
	} 
	// Using malloc, we can reallocate
	else {
		// Check if we need to resize
		if (size + len > strtab->data_capacity) {
			size_t new_capacity = (strtab->data_capacity * 2) + len;
			void* new_data = realloc(strtab->data, new_capacity);
			if (!new_data) {
				COIL_ERROR(COIL_ERR_NOMEM, "Failed to resize string table");
				return 0;
			}
			
			strtab->data = new_data;
			strtab->data_capacity = new_capacity;
		}
		
		// Add string
		memcpy((coil_u8_t*)strtab->data + size, str, len);
	}
	
	// Update size in header
	strtab->header.size += len;
	
	return offset;
}

/**
* Add a section to the object
*/
static coil_u16_t add_section_internal(
  coil_object_t* obj,
  const coil_section_header_t* header,
  const void* data,
  coil_u64_t data_size,
  coil_arena_t* arena
) {
  if (!obj || !header) {
    return 0;
  }
  
  // Using arena memory model
  if (obj->uses_arena) {
    // Allocate space for new sections array
    coil_section_t* new_sections = alloc_mem(
      arena, 
      (obj->header.section_count + 1) * sizeof(coil_section_t)
    );
    
    if (!new_sections) {
      COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate sections array");
      return 0;
    }
    
    // Copy existing sections
    if (obj->sections && obj->header.section_count > 0) {
      memcpy(new_sections, obj->sections, obj->header.section_count * sizeof(coil_section_t));
    }
    
    // Create new section
    coil_section_t* section = &new_sections[obj->header.section_count];
    memcpy(&section->header, header, sizeof(coil_section_header_t));
    
    // Initialize data pointers
    section->data = NULL;
    section->data_capacity = 0;
    section->header.size = 0;
    
    // Allocate and copy data if needed
    if (data_size > 0) {
      if (data) {
        section->data = push_data(arena, data, data_size);
        if (!section->data) {
          COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate section data");
          return 0;
        }
      } else {
        section->data = arena_alloc_default(arena, data_size);
        if (!section->data) {
          COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate section data");
          return 0;
        }
        memset(section->data, 0, data_size);
      }
      
      section->data_capacity = data_size;
      section->header.size = data_size;
    }
    
    // Update sections pointer
    obj->sections = new_sections;
    obj->sections_capacity = obj->header.section_count + 1;
  }
  // Using malloc memory model
  else {
    // Check if we need to resize sections array
    if (obj->header.section_count >= obj->sections_capacity) {
      size_t new_capacity = obj->sections_capacity * 2;
      if (new_capacity < 8) new_capacity = 8;
      
      coil_section_t* new_sections = realloc(obj->sections, new_capacity * sizeof(coil_section_t));
      if (!new_sections) {
        COIL_ERROR(COIL_ERR_NOMEM, "Failed to resize sections array");
        return 0;
      }
      
      obj->sections = new_sections;
      obj->sections_capacity = new_capacity;
    }
    
    // Create the new section
    coil_section_t section;
    memcpy(&section.header, header, sizeof(coil_section_header_t));
    
    // Initialize data pointers
    section.data = NULL;
    section.data_capacity = 0;
    section.header.size = 0;
    
    // Allocate and copy section data if needed
    if (data_size > 0) {
      section.data = malloc(data_size);
      if (!section.data) {
        COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate section data");
        return 0;
      }
      
      if (data) {
        memcpy(section.data, data, data_size);
      } else {
        memset(section.data, 0, data_size);
      }
      
      section.data_capacity = data_size;
      section.header.size = data_size;
    }
    
    // Add to sections array
    obj->sections[obj->header.section_count] = section;
  }
  
  // Update count and get section index
  coil_u16_t section_index = obj->header.section_count + 1; // Section index is 1-based
  obj->header.section_count++;
  
  // Update strtab and symtab indices if applicable
  if (header->type == COIL_SECTION_STRTAB) {
    obj->strtab_index = section_index;
  } else if (header->type == COIL_SECTION_SYMTAB) {
    obj->symtab_index = section_index;
  }
  
  return section_index;
}

coil_object_t* coil_object_create(coil_arena_t* arena) {
	coil_object_t* obj;
	
	if (arena) {
		obj = arena_alloc_default(arena, sizeof(coil_object_t));
	} else {
		obj = malloc(sizeof(coil_object_t));
	}
	
	if (!obj) {
		COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate object");
		return NULL;
	}
	
	// Initialize header
	init_object_header(&obj->header);
	
	// Initialize sections array
	obj->sections = NULL;
	obj->sections_capacity = 0;
	obj->strtab_index = 0;
	obj->symtab_index = 0;
	obj->uses_arena = (arena != NULL);
	
	return obj;
}

void coil_object_destroy(coil_object_t* obj, coil_arena_t* arena) {
	(void)arena;
	
	if (!obj) {
		return;
	}
	
	// Only free if not using arena
	if (!obj->uses_arena) {
		// Free all section data
		for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
			free(obj->sections[i].data);
		}
		
		// Free sections array
		free(obj->sections);
		
		// Free object
		free(obj);
	}
	
	// If using arena, we don't free anything as the arena owns all memory
}

coil_err_t coil_object_load_from_memory(
	coil_object_t* obj,
	const void* data,
	size_t size,
	coil_arena_t* arena
) {
	if (!obj || !data || size < sizeof(coil_object_header_t)) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Check if we're changing memory model
	if (obj->uses_arena != (arena != NULL)) {
		return COIL_ERROR(COIL_ERR_INVAL, "Cannot change memory model during load");
	}
	
	// Free existing sections if not using arena
	if (!obj->uses_arena) {
		for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
			free(obj->sections[i].data);
		}
		free(obj->sections);
		obj->sections = NULL;
		obj->sections_capacity = 0;
	}
	
	// Reset section tracking
	obj->strtab_index = 0;
	obj->symtab_index = 0;
	
	// Read header
	const coil_u8_t* ptr = (const coil_u8_t*)data;
	memcpy(&obj->header, ptr, sizeof(coil_object_header_t));
	ptr += sizeof(coil_object_header_t);
	
	// Validate magic number
	const coil_u8_t magic[] = COIL_MAGIC_BYTES;
	if (obj->header.magic[0] != magic[0] ||
			obj->header.magic[1] != magic[1] ||
			obj->header.magic[2] != magic[2] ||
			obj->header.magic[3] != magic[3]) {
		return COIL_ERROR(COIL_ERR_FORMAT, "Invalid magic number");
	}
	
	// Validate version
	if (obj->header.version != COIL_VERSION) {
		COIL_WARNING(COIL_ERR_FORMAT, "Version mismatch");
	}
	
	// Allocate sections array
	if (obj->uses_arena) {
		obj->sections = alloc_mem(arena, obj->header.section_count * sizeof(coil_section_t));
		obj->sections_capacity = obj->header.section_count;
	} else {
		obj->sections = malloc(obj->header.section_count * sizeof(coil_section_t));
		obj->sections_capacity = obj->header.section_count;
	}
	
	if (!obj->sections && obj->header.section_count > 0) {
		obj->header.section_count = 0;
		obj->sections_capacity = 0;
		return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate sections array");
	}
	
	// Read sections
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		// Read section header
		if (ptr + sizeof(coil_section_header_t) > (const coil_u8_t*)data + size) {
			return COIL_ERROR(COIL_ERR_FORMAT, "Truncated object file (section header)");
		}
		
		memcpy(&obj->sections[i].header, ptr, sizeof(coil_section_header_t));
		ptr += sizeof(coil_section_header_t);
		
		// Allocate and read section data
		coil_u64_t section_size = obj->sections[i].header.size;
		if (section_size > 0) {
			if (ptr + section_size > (const coil_u8_t*)data + size) {
				return COIL_ERROR(COIL_ERR_FORMAT, "Truncated object file (section data)");
			}
			
			if (obj->uses_arena) {
				obj->sections[i].data = push_data(arena, ptr, section_size);
			} else {
				obj->sections[i].data = malloc(section_size);
				if (obj->sections[i].data) {
					memcpy(obj->sections[i].data, ptr, section_size);
				}
			}
			
			if (!obj->sections[i].data) {
				return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate section data");
			}
			
			obj->sections[i].data_capacity = section_size;
			ptr += section_size;
		} else {
			obj->sections[i].data = NULL;
			obj->sections[i].data_capacity = 0;
		}
		
		// Update strtab and symtab indices if applicable
		if (obj->sections[i].header.type == COIL_SECTION_STRTAB) {
			obj->strtab_index = i + 1;
		} else if (obj->sections[i].header.type == COIL_SECTION_SYMTAB) {
			obj->symtab_index = i + 1;
		}
	}
	
	return COIL_ERR_GOOD;
}

coil_err_t coil_object_load_from_file(
	coil_object_t* obj,
	const char* filename,
	coil_arena_t* arena
) {
	if (!obj || !filename) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Open file
	FILE* file = fopen(filename, "rb");
	if (!file) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to open file");
	}
	
	// Get file size
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	if (file_size <= 0) {
		fclose(file);
		return COIL_ERROR(COIL_ERR_IO, "Invalid file size");
	}
	
	// Allocate buffer for file contents
	void* buffer;
	if (arena) {
		buffer = arena_alloc_default(arena, file_size);
	} else {
		buffer = malloc(file_size);
	}
	
	if (!buffer) {
		fclose(file);
		return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate buffer for file");
	}
	
	// Read file
	size_t read = fread(buffer, 1, file_size, file);
	fclose(file);
	
	if (read != (size_t)file_size) {
		if (!arena) {
			free(buffer);
		}
		return COIL_ERROR(COIL_ERR_IO, "Failed to read file");
	}
	
	// Load from memory
	coil_err_t err = coil_object_load_from_memory(obj, buffer, file_size, arena);
	
	// Free buffer if not using arena
	if (!arena) {
		free(buffer);
	}
	
	return err;
}

coil_err_t coil_object_save_to_memory(
	const coil_object_t* obj,
	coil_arena_t* arena,
	void** data,
	size_t* size
) {
	if (!obj || !data || !size || !arena) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Calculate total size
	size_t total_size = sizeof(coil_object_header_t);
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		total_size += sizeof(coil_section_header_t) + obj->sections[i].header.size;
	}
	
	// Allocate buffer
	void* buffer = arena_alloc_default(arena, total_size);
	if (!buffer) {
		return COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for object data");
	}
	
	// Create a copy of the header with updated file size
	coil_object_header_t header = obj->header;
	header.file_size = total_size;
	
	// Write header
	coil_u8_t* ptr = (coil_u8_t*)buffer;
	memcpy(ptr, &header, sizeof(coil_object_header_t));
	ptr += sizeof(coil_object_header_t);
	
	// Write sections
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		// Write section header
		memcpy(ptr, &obj->sections[i].header, sizeof(coil_section_header_t));
		ptr += sizeof(coil_section_header_t);
		
		// Write section data
		if (obj->sections[i].header.size > 0 && obj->sections[i].data) {
			memcpy(ptr, obj->sections[i].data, obj->sections[i].header.size);
			ptr += obj->sections[i].header.size;
		}
	}
	
	// Set output parameters
	*data = buffer;
	*size = total_size;
	
	return COIL_ERR_GOOD;
}

coil_err_t coil_object_save_to_file(
	const coil_object_t* obj,
	const char* filename
) {
	if (!obj || !filename) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Open file
	FILE* file = fopen(filename, "wb");
	if (!file) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to open file for writing");
	}
	
	// Create a copy of the header with updated file size
	coil_object_header_t header = obj->header;
	size_t total_size = sizeof(coil_object_header_t);
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		total_size += sizeof(coil_section_header_t) + obj->sections[i].header.size;
	}
	header.file_size = total_size;
	
	// Write header
	if (fwrite(&header, sizeof(coil_object_header_t), 1, file) != 1) {
		fclose(file);
		return COIL_ERROR(COIL_ERR_IO, "Failed to write object header");
	}
	
	// Write sections
	for (coil_u16_t i = 0; i < obj->header.section_count; i++) {
		// Write section header
		if (fwrite(&obj->sections[i].header, sizeof(coil_section_header_t), 1, file) != 1) {
			fclose(file);
			return COIL_ERROR(COIL_ERR_IO, "Failed to write section header");
		}
		
		// Write section data
		if (obj->sections[i].header.size > 0 && obj->sections[i].data) {
			if (fwrite(obj->sections[i].data, 1, obj->sections[i].header.size, file) != obj->sections[i].header.size) {
				fclose(file);
				return COIL_ERROR(COIL_ERR_IO, "Failed to write section data");
			}
		}
	}
	
	fclose(file);
	return COIL_ERR_GOOD;
}

const coil_object_header_t* coil_object_get_header(const coil_object_t* obj) {
	if (!obj) {
		return NULL;
	}
	
	return &obj->header;
}

coil_u16_t coil_object_get_section_count(const coil_object_t* obj) {
	if (!obj) {
		return 0;
	}
	
	return obj->header.section_count;
}

coil_err_t coil_object_init_string_table(coil_object_t* obj, coil_arena_t* arena) {
	if (!obj) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid object");
	}
	
	// Check if string table already exists
	if (obj->strtab_index > 0) {
		return COIL_ERR_GOOD;
	}
	
	// Check if memory model matches
	if (obj->uses_arena && !arena) {
		return COIL_ERROR(COIL_ERR_INVAL, "Arena required for arena-based object");
	}
	
	// Create string table section
	coil_section_header_t header;
	memset(&header, 0, sizeof(header));
	header.type = COIL_SECTION_STRTAB;
	
	// Add section (initially empty)
	coil_u16_t index = add_section_internal(obj, &header, NULL, 0, arena);
	if (index == 0) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to create string table");
	}
	
	// Add a single null byte to the section
	coil_section_t* strtab = &obj->sections[index - 1];
	uint8_t null_byte = 0;
	
	if (obj->uses_arena) {
		strtab->data = arena_alloc_default(arena, 1);
	} else {
		strtab->data = malloc(1);
	}
	
	if (!strtab->data) {
		return COIL_ERROR(COIL_ERR_NOMEM, "Failed to initialize string table data");
	}
	
	// Set the null byte
	*((uint8_t*)strtab->data) = null_byte;
	strtab->header.size = 1;
	strtab->data_capacity = 1;
	
	// Add the name ".strtab" to the string table (offset will be 1, after null byte)
	if (add_string_to_table(obj, ".strtab", arena) == 0) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to add string table name");
	}
	
	// Update the name in the section header
	strtab->header.name = 1; // offset past null byte
	
	return COIL_ERR_GOOD;
}

coil_err_t coil_object_init_symbol_table(coil_object_t* obj, coil_arena_t* arena) {
	if (!obj) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid object");
	}
	
	// Check if symbol table already exists
	if (obj->symtab_index > 0) {
		return COIL_ERR_GOOD;
	}
	
	// Check if memory model matches
	if (obj->uses_arena && !arena) {
		return COIL_ERROR(COIL_ERR_INVAL, "Arena required for arena-based object");
	}
	
	// Ensure string table exists
	coil_err_t err = coil_object_init_string_table(obj, arena);
	if (err != COIL_ERR_GOOD) {
		return err;
	}
	
	// Add symbol table name to string table
	coil_u64_t name = add_string_to_table(obj, ".symtab", arena);
	if (name == 0) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to add symbol table name");
	}
	
	// Create symbol table section
	coil_section_header_t header;
	memset(&header, 0, sizeof(header));
	header.name = name;
	header.type = COIL_SECTION_SYMTAB;
	
	// Add section (no initial data)
	coil_u16_t index = add_section_internal(obj, &header, NULL, 0, arena);
	if (index == 0) {
		return COIL_ERROR(COIL_ERR_IO, "Failed to create symbol table");
	}
	
	return COIL_ERR_GOOD;
}

coil_u64_t coil_object_add_string(
	coil_object_t* obj,
	const char* str,
	coil_arena_t* arena
) {
	if (!obj || !str) {
		COIL_ERROR(COIL_ERR_INVAL, "Invalid object or string");
		return 0;
	}
	
	// Check if memory model matches
	if (obj->uses_arena && !arena) {
		COIL_ERROR(COIL_ERR_INVAL, "Arena required for arena-based object");
		return 0;
	}
	
	// Initialize string table if needed
	if (obj->strtab_index == 0) {
		if (coil_object_init_string_table(obj, arena) != COIL_ERR_GOOD) {
			return 0;
		}
	}
	
	return add_string_to_table(obj, str, arena);
}

coil_err_t coil_object_get_string(
	const coil_object_t* obj,
	coil_u64_t offset,
	char* buffer,
	size_t buffer_size
) {
	if (!obj || !buffer || buffer_size == 0) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Clear buffer
	buffer[0] = '\0';
	
	// Check if string table exists
	if (obj->strtab_index == 0) {
		return COIL_ERROR(COIL_ERR_NOTFOUND, "String table not found");
	}
	
	// Get string table
	const coil_section_t* strtab = &obj->sections[obj->strtab_index - 1];
	
	// Check if offset is valid
	if (offset >= strtab->header.size) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid string offset");
	}
	
	// Get string
	const char* str = (const char*)((const coil_u8_t*)strtab->data + offset);
	
	// Calculate string length
	size_t len = 0;
	while (offset + len < strtab->header.size && str[len] != '\0') {
		len++;
	}
	
	// Check if string is null-terminated
	if (offset + len >= strtab->header.size) {
		return COIL_ERROR(COIL_ERR_FORMAT, "String not null-terminated");
	}
	
	// Copy string to buffer with truncation if needed
	size_t copy_len = (len < buffer_size - 1) ? len : buffer_size - 1;
	memcpy(buffer, str, copy_len);
	buffer[copy_len] = '\0';
	
	return COIL_ERR_GOOD;
}

coil_u16_t coil_object_add_section(
	coil_object_t* obj,
	coil_u64_t name_offset,
	coil_u16_t flags,
	coil_u8_t type,
	const void* data,
	coil_u64_t data_size,
	coil_arena_t* arena
) {
	if (!obj) {
		COIL_ERROR(COIL_ERR_INVAL, "Invalid object");
		return 0;
	}
	
	// Check if memory model matches
	if (obj->uses_arena && !arena) {
		COIL_ERROR(COIL_ERR_INVAL, "Arena required for arena-based object");
		return 0;
	}
	
	// Create section header
	coil_section_header_t header;
	header.name = name_offset;
	header.size = data_size;
	header.flags = flags;
	header.type = type;
	
	// Validate section type
	if (type == COIL_SECTION_STRTAB && obj->strtab_index > 0) {
		COIL_ERROR(COIL_ERR_EXISTS, "Multiple string tables not allowed");
		return 0;
	}
	
	if (type == COIL_SECTION_SYMTAB && obj->symtab_index > 0) {
		COIL_ERROR(COIL_ERR_EXISTS, "Multiple symbol tables not allowed");
		return 0;
	}
	
	return add_section_internal(obj, &header, data, data_size, arena);
}

coil_err_t coil_object_get_section(
	const coil_object_t* obj,
	coil_u16_t index,
	coil_section_header_t* header,
	const void** data,
	coil_u64_t* data_size
) {
	if (!obj || !header || index == 0 || index > obj->header.section_count) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Get section
	const coil_section_t* section = &obj->sections[index - 1];
	
	// Copy header
	memcpy(header, &section->header, sizeof(coil_section_header_t));
	
	// Set data pointer and size if requested
	if (data) {
		*data = section->data;
	}
	
	if (data_size) {
		*data_size = section->header.size;
	}
	
	return COIL_ERR_GOOD;
}

coil_u16_t coil_object_get_section_index(
	const coil_object_t* obj,
	const char* name
) {
	if (!obj || !name) {
		return 0;
	}
	
	return find_section_by_name(obj, name);
}

coil_u16_t coil_object_add_symbol(
	coil_object_t* obj,
	coil_u64_t name,
	coil_u32_t value,
	coil_u16_t section_index,
	coil_u8_t type,
	coil_u8_t binding,
	coil_arena_t* arena
) {
	if (!obj) {
		COIL_ERROR(COIL_ERR_INVAL, "Invalid object");
		return 0;
	}
	
	// Check if memory model matches
	if (obj->uses_arena && !arena) {
		COIL_ERROR(COIL_ERR_INVAL, "Arena required for arena-based object");
		return 0;
	}
	
	// Initialize symbol table if needed
	if (obj->symtab_index == 0) {
		if (coil_object_init_symbol_table(obj, arena) != COIL_ERR_GOOD) {
			return 0;
		}
	}
	
	// Get symbol table
	coil_section_t* symtab = &obj->sections[obj->symtab_index - 1];
	
	// Create symbol
	coil_symbol_t symbol;
	symbol.name = name;
	symbol.value = value;
	symbol.section_index = section_index;
	symbol.type = type;
	symbol.binding = binding;
	
	// Using arena
	if (obj->uses_arena) {
		// Allocate new buffer for symbols
		coil_u64_t new_size = symtab->header.size + sizeof(coil_symbol_t);
		coil_u64_t symbol_count = symtab->header.size / sizeof(coil_symbol_t);
		
		void* new_data;
		if (symtab->header.size == 0) {
			// First symbol
			new_data = arena_alloc_default(arena, sizeof(coil_symbol_t));
			if (!new_data) {
				COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for symbol table");
				return 0;
			}
			memcpy(new_data, &symbol, sizeof(coil_symbol_t));
		} else {
			// Subsequent symbols
			new_data = arena_alloc_default(arena, new_size);
			if (!new_data) {
				COIL_ERROR(COIL_ERR_NOMEM, "Failed to allocate memory for symbol table");
				return 0;
			}
			
			// Copy existing symbols
			memcpy(new_data, symtab->data, symtab->header.size);
			
			// Add new symbol
			memcpy((coil_u8_t*)new_data + symtab->header.size, &symbol, sizeof(coil_symbol_t));
		}
		
		// Update section
		symtab->data = new_data;
		symtab->data_capacity = new_size;
		symtab->header.size = new_size;
		
		return (coil_u16_t)(symbol_count + 1);
	}
	// Using malloc
	else {
		// Calculate new size
		coil_u64_t new_size = symtab->header.size + sizeof(coil_symbol_t);
		
		// Check if we need to resize
		if (new_size > symtab->data_capacity) {
			size_t new_capacity = (symtab->data_capacity * 2) + sizeof(coil_symbol_t);
			void* new_data;
			
			if (symtab->data == NULL) {
				new_data = malloc(new_capacity);
			} else {
				new_data = realloc(symtab->data, new_capacity);
			}
			
			if (!new_data) {
				COIL_ERROR(COIL_ERR_NOMEM, "Failed to resize symbol table");
				return 0;
			}
			
			symtab->data = new_data;
			symtab->data_capacity = new_capacity;
		}
		
		// Add symbol
		memcpy((coil_u8_t*)symtab->data + symtab->header.size, &symbol, sizeof(coil_symbol_t));
		
		// Update size
		symtab->header.size = new_size;
		
		// Return index (1-based)
		return (coil_u16_t)(symtab->header.size / sizeof(coil_symbol_t));
	}
}

coil_err_t coil_object_get_symbol(
	const coil_object_t* obj,
	coil_u16_t index,
	coil_symbol_t* symbol
) {
	if (!obj || !symbol) {
		return COIL_ERROR(COIL_ERR_INVAL, "Invalid arguments");
	}
	
	// Check if symbol table exists
	if (obj->symtab_index == 0) {
		return COIL_ERROR(COIL_ERR_NOTFOUND, "Symbol table not found");
	}
	
	// Get symbol table
	const coil_section_t* symtab = &obj->sections[obj->symtab_index - 1];
	
	// Check if index is valid
	coil_u64_t count = symtab->header.size / sizeof(coil_symbol_t);
	if (index == 0 || index > count) {
		return COIL_ERROR(COIL_ERR_NOTFOUND, "Symbol index out of range");
	}
	
	// Get symbol
	const coil_symbol_t* src = (const coil_symbol_t*)symtab->data + (index - 1);
	memcpy(symbol, src, sizeof(coil_symbol_t));
	
	return COIL_ERR_GOOD;
}

coil_u16_t coil_object_get_symbol_index(
	const coil_object_t* obj,
	const char* name
) {
	if (!obj || !name || obj->symtab_index == 0 || obj->strtab_index == 0) {
		return 0;
	}
	
	// Get symbol table
	const coil_section_t* symtab = &obj->sections[obj->symtab_index - 1];
	
	// Get count of symbols
	coil_u64_t count = symtab->header.size / sizeof(coil_symbol_t);
	
	// Iterate through symbols
	for (coil_u64_t i = 0; i < count; i++) {
		const coil_symbol_t* symbol = (const coil_symbol_t*)symtab->data + i;
		
		// Get symbol name
		char symbol_name[256] = {0};
		if (coil_object_get_string(obj, symbol->name, symbol_name, sizeof(symbol_name)) != COIL_ERR_GOOD) {
			continue;
		}
		
		// Compare names
		if (strcmp(symbol_name, name) == 0) {
			return (coil_u16_t)(i + 1); // 1-based index
		}
	}
	
	return 0; // Not found
}