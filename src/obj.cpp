/**
 * @file obj.cpp
 * @brief Implementation of COIL optimized object format
 */

#include "coil/obj.hpp"
#include "coil/err.hpp"
#include <cstdio>
#include <cstring>

namespace coil {
  // Constructor
  Object::Object() {}
  
  // Initialize with type
  void Object::init() {
    // Initialize string and symbol tables
    initStringTable();
    initSymbolTable();
  }
  
  // -------------------------------- Stream Functionality -------------------------------- //
  
  Result Object::load(Stream& stream) {
    // Load Header
    if (stream.readValue(header) != Result::Success) {
      COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, "Failed to read object header");
    }
    
    // Validate header
    if (header.magic[0] != COIL_MAGIC[0] ||
        header.magic[1] != COIL_MAGIC[1] ||
        header.magic[2] != COIL_MAGIC[2] ||
        header.magic[3] != COIL_MAGIC[3]) {
      COIL_RETURN_ERROR(Result::InvalidFormat, ErrorLevel::Error, "Invalid object format magic number");
    }
    
    if (header.version != COIL_VERSION) {
      COIL_REPORT_ERROR(ErrorLevel::Warning, "Object format version mismatch. File: %hu, Library: %hu", header.version, COIL_VERSION);
    }
    
    // Resize sections vector
    sections.resize(header.section_count);
    
    // Load Sections
    for (size_t i = 0; i < header.section_count; ++i) {
      Section& section = sections[i];
      
      // Load Section Header
      if (stream.readValue(section.header) != Result::Success) {
        COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, 
                                 "Failed to read section header");
      }
      
      // Resize section data
      section.data.resize(section.header.size);
      
