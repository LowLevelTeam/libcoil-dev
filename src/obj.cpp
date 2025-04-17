/**
 * @file obj.cpp
 * @brief Implementation of COIL optimized object format
 */

#include "coil/obj.hpp"
#include "coil/err.hpp"
#include <algorithm>
#include <sstream>
#include <limits>

namespace coil {

// -------------------------------- Section Base Class -------------------------------- //
BaseSection::BaseSection(const SectionHeader& header) : m_header(header) {}

// -------------------------------- Data Section Implementation -------------------------------- //
DataSection::DataSection(const SectionHeader& header) : BaseSection(header) {}

void DataSection::save(Stream& stream) const {
  // Write section header
  stream.writev(m_header);
  
  // Write section data
  if (!m_data.empty()) {
    size_t written = stream.write(m_data.data(), m_data.size());
    if (written != m_data.size()) {
      throw IOException("Failed to write data section");
    }
  }
}

void DataSection::load(Stream& stream) {
  // Header should already be loaded by caller
  
  // Check for realistic size before attempting resize
  if (m_header.size > std::numeric_limits<size_t>::max() / 2) {
    std::ostringstream ss;
    ss << "Section size too large: " << m_header.size;
    throw OutOfMemoryException(ss.str());
  }
  
  // Pre-allocate memory for section data
  if (m_header.size > 0) {
    m_data.resize(m_header.size);
    
    // Read section data
    size_t read = stream.read(m_data.data(), m_data.size());
    if (read != m_data.size()) {
      throw IOException("Failed to read data section");
    }
  }
}

std::unique_ptr<BaseSection> DataSection::clone() const {
  auto copy = std::make_unique<DataSection>(m_header);
  copy->m_data = m_data;
  return copy;
}

// -------------------------------- Symbol Section Implementation -------------------------------- //
SymbolSection::SymbolSection(const SectionHeader& header) : BaseSection(header) {}

void SymbolSection::save(Stream& stream) const {
  // Write section header
  stream.writev(m_header);
  
  // Write symbols
  for (const auto& symbol : m_symbols) {
    stream.writev(symbol);
  }
}

void SymbolSection::load(Stream& stream) {
  // Header should already be loaded by caller
  
  // Calculate number of symbols
  if (m_header.size % sizeof(Symbol) != 0) {
    std::ostringstream ss;
    ss << "Invalid symbol section size: " << m_header.size 
       << " is not a multiple of " << sizeof(Symbol);
    throw FormatException(ss.str());
  }
  
  size_t symbol_count = m_header.size / sizeof(Symbol);
  
  // Check for realistic count before attempting resize
  if (symbol_count > std::numeric_limits<size_t>::max() / sizeof(Symbol) / 2) {
    std::ostringstream ss;
    ss << "Symbol count too large: " << symbol_count;
    throw OutOfMemoryException(ss.str());
  }
  
  // Pre-allocate memory
  m_symbols.resize(symbol_count);
  
  // Read symbols
  for (auto& symbol : m_symbols) {
    stream.read(&symbol, sizeof(Symbol));
  }
}

u16 SymbolSection::addSymbol(const Symbol& symbol) {
  // Add symbol
  m_symbols.push_back(symbol);
  
  // Update header size
  m_header.size = m_symbols.size() * sizeof(Symbol);
  
  return static_cast<u16>(m_symbols.size());
}

const Symbol* SymbolSection::getSymbol(u16 index) const {
  if (index == 0 || index > m_symbols.size()) {
    return nullptr;
  }
  return &m_symbols[index - 1];
}

Symbol* SymbolSection::getSymbol(u16 index) {
  if (index == 0 || index > m_symbols.size()) {
    return nullptr;
  }
  return &m_symbols[index - 1];
}

std::unique_ptr<BaseSection> SymbolSection::clone() const {
  auto copy = std::make_unique<SymbolSection>(m_header);
  copy->m_symbols = m_symbols;
  return copy;
}

// -------------------------------- Section Factory -------------------------------- //
std::unique_ptr<BaseSection> createSection(const SectionHeader& header) {
  std::unique_ptr<BaseSection> section;
  
  if (header.type == static_cast<u8>(SectionType::SymTab)) {
    section = std::make_unique<SymbolSection>(header);
  } else {
    section = std::make_unique<DataSection>(header);
  }
  
  return section;
}

// -------------------------------- Object Implementation -------------------------------- //
Object::Object() {
  // Initialize header with defaults
  m_header.magic[0] = COIL_MAGIC[0];
  m_header.magic[1] = COIL_MAGIC[1];
  m_header.magic[2] = COIL_MAGIC[2];
  m_header.magic[3] = COIL_MAGIC[3];
  m_header.version = COIL_VERSION;
  m_header.section_count = 0;
  m_header.file_size = 0;
}

Object Object::create() {
  return Object();
}

// -------------------------------- Stream Functionality -------------------------------- //
void Object::load(Stream& stream) {
  // Clear any existing data
  m_sections.clear();
  m_strtab = nullptr;
  m_symtab = nullptr;
  
  // Read header
  stream.read(&m_header, sizeof(ObjectHeader));
  
  // Validate magic number
  if (m_header.magic[0] != COIL_MAGIC[0] ||
      m_header.magic[1] != COIL_MAGIC[1] ||
      m_header.magic[2] != COIL_MAGIC[2] ||
      m_header.magic[3] != COIL_MAGIC[3]) {
    std::ostringstream ss;
    ss << "Invalid object file magic number "
       << m_header.magic[0] << m_header.magic[1] 
       << m_header.magic[2] << m_header.magic[3];
    throw FormatException(ss.str());
  }
  
  // Check version compatibility
  if (m_header.version != COIL_VERSION) {
    std::ostringstream ss;
    ss << "Object format version mismatch: file=" << m_header.version 
       << ", expected=" << COIL_VERSION;
    Logger::warning(ss.str());
  }
  
  // Pre-check if section count is reasonable
  if (m_header.section_count > 1000) {
    std::ostringstream ss;
    ss << "Very large section count: " << m_header.section_count;
    Logger::warning(ss.str());
  }
  
  // Reserve space for sections
  m_sections.reserve(m_header.section_count);
  
  // Read all section headers and data
  for (u16 i = 0; i < m_header.section_count; i++) {
    // Read section header
    SectionHeader secHeader;
    stream.read(&secHeader, sizeof(SectionHeader));
    
    // Perform basic validation on section size
    if (secHeader.size > 100 * 1024 * 1024) { // 100MB sanity check
      std::ostringstream ss;
      ss << "Very large section size: " << secHeader.size << " bytes";
      Logger::warning(ss.str());
    }
    
    // Create appropriate section type
    auto section = createSection(secHeader);
    
    // Load section data
    section->load(stream);
    
    // Store special section pointers
    if (secHeader.type == static_cast<u8>(SectionType::StrTab)) {
      if (m_strtab != nullptr) {
        throw FormatException("Multiple string tables in object file");
      }
      m_strtab = static_cast<DataSection*>(section.get());
    } else if (secHeader.type == static_cast<u8>(SectionType::SymTab)) {
      if (m_symtab != nullptr) {
        throw FormatException("Multiple symbol tables in object file");
      }
      m_symtab = static_cast<SymbolSection*>(section.get());
    }
    
    // Add section to the list
    m_sections.push_back(std::move(section));
  }
}

void Object::save(Stream& stream) const {
  // Update section count
  const_cast<u16&>(m_header.section_count) = static_cast<u16>(m_sections.size());
  
  // Calculate file size (approximate)
  u64 total_size = sizeof(ObjectHeader);
  for (const auto& section : m_sections) {
    total_size += sizeof(SectionHeader) + section->getSize();
  }
  const_cast<u64&>(m_header.file_size) = total_size;
  
  // Write header
  stream.write(&m_header, sizeof(ObjectHeader));
  
  // Write all sections
  for (const auto& section : m_sections) {
    section->save(stream);
  }
}

// -------------------------------- Section Functionality -------------------------------- //
u16 Object::getSectionIndex(std::string_view name) const {
  if (name.empty() || !m_strtab) {
    return 0;
  }
  
  // Search for section with matching name
  for (size_t i = 0; i < m_sections.size(); i++) {
    const SectionHeader& sec_header = m_sections[i]->getHeader();
    std::string_view secname = getString(sec_header.name);
    
    if (!secname.empty() && secname == name) {
      return static_cast<u16>(i + 1);
    }
  }
  
  return 0;
}

BaseSection* Object::getSection(std::string_view name) {
  u16 index = getSectionIndex(name);
  return getSection(index);
}

BaseSection* Object::getSection(u16 index) {
  if (index == 0 || index > m_sections.size()) {
    return nullptr;
  }
  return m_sections[index - 1].get();
}

const BaseSection* Object::getSection(u16 index) const {
  if (index == 0 || index > m_sections.size()) {
    return nullptr;
  }
  return m_sections[index - 1].get();
}

u16 Object::addSection(u64 name_offset, u16 flags, u8 type, u64 size, std::span<const u8> data) {
  SectionHeader sec_header;
  sec_header.name = name_offset;
  sec_header.size = size;
  sec_header.flags = flags;
  sec_header.type = type;
  return addSection(sec_header, data);
}

u16 Object::addSection(const SectionHeader& sec_header, std::span<const u8> data) {
  // Create appropriate section type
  auto section = createSection(sec_header);
  
  // Add initial data if provided
  if (!data.empty()) {
    if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
      if (data.size() % sizeof(Symbol) != 0) {
        std::ostringstream ss;
        ss << "Invalid symbol data size " << data.size();
        throw InvalidArgException(ss.str());
      }
      
      // Add symbols
      auto* symSection = static_cast<SymbolSection*>(section.get());
      const Symbol* symbols = reinterpret_cast<const Symbol*>(data.data());
      size_t symbolCount = data.size() / sizeof(Symbol);
      
      for (size_t i = 0; i < symbolCount; i++) {
        if (symSection->addSymbol(symbols[i]) == 0) {
          throw IOException("Failed to add symbol");
        }
      }
    } else {
      // Add raw data
      auto* dataSection = static_cast<DataSection*>(section.get());
      dataSection->getData().assign(data.begin(), data.end());
    }
  }
  
