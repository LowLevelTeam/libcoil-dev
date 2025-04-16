/**
* @file obj.cpp
* @brief Implementation of COIL object format
*/

#include "coil/obj.hpp"
#include "coil/err.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace coil {

/**
* @brief Object file header
*/
struct ObjectHeader {
  u32 magic;         ///< Magic number 'COIL'
  u16 version;       ///< Format version
  u16 type;          ///< Object type
  u16 section_count; ///< Number of sections
  u16 flags;         ///< Object flags
};

// Magic number for identifying COIL object files
constexpr u32 COIL_MAGIC = 0x434F494C; // 'COIL' in ASCII
constexpr u16 COIL_VERSION = 0x0001;   // Version 0.1

// Internal helper functions
namespace {
  
Result writeSection(Stream& stream, const Section& section) {
  // Write section header
  if (stream.writeValue(section.header) != Result::Success) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to write section header for %s", section.name);
  }
  
  // Write section name (fixed size)
  if (stream.write(section.name, sizeof(section.name)) != sizeof(section.name)) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to write section name for %s", section.name);
  }
  
  // Calculate section data position
  size_t dataPos = section.header.offset;
  size_t currentPos = stream.tell();
  
  // Seek to section data position
  if (dataPos != currentPos) {
      if (stream.seek(dataPos) != Result::Success) {
          return makeError(Result::IoError, ErrorLevel::Error, 
                          "Failed to seek to section data position for %s", section.name);
      }
  }
  
  // Write section data if present
  if (section.header.size > 0 && section.data != nullptr) {
      if (stream.write(section.data, section.header.size) != section.header.size) {
          return makeError(Result::IoError, ErrorLevel::Error, 
                          "Failed to write section data for %s", section.name);
      }
  }
  
  return Result::Success;
}

Result readSection(Stream& stream, Section& section) {
  // Read section header
  if (stream.readValue(section.header) != Result::Success) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to read section header");
  }
  
  // Read section name (fixed size)
  if (stream.read(section.name, sizeof(section.name)) != sizeof(section.name)) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to read section name");
  }
  
  // Section data will be read later when needed
  section.data = nullptr;
  
  return Result::Success;
}

} // anonymous namespace

// Initialize an empty object
Object::Object() 
  : type(static_cast<u16>(ObjType::None)), 
    section_count(0), 
    str_table_index(0),
    sym_table_index(0),
    symbol_count(0),
    relocation_count(0),
    strtab_size(0) {
    
    // Initialize the string table with a null byte
    strtab_buffer[0] = '\0';
    strtab_size = 1;
}

// Destructor for Object
Object::~Object() {
    // No dynamic memory to free in basic implementation
}

Object Object::create(ObjType type) {
  Object obj;
  obj.type = static_cast<u16>(type);
  obj.section_count = 0;
  obj.str_table_index = 0;
  obj.sym_table_index = 0;
  obj.symbol_count = 0;
  obj.relocation_count = 0;
  
  // Initialize the string table with a null byte
  obj.strtab_buffer[0] = '\0';
  obj.strtab_size = 1;
  
  return obj;
}

Result Object::load(Stream& stream, Object& obj) {
  // Read object header
  ObjectHeader header;
  if (stream.readValue(header) != Result::Success) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to read object header");
  }
  
  // Verify magic number
  if (header.magic != COIL_MAGIC) {
      return makeError(Result::InvalidFormat, ErrorLevel::Error, 
                      "Invalid object file format: incorrect magic number");
  }
  
  // Verify version
  if (header.version > COIL_VERSION) {
      return makeError(Result::NotSupported, ErrorLevel::Error, 
                      "Unsupported object file version: %d", header.version);
  }
  
  // Set object properties
  obj.type = header.type;
  obj.section_count = 0; // Will be incremented as sections are loaded
  
  // Read all sections
  for (u16 i = 0; i < header.section_count; i++) {
      // Check if we have room for another section
      if (obj.section_count >= MAX_SECTIONS) {
          reportError(ErrorLevel::Warning, 
                      "Object file contains more sections than supported maximum");
          break;
      }
      
      // Read section
      Section& section = obj.sections[obj.section_count];
      Result result = readSection(stream, section);
      if (result != Result::Success) {
          return result;
      }
      
      // Check for string table section
      if (static_cast<SectionType>(section.header.type) == SectionType::StrTab) {
          obj.str_table_index = obj.section_count;
      }
      
      // Check for symbol table section
      if (static_cast<SectionType>(section.header.type) == SectionType::SymTab) {
          obj.sym_table_index = obj.section_count;
      }
      
      obj.section_count++;
  }
  
  return Result::Success;
}

