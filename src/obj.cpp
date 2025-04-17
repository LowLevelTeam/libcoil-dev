/**
 * @file obj.cpp
 * @brief Implementation of COIL optimized object format
 */

#include "coil/obj.hpp"
#include "coil/err.hpp"
#include <cstdio>
#include <string>
#include <algorithm>
#include <limits>

namespace coil {

  // -------------------------------- Section Base Class -------------------------------- //
  BaseSection::BaseSection(const SectionHeader& header) : header(header) {}

  // -------------------------------- Data Section Implementation -------------------------------- //
  DataSection::DataSection(const SectionHeader& header) : BaseSection(header) {}

  Result DataSection::save(Stream& stream) const {
    // Write section header
    Result result = stream.writeValue(header);
    if (result != Result::Success) {
      return result;
    }
    
    // Write section data
    if (!data.empty()) {
      size_t written = stream.write(data.data(), data.size());
      if (written != data.size()) {
        return Result::IoError;
      }
    }
    
    return Result::Success;
  }

  Result DataSection::load(Stream& stream) {
    // Header should already be loaded by caller
    
    // Check for realistic size before attempting resize
    if (header.size > std::numeric_limits<size_t>::max() / 2) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Section size too large: %llu",
                        (unsigned long long)header.size);
      return Result::OutOfMemory;
    }
    
    // Pre-allocate memory and check if it succeeded
    if (header.size > 0) {
      // For no-exceptions environment, we need to check size
      const size_t old_size = data.size();
      
      // Try to resize
      data.resize(header.size);
      
      // Check if resize actually worked
      if (data.size() != header.size) {
        // Restore previous size if possible
        if (old_size < data.size()) {
          data.resize(old_size);
        }
        COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate %llu bytes for section data", 
                          (unsigned long long)header.size);
        return Result::OutOfMemory;
      }
      
      // Read section data
      size_t read = stream.read(data.data(), data.size());
      if (read != data.size()) {
        return Result::IoError;
      }
    }
    
    return Result::Success;
  }

  std::unique_ptr<BaseSection> DataSection::clone() const {
    std::unique_ptr<BaseSection> copy(new DataSection(header));
    if (!copy) {
      return nullptr;
    }
    
    static_cast<DataSection*>(copy.get())->data = data;
    return copy;
  }

  // -------------------------------- Symbol Section Implementation -------------------------------- //
  SymbolSection::SymbolSection(const SectionHeader& header) : BaseSection(header) {}

  Result SymbolSection::save(Stream& stream) const {
    // Write section header
    Result result = stream.writeValue(header);
    if (result != Result::Success) {
      return result;
    }
    
    // Write symbols
    for (const auto& symbol : symbols) {
      result = stream.writeValue(symbol);
      if (result != Result::Success) {
        return result;
      }
    }
    
    return Result::Success;
  }

  Result SymbolSection::load(Stream& stream) {
    // Header should already be loaded by caller
    
    // Calculate number of symbols
    if (header.size % sizeof(Symbol) != 0) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Invalid symbol section size: %llu is not a multiple of %llu",
                        (unsigned long long)header.size, (unsigned long long)sizeof(Symbol));
      return Result::InvalidFormat;
    }
    
    size_t symbol_count = header.size / sizeof(Symbol);
    
    // Check for realistic count before attempting resize
    if (symbol_count > std::numeric_limits<size_t>::max() / sizeof(Symbol) / 2) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Symbol count too large: %llu",
                        (unsigned long long)symbol_count);
      return Result::OutOfMemory;
    }
    
    // Pre-allocate memory
    const size_t old_size = symbols.size();
    symbols.resize(symbol_count);
    
    // Check if resize worked
    if (symbols.size() != symbol_count) {
      // Restore previous size if possible
      if (old_size < symbols.size()) {
        symbols.resize(old_size);
      }
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate memory for %llu symbols",
                        (unsigned long long)symbol_count);
      return Result::OutOfMemory;
    }
    
    // Read symbols
    for (auto& symbol : symbols) {
      Result result = stream.readValue(symbol);
      if (result != Result::Success) {
        return result;
      }
    }
    
    return Result::Success;
  }

  u16 SymbolSection::addSymbol(const Symbol& symbol) {
    // Check if we have room
    if (symbols.size() + 1 > symbols.capacity()) {
      // Need to grow - calculate safe new capacity
      size_t new_capacity = symbols.capacity() == 0 ? 8 : symbols.capacity() * 2;
      
      // Check for overflow
      if (new_capacity > std::numeric_limits<size_t>::max() / sizeof(Symbol)) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Symbol table would exceed maximum size");
        return 0;
      }
      
      // Try to reserve space
      const size_t old_size = symbols.size();
      symbols.reserve(new_capacity);
      
      // Verify it worked
      if (symbols.capacity() != new_capacity) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to expand symbol table capacity");
        return 0;
      }
    }
    
    // Add symbol
    symbols.push_back(symbol);
    
    // Verify it worked
    if (symbols.size() <= 0) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to add symbol to table");
      return 0;
    }
    
    // Update header size
    header.size = symbols.size() * sizeof(Symbol);
    
    return static_cast<u16>(symbols.size());
  }

  const Symbol* SymbolSection::getSymbol(u16 index) const {
    if (index == 0 || index > symbols.size()) {
      return nullptr;
    }
    return &symbols[index - 1];
  }

  Symbol* SymbolSection::getSymbol(u16 index) {
    if (index == 0 || index > symbols.size()) {
      return nullptr;
    }
    return &symbols[index - 1];
  }

  std::unique_ptr<BaseSection> SymbolSection::clone() const {
    std::unique_ptr<BaseSection> copy(new SymbolSection(header));
    if (!copy) {
      return nullptr;
    }
    
    static_cast<SymbolSection*>(copy.get())->symbols = symbols;
    return copy;
  }

  // -------------------------------- Section Factory -------------------------------- //
  std::unique_ptr<BaseSection> createSection(const SectionHeader& header) {
    std::unique_ptr<BaseSection> section;
    
    if (header.type == static_cast<u8>(SectionType::SymTab)) {
      section.reset(new SymbolSection(header));
    } else {
      section.reset(new DataSection(header));
    }
    
    if (!section) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate memory for section");
    }
    
    return section;
  }

  // -------------------------------- Object Implementation -------------------------------- //
  Object::Object() {
    // Initialize header with defaults
    header.magic[0] = COIL_MAGIC[0];
    header.magic[1] = COIL_MAGIC[1];
    header.magic[2] = COIL_MAGIC[2];
    header.magic[3] = COIL_MAGIC[3];
    header.version = COIL_VERSION;
    header.section_count = 0;
    header.file_size = 0;
  }

  Object Object::create() {
    return Object();
  }

  // -------------------------------- Stream Functionality -------------------------------- //
  Result Object::load(Stream& stream) {
    // Clear any existing data
    sections.clear();
    strtab = nullptr;
    symtab = nullptr;

    // Read header
    Result result = stream.readValue(header);
    if (result != Result::Success) {
      return result;
    }

    // Validate magic number
    if (header.magic[0] != COIL_MAGIC[0] ||
        header.magic[1] != COIL_MAGIC[1] ||
        header.magic[2] != COIL_MAGIC[2] ||
        header.magic[3] != COIL_MAGIC[3]) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Invalid object file magic number %c%c%c%c", 
                       header.magic[0], header.magic[1], header.magic[2], header.magic[3]);
      return Result::InvalidFormat;
    }

    // Check version compatibility
    if (header.version != COIL_VERSION) {
      COIL_REPORT_ERROR(
        ErrorLevel::Warning, 
        "Object format version mismatch: file=%hu, expected=%hu",
        header.version, COIL_VERSION
      );
    }

    // Pre-check if section count is reasonable to avoid excessive memory allocation
    if (header.section_count > 1000) {
      COIL_REPORT_ERROR(ErrorLevel::Warning, "Very large section count: %hu", header.section_count);
    }

    // Reserve space for sections
    const size_t old_capacity = sections.capacity();
    sections.reserve(header.section_count);
    
    // Verify reserve worked
    if (sections.capacity() < header.section_count && header.section_count > 0) {
      // Restore if possible
      if (old_capacity > sections.capacity()) {
        sections.reserve(old_capacity);
      }
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate memory for %hu sections", header.section_count);
      return Result::OutOfMemory;
    }

    // Read all section headers and data
    for (u16 i = 0; i < header.section_count; i++) {
      // Read section header
      SectionHeader secHeader;
      result = stream.readValue(secHeader);
      if (result != Result::Success) {
        return result;
      }

      // Perform basic validation on section size
      if (secHeader.size > 100 * 1024 * 1024) { // 100MB sanity check
        COIL_REPORT_ERROR(ErrorLevel::Warning, "Very large section size: %llu bytes", 
                         (unsigned long long)secHeader.size);
      }

      // Create appropriate section type
      auto section = createSection(secHeader);
      if (!section) {
        return Result::OutOfMemory;
      }
      
      // Load section data
      result = section->load(stream);
      if (result != Result::Success) {
        return result;
      }

      // Store special section pointers
      if (secHeader.type == static_cast<u8>(SectionType::StrTab)) {
        if (strtab != nullptr) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Multiple string tables in object file %d", i);
          return Result::InvalidFormat;
        }
        // Using static_cast instead of dynamic_cast since we're compiling with -fno-rtti
        strtab = static_cast<DataSection*>(section.get());
      } else if (secHeader.type == static_cast<u8>(SectionType::SymTab)) {
        if (symtab != nullptr) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Multiple symbol tables in object file %d", i);
          return Result::InvalidFormat;
        }
        // Using static_cast instead of dynamic_cast since we're compiling with -fno-rtti
        symtab = static_cast<SymbolSection*>(section.get());
      }

      // Add section to the list
      const size_t old_size = sections.size();
      sections.push_back(std::move(section));
      
      // Verify it worked
      if (sections.size() <= old_size) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to add section %d", i);
        return Result::OutOfMemory;
      }
    }

    return Result::Success;
  }

  Result Object::save(Stream& stream) const {
    // Update section count
    // Need to const_cast since we're modifying header in a const method
    const_cast<u16&>(this->header.section_count) = static_cast<u16>(sections.size());
    
    // Calculate file size (approximate)
    u64 total_size = sizeof(ObjectHeader);
    for (const auto& section : sections) {
      total_size += sizeof(SectionHeader) + section->getSize();
    }
    const_cast<u64&>(this->header.file_size) = total_size;

    // Write header
    Result result = stream.writeValue(header);
    if (result != Result::Success) {
      return result;
    }

    // Write all sections
    for (const auto& section : sections) {
      result = section->save(stream);
      if (result != Result::Success) {
        return result;
      }
    }

    return Result::Success;
  }

  // -------------------------------- Section Functionality -------------------------------- //
  u16 Object::getSectionIndex(const char* name, size_t namelen) const {
    if (!name || !strtab) {
      return 0;
    }

    // Search for section with matching name
    for (size_t i = 0; i < sections.size(); i++) {
      const SectionHeader& sec_header = sections[i]->getHeader();
      const char* secname = getString(sec_header.name);
      
      if (secname && strncmp(secname, name, namelen) == 0 && secname[namelen] == '\0') {
        return static_cast<u16>(i + 1);
      }
    }

    return 0;
  }

  BaseSection* Object::getSection(const char* name) {
    u16 index = getSectionIndex(name, strlen(name));
    return getSection(index);
  }

  BaseSection* Object::getSection(u16 index) {
    if (index == 0 || index > sections.size()) {
      return nullptr;
    }
    return sections[index - 1].get();
  }

  const BaseSection* Object::getSection(u16 index) const {
    if (index == 0 || index > sections.size()) {
      return nullptr;
    }
    return sections[index - 1].get();
  }

  u16 Object::addSection(u64 name_offset, u16 flags, u8 type, u64 size, const u8* data, u64 datasize) {
    SectionHeader sec_header;
    sec_header.name = name_offset;
    sec_header.size = size;
    sec_header.flags = flags;
    sec_header.type = type;
    return addSection(sec_header, data, datasize);
  }

  u16 Object::addSection(const SectionHeader& sec_header, const u8* data, u64 datasize) {
    // Create appropriate section type
    auto section = createSection(sec_header);
    if (!section) {
      return 0;
    }
    
    // Add initial data if provided
    if (data && datasize > 0) {
      if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
        if (datasize % sizeof(Symbol) != 0) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Invalid symbol data size %llu", (unsigned long long)datasize);
          return 0;
        }
        
        // Add symbols
        auto* symSection = static_cast<SymbolSection*>(section.get());
        const Symbol* symbols = reinterpret_cast<const Symbol*>(data);
        size_t symbolCount = datasize / sizeof(Symbol);
        
        for (size_t i = 0; i < symbolCount; i++) {
          if (symSection->addSymbol(symbols[i]) == 0) {
            return 0; // Failed to add symbol
          }
        }
      } else {
        // Add raw data
        auto* dataSection = static_cast<DataSection*>(section.get());
        
        // First check if we can allocate the memory
        const size_t old_size = dataSection->getData().size();
        dataSection->getData().resize(datasize);
        
        // Verify resize worked
        if (dataSection->getData().size() != datasize) {
          // Try to restore
          if (old_size < dataSection->getData().size()) {
            dataSection->getData().resize(old_size);
          }
          COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to allocate %llu bytes for section data", 
                         (unsigned long long)datasize);
          return 0;
        }
        
        // Copy the data
        memcpy(dataSection->getData().data(), data, datasize);
      }
    }

    // Update special section pointers
    if (sec_header.type == static_cast<u8>(SectionType::StrTab)) {
      if (strtab != nullptr) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Multiple string tables not supported %p", (void*)strtab);
        return 0;
      }
      strtab = static_cast<DataSection*>(section.get());
      
      // Ensure string table starts with null byte
      if (strtab->getData().empty()) {
        const size_t old_size = strtab->getData().size();
        strtab->getData().resize(old_size + 1);
        
        // Verify it worked
        if (strtab->getData().size() != old_size + 1) {
          COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to initialize string table with null byte");
          strtab = nullptr;
          return 0;
        }
        
        // Set null byte
        strtab->getData()[old_size] = 0;
      }
    } else if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
      if (symtab != nullptr) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Multiple symbol tables not supported %p", (void*)symtab);
        return 0;
      }
      symtab = static_cast<SymbolSection*>(section.get());
    }

    // Check if we need to grow the vector
    if (sections.size() + 1 > sections.capacity()) {
      size_t new_capacity = sections.capacity() == 0 ? 8 : sections.capacity() * 2;
      
      // Check for overflow
      if (new_capacity > std::numeric_limits<size_t>::max() / sizeof(std::unique_ptr<BaseSection>)) {
        COIL_REPORT_ERROR(ErrorLevel::Error, "Section table would exceed maximum size");
        
        // Reset pointers
        if (sec_header.type == static_cast<u8>(SectionType::StrTab)) {
          strtab = nullptr;
        } else if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
          symtab = nullptr;
        }
        
        return 0;
      }
      
      // Try to reserve space
      const size_t old_capacity = sections.capacity();
      sections.reserve(new_capacity);
      
      // Verify it worked
      if (sections.capacity() != new_capacity && new_capacity > 0) {
        // Try to restore
        if (old_capacity > sections.capacity()) {
          sections.reserve(old_capacity);
        }
        
        COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to expand section table capacity");
        
        // Reset pointers
        if (sec_header.type == static_cast<u8>(SectionType::StrTab)) {
          strtab = nullptr;
        } else if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
          symtab = nullptr;
        }
        
        return 0;
      }
    }

    // Add section to the list
    const size_t old_size = sections.size();
    sections.push_back(std::move(section));
    
    // Verify it worked
    if (sections.size() <= old_size) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to add section");
      
      // Reset pointers
      if (sec_header.type == static_cast<u8>(SectionType::StrTab)) {
        strtab = nullptr;
      } else if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
        symtab = nullptr;
      }
      
      return 0;
    }
    
    // Update section count in header
    this->header.section_count = static_cast<u16>(sections.size());
    
    return static_cast<u16>(sections.size());
  }

  // -------------------------------- Symbol Table Functionality -------------------------------- //
  Result Object::initSymbolTable() {
    if (symtab != nullptr) {
      return Result::Success;
    }

    // Create a string table first if needed
    Result result = initStringTable();
    if (result != Result::Success) {
      return result;
    }

    // Add name to string table
    u64 name_offset = addString(".symtab");
    if (name_offset == 0) {
      return Result::IoError;
    }

    // Create symbol table section
    SectionHeader sec_header;
    sec_header.name = name_offset;
    sec_header.size = 0;
    sec_header.flags = 0;
    sec_header.type = static_cast<u8>(SectionType::SymTab);

    u16 index = addSection(sec_header, nullptr, 0);
    if (index == 0) {
      return Result::IoError;
    }
    
    // Update section count in header
    this->header.section_count = static_cast<u16>(sections.size());

    return Result::Success;
  }

  u16 Object::getSymbolIndex(const char* name, size_t namelen) const {
    if (!name || !symtab || !strtab) {
      return 0;
    }

    // Search for symbol with matching name
    const auto& symbols = symtab->getSymbols();
    for (size_t i = 0; i < symbols.size(); i++) {
      const Symbol& sym = symbols[i];
      const char* symname = getString(sym.name);
      
      if (symname && strncmp(symname, name, namelen) == 0 && symname[namelen] == '\0') {
        return static_cast<u16>(i + 1);
      }
    }

    return 0;
  }

  Symbol* Object::getSymbol(u16 index) {
    if (!symtab) {
      return nullptr;
    }
    return symtab->getSymbol(index);
  }

  const Symbol* Object::getSymbol(u16 index) const {
    if (!symtab) {
      return nullptr;
    }
    return symtab->getSymbol(index);
  }

  u16 Object::addSymbol(u64 name, u32 value, u16 section_index, u8 type, u8 binding) {
    Symbol symbol;
    symbol.name = name;
    symbol.value = value;
    symbol.section_index = section_index;
    symbol.type = type;
    symbol.binding = binding;
    return addSymbol(symbol);
  }

  u16 Object::addSymbol(const Symbol& symbol) {
    if (!symtab) {
      Result result = initSymbolTable();
      if (result != Result::Success) {
        return 0;
      }
    }

    return symtab->addSymbol(symbol);
  }

  // -------------------------------- String Table Functionality -------------------------------- //
  Result Object::initStringTable() {
    if (strtab != nullptr) {
      return Result::Success;
    }

    // Create string table section
    SectionHeader sec_header;
    sec_header.name = 0; // No name initially
    sec_header.size = 1; // Start with null byte
    sec_header.flags = 0;
    sec_header.type = static_cast<u8>(SectionType::StrTab);

    // Add an empty string table with just a null byte
    u8 nullByte = 0;
    u16 index = addSection(sec_header, &nullByte, 1);
    if (index == 0) {
      return Result::IoError;
    }

    // Now add the name to the string table
    u64 name_offset = addString(".strtab");
    if (name_offset == 0) {
      return Result::IoError;
    }

    // Update the name in the header
    strtab->setHeader({name_offset, sec_header.size, sec_header.flags, sec_header.type});
    
    // Update section count in header
    this->header.section_count = static_cast<u16>(sections.size());

    return Result::Success;
  }

  const char* Object::getString(u64 offset) const {
    if (!strtab) {
      return nullptr;
    }
    
    const auto& data = strtab->getData();
    if (offset >= data.size()) {
      return nullptr;
    }
    
    return reinterpret_cast<const char*>(data.data() + offset);
  }

  u64 Object::addString(const char* str) {
    if (!str) {
      return 0;
    }
    
    if (!strtab) {
      Result result = initStringTable();
      if (result != Result::Success) {
        return 0;
      }
    }
    
    // Get current data
    auto& data = strtab->getData();
    
    // Check if string already exists in table
    size_t str_len = strlen(str);
    for (size_t i = 0; i < data.size(); ) {
      // Make sure we don't read past the end of data
      if (i >= data.size()) break;
      
      const char* existing = reinterpret_cast<const char*>(data.data() + i);
      
      // Find null terminator
      size_t j = 0;
      while (i + j < data.size() && data[i + j] != 0) {
        j++;
      }
      
      // Found null terminator?
      if (i + j >= data.size()) break;
      
      // Check if strings match
      if (j == str_len && memcmp(existing, str, str_len) == 0) {
        return static_cast<u64>(i);
      }
      
      // Move to next string
      i += j + 1;
    }
    
    // String not found, add it
    u64 offset = data.size();
    
    // Ensure we have capacity
    size_t new_size = data.size() + str_len + 1;
    
    // Check for overflow
    if (new_size > std::numeric_limits<size_t>::max() / sizeof(u8)) {
      COIL_REPORT_ERROR(ErrorLevel::Error, "String table would exceed maximum size");
      return 0;
    }
    
    // Resize the vector
    const size_t old_size = data.size();
    data.resize(new_size);
    
    // Check if resize succeeded
    if (data.size() != new_size) {
      // Attempt to restore
      if (old_size < data.size()) {
        data.resize(old_size);
      }
      COIL_REPORT_ERROR(ErrorLevel::Error, "Failed to add string '%s' to string table", str);
      return 0;
    }
    
    // Copy the string and null terminator
    memcpy(data.data() + offset, str, str_len);
    data[offset + str_len] = 0;
    
    // Update section size in header
    SectionHeader header = strtab->getHeader();
    header.size = data.size();
    strtab->setHeader(header);
    
    return offset;
  }

} // namespace coil