  // Update special section pointers
  if (sec_header.type == static_cast<u8>(SectionType::StrTab)) {
    if (m_strtab != nullptr) {
      throw AlreadyExistsException("Multiple string tables not supported");
    }
    m_strtab = static_cast<DataSection*>(section.get());
    
    // Ensure string table starts with null byte
    if (m_strtab->getData().empty()) {
      m_strtab->getData().push_back(0);
    }
  } else if (sec_header.type == static_cast<u8>(SectionType::SymTab)) {
    if (m_symtab != nullptr) {
      throw AlreadyExistsException("Multiple symbol tables not supported");
    }
    m_symtab = static_cast<SymbolSection*>(section.get());
  }
  
  // Add section to the list
  m_sections.push_back(std::move(section));
  
  // Update section count in header
  m_header.section_count = static_cast<u16>(m_sections.size());
  
  return static_cast<u16>(m_sections.size());
}

// -------------------------------- Symbol Table Functionality -------------------------------- //
void Object::initSymbolTable() {
  if (m_symtab != nullptr) {
    return;
  }
  
  // Create a string table first if needed
  initStringTable();
  
  // Add name to string table
  u64 name_offset = addString(".symtab");
  if (name_offset == 0) {
    throw IOException("Failed to add string to string table");
  }
  
  // Create symbol table section
  SectionHeader sec_header;
  sec_header.name = name_offset;
  sec_header.size = 0;
  sec_header.flags = 0;
  sec_header.type = static_cast<u8>(SectionType::SymTab);
  
  u16 index = addSection(sec_header);
  if (index == 0) {
    throw IOException("Failed to add symbol table section");
  }
}