u32 Object::addStringToTable(const char* str) {
  if (!str) return 0; // Return offset 0 for null strings
  
  // Check if string already exists in the table
  u32 offset = 1; // Start after null byte at offset 0
  while (offset < strtab_size) {
      if (strcmp(&strtab_buffer[offset], str) == 0) {
          return offset;
      }
      
      // Move to next string
      offset += strlen(&strtab_buffer[offset]) + 1;
  }
  
  // String not found, add it
  size_t len = strlen(str) + 1; // Include null terminator
  
  // Check if it would fit in the buffer
  if (strtab_size + len > INITIAL_STRTAB_SIZE) {
      reportError(ErrorLevel::Error, "String table overflow");
      return 0;
  }
  
  // Add the string
  u32 new_offset = strtab_size;
  memcpy(&strtab_buffer[strtab_size], str, len);
  strtab_size += len;
  
  return new_offset;
}

u16 Object::findOrCreateSection(const char* name, SectionType type, SectionFlag flags) {
  // First try to find an existing section
  for (u16 i = 0; i < section_count; i++) {
      if (strcmp(sections[i].name, name) == 0) {
          return i;
      }
  }
  
  // If not found, create a new one
  if (section_count >= MAX_SECTIONS) {
      reportError(ErrorLevel::Error, "Maximum number of sections reached");
      return MAX_SECTIONS;
  }
  
  // Initialize with empty data, will be populated later
  Result result = addSection(name, type, flags, nullptr, 0);
  
  if (result != Result::Success) {
      return MAX_SECTIONS;
  }
  
  return section_count - 1;
}

Result Object::finalizeTablesForSave() {
  // Create or update string table
  if (str_table_index == 0 || str_table_index >= section_count) {
      // If no string table exists, we need to create one
      // This should never happen in a proper implementation since
      // we initialize a string table on object creation
      if (section_count >= MAX_SECTIONS) {
          return makeError(Result::InvalidArg, ErrorLevel::Error, 
                          "Cannot create string table: maximum sections reached");
      }
      
      Section& strSection = sections[section_count];
      memset(&strSection, 0, sizeof(Section));
      strncpy(strSection.name, ".strtab", sizeof(strSection.name) - 1);
      
      strSection.header.type = static_cast<u32>(SectionType::StrTab);
      strSection.header.flags = static_cast<u32>(SectionFlag::Strings);
      strSection.header.size = strtab_size;
      strSection.data = reinterpret_cast<const u8*>(strtab_buffer);
      
      const_cast<Object*>(this)->str_table_index = section_count;
      const_cast<Object*>(this)->section_count++;
  } else {
      // Update the existing string table
      Section& strSection = sections[str_table_index];
      strSection.header.size = strtab_size;
      strSection.data = reinterpret_cast<const u8*>(strtab_buffer);
  }
  
  // Create or update symbol table if we have symbols
  if (symbol_count > 0) {
      // Allocate memory for the symbol table data
      Symbol* symtab_data = static_cast<Symbol*>(malloc(symbol_count * sizeof(Symbol)));
      if (!symtab_data) {
          return makeError(Result::OutOfMemory, ErrorLevel::Error, 
                          "Failed to allocate memory for symbol table");
      }
      
      // Copy symbols
      for (u32 i = 0; i < symbol_count; i++) {
          symtab_data[i] = symbols[i].symbol;
      }
      
      // Find or create symbol table section
      u16 symtab_idx = const_cast<Object*>(this)->findOrCreateSection(
          ".symtab", SectionType::SymTab, SectionFlag::None);
      
      if (symtab_idx >= section_count) {
          free(symtab_data);
          return makeError(Result::InvalidArg, ErrorLevel::Error, 
                          "Failed to create symbol table section");
      }
      
      // Update section data
      Section& symSection = sections[symtab_idx];
      symSection.header.size = symbol_count * sizeof(Symbol);
      symSection.header.entry_size = sizeof(Symbol);
      symSection.header.link = str_table_index; // Link to string table
      symSection.data = reinterpret_cast<const u8*>(symtab_data);
      
      const_cast<Object*>(this)->sym_table_index = symtab_idx;
  }
  
  // Process relocations if we have any
  if (relocation_count > 0) {
      // Group relocations by section
      for (u32 i = 0; i < relocation_count; i++) {
          u16 sec_idx = relocations[i].section_index;
          
          // Create relocation section name
          char relname[40];
          snprintf(relname, sizeof(relname), ".rel%s", sections[sec_idx].name);
          
          // Find or create relocation section
          u16 rel_sec_idx = const_cast<Object*>(this)->findOrCreateSection(
              relname, SectionType::RelTable, SectionFlag::None);
          
          if (rel_sec_idx >= section_count) {
              continue;
          }
          
          // Count relocations for this section
          u32 sec_rel_count = 0;
          for (u32 j = 0; j < relocation_count; j++) {
              if (relocations[j].section_index == sec_idx) {
                  sec_rel_count++;
              }
          }
          
          // Allocate memory for relocations
          Relocation* rel_data = static_cast<Relocation*>(
              malloc(sec_rel_count * sizeof(Relocation)));
          
          if (!rel_data) {
              return makeError(Result::OutOfMemory, ErrorLevel::Error, 
                              "Failed to allocate memory for relocation table");
          }
          
          // Fill relocation data
          u32 rel_idx = 0;
          for (u32 j = 0; j < relocation_count; j++) {
              if (relocations[j].section_index == sec_idx) {
                  rel_data[rel_idx++] = relocations[j].relocation;
              }
          }
          
          // Update section
          Section& relSection = sections[rel_sec_idx];
          relSection.header.size = sec_rel_count * sizeof(Relocation);
          relSection.header.entry_size = sizeof(Relocation);
          relSection.header.info = sec_idx;
          relSection.data = reinterpret_cast<const u8*>(rel_data);
      }
  }
  
  return Result::Success;
}

