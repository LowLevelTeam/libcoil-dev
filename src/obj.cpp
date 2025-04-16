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

Object Object::create(ObjType type) {
  Object obj;
  obj.type = static_cast<u16>(type);
  obj.section_count = 0;
  obj.str_table_index = 0;
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
      
      obj.section_count++;
  }
  
  return Result::Success;
}

Result Object::save(Stream& stream) const {
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
  
  // Write each section
  for (u16 i = 0; i < section_count; i++) {
      Section& section = const_cast<Section&>(sections[i]);
      
      // Set the section data offset
      section.header.offset = static_cast<u32>(currentOffset);
      
      // Write the section
      Result result = writeSection(stream, section);
      if (result != Result::Success) {
          return result;
      }
      
      // Update current offset for next section
      currentOffset += section.header.size;
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
  // Find the symbol table section
  Section* symtab = nullptr;
  for (u16 i = 0; i < section_count; i++) {
      if (static_cast<SectionType>(sections[i].header.type) == SectionType::SymTab) {
          symtab = &sections[i];
          break;
      }
  }
  
  // If no symbol table exists, create one
  if (!symtab) {
      // First, make sure we have a string table
      if (str_table_index == 0) {
          // Create a string table
          const u8* empty = reinterpret_cast<const u8*>("");
          Result result = addSection(".strtab", SectionType::StrTab, SectionFlag::Strings, empty, 1);
          if (result != Result::Success) {
              return result;
          }
      }
      
      // Create a symbol table
      Result result = addSection(".symtab", SectionType::SymTab, SectionFlag::None, nullptr, 0);
      if (result != Result::Success) {
          return result;
      }
      
      symtab = &sections[section_count - 1];
      symtab->header.entry_size = sizeof(Symbol);
      symtab->header.link = str_table_index; // Link to string table
  }
  
  // Add the name to the string table (not implemented here)
  // This would require modifying the string table data, which is beyond the scope of this implementation
  
  // For now, we'll just return NotSupported
  return makeError(Result::NotSupported, ErrorLevel::Error, 
                  "Symbol addition not fully implemented");
}

const Symbol* Object::findSymbol(const char* name) const {
  // Find the symbol table section
  const Section* symtab = nullptr;
  for (u16 i = 0; i < section_count; i++) {
      if (static_cast<SectionType>(sections[i].header.type) == SectionType::SymTab) {
          symtab = &sections[i];
          break;
      }
  }
  
  if (!symtab || !symtab->data) {
      return nullptr;
  }
  
  // Get the string table
  if (str_table_index >= section_count) {
      return nullptr;
  }
  
  const Section* strtab = &sections[str_table_index];
  if (!strtab->data) {
      return nullptr;
  }
  
  // Search for the symbol by name
  // Note: This is a simplified implementation and assumes the string table 
  // is properly formatted with null-terminated strings
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
  
  return nullptr;
}

Result Object::addRelocation(u16 section_index, u32 offset, u32 symbol_index,
                          RelocationType type, u8 size, i32 addend) {
  // Check if the section exists
  if (section_index >= section_count) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Invalid section index %u", section_index);
  }
  
  // Find or create a relocation table for this section
  Section* reltab = nullptr;
  char relSecName[32];
  snprintf(relSecName, sizeof(relSecName), ".rel%s", sections[section_index].name);
  
  for (u16 i = 0; i < section_count; i++) {
      if (strcmp(sections[i].name, relSecName) == 0) {
          reltab = &sections[i];
          break;
      }
  }
  
  // If no relocation table exists, create one
  if (!reltab) {
      Result result = addSection(relSecName, SectionType::RelTable, SectionFlag::None, nullptr, 0);
      if (result != Result::Success) {
          return result;
      }
      
      reltab = &sections[section_count - 1];
      reltab->header.entry_size = sizeof(Relocation);
      reltab->header.info = section_index; // Reference to the section being relocated
  }
  
  // Add the relocation entry (not implemented here)
  // This would require modifying the relocation table data, which is beyond the scope of this implementation
  
  // For now, we'll just return NotSupported
  return makeError(Result::NotSupported, ErrorLevel::Error, 
                  "Relocation addition not fully implemented");
}

} // namespace coil