u16 Object::getSymbolIndex(std::string_view name) const {
  if (name.empty() || !m_symtab || !m_strtab) {
    return 0;
  }
  
  // Search for symbol with matching name
  const auto& symbols = m_symtab->getSymbols();
  for (size_t i = 0; i < symbols.size(); i++) {
    const Symbol& sym = symbols[i];
    std::string_view symname = getString(sym.name);
    
    if (!symname.empty() && symname == name) {
      return static_cast<u16>(i + 1);
    }
  }
  
  return 0;
}

Symbol* Object::getSymbol(u16 index) {
  if (!m_symtab) {
    return nullptr;
  }
  return m_symtab->getSymbol(index);
}

const Symbol* Object::getSymbol(u16 index) const {
  if (!m_symtab) {
    return nullptr;
  }
  return m_symtab->getSymbol(index);
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
  if (!m_symtab) {
    initSymbolTable();
  }
  
  return m_symtab->addSymbol(symbol);
}

// -------------------------------- String Table Functionality -------------------------------- //
void Object::initStringTable() {
  if (m_strtab != nullptr) {
    return;
  }
  
  // Create string table section
  SectionHeader sec_header;
  sec_header.name = 0; // No name initially
  sec_header.size = 1; // Start with null byte
  sec_header.flags = 0;
  sec_header.type = static_cast<u8>(SectionType::StrTab);
  
  // Add an empty string table with just a null byte
  std::vector<u8> initialData = {0};
  u16 index = addSection(sec_header, initialData);
  if (index == 0) {
    throw IOException("Failed to create string table");
  }
  
  // Now add the name to the string table
  u64 name_offset = addString(".strtab");
  if (name_offset == 0) {
    throw IOException("Failed to add string table name");
  }
  
  // Update the name in the header
  m_strtab->setHeader({name_offset, sec_header.size, sec_header.flags, sec_header.type});
}