Result Object::save(Stream& stream) {
  // Finalize symbol and string tables
  Result finalize_result = finalizeTablesForSave();
  if (finalize_result != Result::Success) {
      return finalize_result;
  }
  
  // Create and write object header
  ObjectHeader header;
  header.magic = COIL_MAGIC;
  header.version = COIL_VERSION;
  header.type = type;
  header.section_count = section_count;
  header.flags = 0; // Reserved for future use
  
  if (stream.writeValue(header) != Result::Success) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to write object header");
  }
  
  // Calculate section data offsets
  size_t headerSize = sizeof(ObjectHeader);
  size_t sectionHeadersSize = section_count * (sizeof(SectionHeader) + sizeof(sections[0].name));
  size_t currentOffset = headerSize + sectionHeadersSize;
  
  // Update offsets for sections
  for (u16 i = 0; i < section_count; i++) {
      Section& section = const_cast<Section&>(sections[i]);
      section.header.offset = static_cast<u32>(currentOffset);
      currentOffset += section.header.size;
  }
  
  // Write each section
  for (u16 i = 0; i < section_count; i++) {
      Result result = writeSection(stream, sections[i]);
      if (result != Result::Success) {
          return result;
      }
  }
  
  // Clean up temporary buffers used for saving
  for (u16 i = 0; i < section_count; i++) {
      Section& section = const_cast<Section&>(sections[i]);
      SectionType type = static_cast<SectionType>(section.header.type);
      
      // Check if this is a section we created during save
      if ((type == SectionType::SymTab || type == SectionType::RelTable) &&
          section.data != nullptr) {
          free(const_cast<u8*>(section.data));
          section.data = nullptr;
      }
  }
  
  return Result::Success;
}