      // Load Section Data
      size_t bytesread = stream.read(section.data.data(), section.header.size);
      if (bytesread != section.header.size) {
        COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, 
                                 "Failed to read section data");
      }
      
      // Check for string table and symbol table
      if (static_cast<SectionType>(section.header.type) == SectionType::StrTab) {
        if (strtab) {
          COIL_REPORT_ERROR(ErrorLevel::Warning,
                           "Multiple string tables found in object file");
        } else {
          strtab = &sections[i];
        }
      } else if (static_cast<SectionType>(section.header.type) == SectionType::SymTab) {
        // Store symbol data
        symbol_data = section.data;
        setupSymbolTable();
      }
    }
    
    return Result::Success;
  }
  
  Result Object::save(Stream& stream) {
    // Update header file size and section count
    header.section_count = static_cast<u16>(sections.size());
    header.file_size = sizeof(ObjectHeader);
    
    // Calculate total file size
    for (const Section& section : sections) {
      header.file_size += sizeof(SectionHeader) + section.header.size;
    }
    
    // Write Header
    if (stream.writeValue(header) != Result::Success) {
      COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, 
                              "Failed to write object header");
    }
    
    // Write Sections
    for (const Section& section : sections) {
      // Write Section Header
      if (stream.writeValue(section.header) != Result::Success) {
        COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, 
                                "Failed to write section header");
      }
      
      // Write Section Data
      size_t byteswritten = stream.write(section.data.data(), section.header.size);
      if (byteswritten != section.header.size) {
        COIL_RETURN_ERROR(Result::IoError, ErrorLevel::Error, 
                                "Failed to write section data");
      }
    }
    
    return Result::Success;
  }
  
  // -------------------------------- Section Functionality -------------------------------- //
  
  u16 Object::addSection(const char* name, SectionType type, SectionFlag flags, 
                         const void* data, size_t size) {
    // Check if there's a string table
    if (!strtab && type != SectionType::StrTab) {
      if (initStringTable() != Result::Success) {
        return 0;
      }
    }
    
    // Add the name to string table
    u64 name_offset = 0;
    if (name) {
      name_offset = addString(name);
      if (name_offset == 0 && type != SectionType::StrTab) {
        return 0;
      }
    }
    
    // Create new section
    Section section;
    SectionHeader& header = section.header;
    header.name_offset = name_offset;
    header.size = size;
    header.reserved = 0;
    header.flags = static_cast<u16>(flags);
    header.type = static_cast<u8>(type);
    
    // Add data if provided
    if (data && size > 0) {
      section.data.resize(size);
      std::memcpy(section.data.data(), data, size);
    } else {
      section.data.clear();
    }
    
    // Add to sections
    sections.push_back(section);
    
    // If this is a string table and we don't have one yet, set it
    if (type == SectionType::StrTab && !strtab) {
      strtab = &sections.back();
    }
    
    // If this is a symbol table, update our symbols pointer
    if (type == SectionType::SymTab) {
      symbol_data = section.data;
      setupSymbolTable();
    }
    
    return static_cast<u16>(sections.size());
  }
  
  u16 Object::getSectionIndex(const char* name, size_t namelen) {
    // Need string table
    if (!strtab) {
      return 0;
    }
    
    // Search sections
    for (size_t i = 0; i < sections.size(); ++i) {
      // Get string table offset for section name
      u64 sectnameoff = sections[i].header.name_offset;
      
      // Get section name from string table
      const char* sectname = getString(sectnameoff);
      if (!sectname) {
        continue;
      }
      
      // Compare section name with expected
      if (std::strncmp(sectname, name, namelen) == 0) {
        return static_cast<u16>(i + 1); // 1-indexed
      }
    }
    
    return 0; // Not found
  }
  
  u16 Object::getSectionIndex(const char* name) {
    return getSectionIndex(name, std::strlen(name));
  }
  
  Section* Object::getSection(u16 index) {
    if (index == 0 || index > sections.size()) {
      return nullptr;
    }
    return &sections[index - 1]; // Convert from 1-indexed to 0-indexed
  }
  
  const Section* Object::getSection(u16 index) const {
    if (index == 0 || index > sections.size()) {
      return nullptr;
    }
    return &sections[index - 1]; // Convert from 1-indexed to 0-indexed
  }
  
  // -------------------------------- Symbol Table Functionality -------------------------------- //
  
  u16 Object::addSymbol(const char* name, u32 value, u16 section_index, 
                        SymbolType type, SymbolBinding binding) {
    // Ensure we have a string table
    if (!strtab) {
      if (initStringTable() != Result::Success) {
        return 0;
      }
    }
    
    // Ensure we have a symbol table
    if (!symbols) {
      if (initSymbolTable() != Result::Success) {
        return 0;
      }
    }
    
    // Add the name to string table
    u64 name_offset = 0;
    if (name) {
      name_offset = addString(name);
      if (name_offset == 0) {
        return 0;
      }
    }
    
    // Check if we need to resize the symbol array
    if (symbol_count >= symbol_capacity) {
      size_t new_capacity = symbol_capacity * 2;
      if (new_capacity == 0) {
        new_capacity = 16;
      }
      
      // Resize the symbol data vector
      symbol_data.resize(new_capacity * sizeof(Symbol));
      symbols = reinterpret_cast<Symbol*>(symbol_data.data());
      symbol_capacity = new_capacity;
      
      // Update section size
      for (size_t i = 0; i < sections.size(); ++i) {
        if (static_cast<SectionType>(sections[i].header.type) == SectionType::SymTab) {
          sections[i].header.size = symbol_data.size();
          break;
        }
      }
    }
    
    // Create new symbol
    Symbol& symbol = symbols[symbol_count++];
    symbol.name = name_offset;
    symbol.value = value;
    symbol.section_index = section_index;
    symbol.type = static_cast<u8>(type);
    symbol.binding = static_cast<u8>(binding);
    
    return static_cast<u16>(symbol_count); // Return 1-indexed
  }
  
  u16 Object::getSymbolIndex(const char* name, size_t namelen) {
    // Need symbols and string table
    if (!symbols || !strtab) {
      return 0;
    }
    
    // Search symbols
    for (size_t i = 0; i < symbol_count; ++i) {
      // Get string offset
      u64 symnameoff = symbols[i].name;
      
      // Get symbol name
      const char* symname = getString(symnameoff);
      if (!symname) {
        continue;
      }
      
      // Compare with expected
      if (std::strncmp(symname, name, namelen) == 0) {
        return static_cast<u16>(i + 1); // 1-indexed
      }
    }
    
    return 0; // Not found
  }
  
  u16 Object::getSymbolIndex(const char* name) {
    return getSymbolIndex(name, std::strlen(name));
  }
  
  Symbol* Object::getSymbol(u16 index) {
    if (index == 0 || index > symbol_count || !symbols) {
      return nullptr;
    }
    return &symbols[index - 1]; // Convert from 1-indexed to 0-indexed
  }
  
  const Symbol* Object::getSymbol(u16 index) const {
    if (index == 0 || index > symbol_count || !symbols) {
      return nullptr;
    }
    return &symbols[index - 1]; // Convert from 1-indexed to 0-indexed
  }
  
  // -------------------------------- String Table Functionality -------------------------------- //
  
  u64 Object::addString(const char* str) {
    // Need string table
    if (!strtab) {
      if (initStringTable() != Result::Success) {
        return 0;
      }
    }
    
    // Get current size as the offset
    u64 offset = strtab->data.size();
    size_t len = std::strlen(str) + 1; // Include null terminator
    
    // Add to string table
    strtab->data.resize(offset + len);
    std::memcpy(strtab->data.data() + offset, str, len);
    
    // Update section size
    strtab->header.size = strtab->data.size();
    
    return offset;
  }
  
  const char* Object::getString(u64 offset) const {
    // Need string table
    if (!strtab) {
      return nullptr;
    }
    
    // Check offset
    if (offset >= strtab->data.size()) {
      return nullptr;
    }
    
    // Return pointer to string
    return reinterpret_cast<const char*>(strtab->data.data() + offset);
  }
  
  // -------------------------------- Private Methods -------------------------------- //
  
  Result Object::initStringTable() {
    // Check if we already have a string table
    if (strtab) {
      return Result::Success;
    }
    
    // Create empty string table section
    u16 index = addSection(".strtab", SectionType::StrTab, SectionFlag::None, nullptr, 0);
    if (index == 0) {
      COIL_RETURN_ERROR(Result::OutOfMemory, ErrorLevel::Error, 
                               "Failed to create string table");
    }
    
    // Add initial empty string (index 0 is reserved)
    strtab->data.push_back('\0');
    strtab->header.size = 1;
    
    return Result::Success;
  }
  
  Result Object::initSymbolTable() {
    // Check if we already have a symbol table
    if (symbols) {
      return Result::Success;
    }
    
    // Create empty symbol table section
    u16 index = addSection(".symtab", SectionType::SymTab, SectionFlag::None, nullptr, 0);
    if (index == 0) {
      COIL_RETURN_ERROR(Result::OutOfMemory, ErrorLevel::Error, 
                               "Failed to create symbol table");
    }
    
    // Initialize symbol array
    symbol_capacity = 16;
    symbol_count = 0;
    symbol_data.resize(symbol_capacity * sizeof(Symbol));
    
    // Initialize symbols pointer
    symbols = reinterpret_cast<Symbol*>(symbol_data.data());
    
    // Update section
    Section* symtab = getSection(index);
    symtab->data = symbol_data;
    symtab->header.size = symbol_data.size();
    
    return Result::Success;
  }
  
  void Object::setupSymbolTable() {
    if (symbol_data.empty()) {
      symbols = nullptr;
      symbol_count = 0;
      symbol_capacity = 0;
      return;
    }
    
    // Set up symbols pointer
    symbols = reinterpret_cast<Symbol*>(symbol_data.data());
    
    // Calculate capacity and count
    symbol_capacity = symbol_data.size() / sizeof(Symbol);
    symbol_count = symbol_capacity; // Assume all slots are used
  }

} // namespace coil