std::string_view Object::getString(u64 offset) const {
  if (!m_strtab) {
    return {};
  }
  
  const auto& data = m_strtab->getData();
  if (offset >= data.size()) {
    return {};
  }
  
  // Find the length of the string (up to null terminator)
  size_t len = 0;
  while (offset + len < data.size() && data[offset + len] != 0) {
    len++;
  }
  
  return std::string_view(reinterpret_cast<const char*>(data.data() + offset), len);
}

u64 Object::addString(std::string_view str) {
  if (str.empty()) {
    return 0;
  }
  
  if (!m_strtab) {
    initStringTable();
  }
  
  // Get current data
  auto& data = m_strtab->getData();
  
  // Check if string already exists in table
  for (size_t i = 0; i < data.size(); ) {
    // Make sure we don't read past the end of data
    if (i >= data.size()) break;
    
    std::string_view existing(reinterpret_cast<const char*>(data.data() + i));
    
    // Find string length
    size_t j = 0;
    while (i + j < data.size() && data[i + j] != 0) {
      j++;
    }
    
    // Found null terminator?
    if (i + j >= data.size()) break;
    
    // Check if strings match
    if (j == str.size() && existing.substr(0, j) == str) {
      return static_cast<u64>(i);
    }
    
    // Move to next string
    i += j + 1;
  }
  
  // String not found, add it
  u64 offset = data.size();
  
  // Add string and null terminator
  data.insert(data.end(), str.begin(), str.end());
  data.push_back(0);
  
  // Update section size in header
  SectionHeader header = m_strtab->getHeader();
  header.size = data.size();
  m_strtab->setHeader(header);
  
  return offset;
}

} // namespace coil