Result Object::addSection(const char* name, SectionType type, SectionFlag flags, 
                        const u8* data, u32 size) {
  // Check if we have room for another section
  if (section_count >= MAX_SECTIONS) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Maximum number of sections (%zu) reached", MAX_SECTIONS);
  }
  
  // Check name length
  if (strlen(name) >= sizeof(sections[0].name)) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Section name '%s' is too long", name);
  }
  
  // Initialize the section
  Section& section = sections[section_count];
  memset(&section, 0, sizeof(Section));
  
  // Copy the name
  strncpy(section.name, name, sizeof(section.name) - 1);
  section.name[sizeof(section.name) - 1] = '\0';
  
  // Set header fields
  section.header.type = static_cast<u32>(type);
  section.header.flags = static_cast<u32>(flags);
  section.header.size = size;
  section.header.align = 4; // Default alignment
  
  // Set data pointer
  section.data = data;
  
  // If this is a string table, remember its index
  if (type == SectionType::StrTab) {
      str_table_index = section_count;
  }
  
  // If this is a symbol table, remember its index
  if (type == SectionType::SymTab) {
      sym_table_index = section_count;
  }
  
  // Increment section count
  section_count++;
  
  return Result::Success;
}

const Section* Object::getSection(const char* name) const {
  // Search for section by name
  for (u16 i = 0; i < section_count; i++) {
      if (strcmp(sections[i].name, name) == 0) {
          return &sections[i];
      }
  }
  
  return nullptr;
}

const Section* Object::getSection(u16 index) const {
  if (index < section_count) {
      return &sections[index];
  }
  
  return nullptr;
}

Result Object::addSymbol(const char* name, u32 value, u32 size, 
                      SymbolType type, SymbolBinding binding, u16 section_index) {
  // Validate section index
  if (section_index >= section_count && section_index != 0) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Invalid section index %u for symbol '%s'", 
                      section_index, name);
  }
  
  // Check if we have room for another symbol
  if (symbol_count >= MAX_SYMBOLS) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Maximum number of symbols reached");
  }
  
  // Add the symbol
  SymbolEntry& entry = symbols[symbol_count];
  
  // Set symbol properties
  entry.symbol.value = value;
  entry.symbol.size = size;
  entry.symbol.setTypeAndBinding(type, binding);
  entry.symbol.other = 0;
  entry.symbol.section_index = section_index;
  
  // Store name pointer for later addition to string table
  entry.name = name;
  
  // Add name to string table and get offset
  entry.symbol.name = addStringToTable(name);
  
  // Increment symbol count
  symbol_count++;
  
  return Result::Success;
}

const Symbol* Object::findSymbol(const char* name) const {
  // First check our buffered symbols
  for (u32 i = 0; i < symbol_count; i++) {
      if (strcmp(symbols[i].name, name) == 0) {
          return &symbols[i].symbol;
      }
  }
  
  // If not found and we have a symbol table section, check there
  if (sym_table_index < section_count) {
      const Section* symtab = &sections[sym_table_index];
      if (symtab->data) {
          // Get the string table
          if (str_table_index >= section_count) {
              return nullptr;
          }
          
          const Section* strtab = &sections[str_table_index];
          if (!strtab->data) {
              return nullptr;
          }
          
          // Search for the symbol by name
          const Symbol* symbols = reinterpret_cast<const Symbol*>(symtab->data);
          u32 numSymbols = symtab->header.size / sizeof(Symbol);
          
          for (u32 i = 0; i < numSymbols; i++) {
              const Symbol& sym = symbols[i];
              if (sym.name < strtab->header.size) {
                  const char* symName = reinterpret_cast<const char*>(strtab->data + sym.name);
                  if (strcmp(symName, name) == 0) {
                      return &sym;
                  }
              }
          }
      }
  }
  
  return nullptr;
}

Result Object::addRelocation(u16 section_index, u32 offset, u32 symbol_index,
                          RelocationType type, u8 size, i32 addend) {
  // Check if the section exists
  if (section_index >= section_count) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Invalid section index %u", section_index);
  }
  
  // Check if we have room for another relocation
  if (relocation_count >= MAX_RELOCATIONS) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Maximum number of relocations reached");
  }
  
  // Add the relocation
  RelocationEntry& entry = relocations[relocation_count];
  entry.section_index = section_index;
  
  // Set relocation properties
  entry.relocation.offset = offset;
  entry.relocation.symbol_index = symbol_index;
  entry.relocation.type = static_cast<u8>(type);
  entry.relocation.size = size;
  entry.relocation.addend = addend;
  
  // Increment relocation count
  relocation_count++;
  
  return Result::Success;
}

} // namespace coil