#include "coil/binary_format.h"
#include "coil/instruction_set.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace coil {

// CoilHeader implementation
bool CoilHeader::isValid() const {
  // Check magic number
  if (magic[0] != 'C' || magic[1] != 'O' || magic[2] != 'I' || magic[3] != 'L') {
      return false;
  }
  
  // Check for required offsets
  if (symbol_offset == 0 || section_offset == 0) {
      return false;
  }
  
  // Check file size
  if (file_size == 0) {
      return false;
  }
  
  return true;
}

std::vector<uint8_t> CoilHeader::encode() const {
  std::vector<uint8_t> result(sizeof(CoilHeader));
  uint8_t* ptr = result.data();
  
  // Copy header data
  std::memcpy(ptr, magic, 4); ptr += 4;
  *ptr++ = major;
  *ptr++ = minor;
  *ptr++ = patch;
  *ptr++ = flags;
  
  // Store offsets in little-endian (default)
  bool isBigEndian = (flags & FormatFlags::FORMAT_BIG_ENDIAN) != 0;
  
  if (!isBigEndian) {
      *reinterpret_cast<uint32_t*>(ptr) = symbol_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = section_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = reloc_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = debug_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = file_size; ptr += 4;
  } else {
      // Implement big-endian encoding if needed
      // For now we'll just use little-endian
      *reinterpret_cast<uint32_t*>(ptr) = symbol_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = section_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = reloc_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = debug_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = file_size; ptr += 4;
  }
  
  return result;
}

CoilHeader CoilHeader::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + sizeof(CoilHeader)) {
      throw std::runtime_error("Insufficient data for CoilHeader");
  }
  
  CoilHeader header;
  const uint8_t* ptr = data.data() + offset;
  
  // Copy header data
  std::memcpy(header.magic, ptr, 4); ptr += 4;
  header.major = *ptr++;
  header.minor = *ptr++;
  header.patch = *ptr++;
  header.flags = *ptr++;
  
  // Read offsets based on endianness
  bool isBigEndian = (header.flags & FormatFlags::FORMAT_BIG_ENDIAN) != 0;
  
  if (!isBigEndian) {
      header.symbol_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.section_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.reloc_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.debug_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.file_size = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
  } else {
      // Implement big-endian decoding if needed
      // For now we'll just use little-endian
      header.symbol_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.section_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.reloc_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.debug_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.file_size = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
  }
  
  offset += sizeof(CoilHeader);
  return header;
}

CoilHeader CoilHeader::createDefault() {
  CoilHeader header;
  header.magic[0] = 'C';
  header.magic[1] = 'O';
  header.magic[2] = 'I';
  header.magic[3] = 'L';
  header.major = 1;
  header.minor = 0;
  header.patch = 0;
  header.flags = FormatFlags::OBJECT_FILE;
  header.symbol_offset = sizeof(CoilHeader);  // Will be updated later
  header.section_offset = sizeof(CoilHeader); // Will be updated later
  header.reloc_offset = 0;                    // No relocations by default
  header.debug_offset = 0;                    // No debug info by default
  header.file_size = sizeof(CoilHeader);      // Will be updated later
  return header;
}

// CoilOHeader implementation
bool CoilOHeader::isValid() const {
  // Check magic number
  if (magic[0] != 'C' || magic[1] != 'I' || magic[2] != 'L' || magic[3] != 'O') {
      return false;
  }
  
  // Check for required offsets
  if (symbol_offset == 0 || section_offset == 0) {
      return false;
  }
  
  // Check file size
  if (file_size == 0) {
      return false;
  }
  
  return true;
}

std::vector<uint8_t> CoilOHeader::encode() const {
  std::vector<uint8_t> result(sizeof(CoilOHeader));
  uint8_t* ptr = result.data();
  
  // Copy header data
  std::memcpy(ptr, magic, 4); ptr += 4;
  *ptr++ = major;
  *ptr++ = minor;
  *ptr++ = patch;
  *ptr++ = flags;
  
  // Store offsets in little-endian (default)
  bool isBigEndian = (flags & FormatFlags::FORMAT_BIG_ENDIAN) != 0;
  
  if (!isBigEndian) {
      *reinterpret_cast<uint32_t*>(ptr) = symbol_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = section_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = meta_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = file_size; ptr += 4;
  } else {
      // Implement big-endian encoding if needed
      // For now we'll just use little-endian
      *reinterpret_cast<uint32_t*>(ptr) = symbol_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = section_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = meta_offset; ptr += 4;
      *reinterpret_cast<uint32_t*>(ptr) = file_size; ptr += 4;
  }
  
  return result;
}

CoilOHeader CoilOHeader::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + sizeof(CoilOHeader)) {
      throw std::runtime_error("Insufficient data for CoilOHeader");
  }
  
  CoilOHeader header;
  const uint8_t* ptr = data.data() + offset;
  
  // Copy header data
  std::memcpy(header.magic, ptr, 4); ptr += 4;
  header.major = *ptr++;
  header.minor = *ptr++;
  header.patch = *ptr++;
  header.flags = *ptr++;
  
  // Read offsets based on endianness
  bool isBigEndian = (header.flags & FormatFlags::FORMAT_BIG_ENDIAN) != 0;
  
  if (!isBigEndian) {
      header.symbol_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.section_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.meta_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.file_size = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
  } else {
      // Implement big-endian decoding if needed
      // For now we'll just use little-endian
      header.symbol_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.section_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.meta_offset = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
      header.file_size = *reinterpret_cast<const uint32_t*>(ptr); ptr += 4;
  }
  
  offset += sizeof(CoilOHeader);
  return header;
}

CoilOHeader CoilOHeader::createDefault() {
  CoilOHeader header;
  header.magic[0] = 'C';
  header.magic[1] = 'I';
  header.magic[2] = 'L';
  header.magic[3] = 'O';
  header.major = 1;
  header.minor = 0;
  header.patch = 0;
  header.flags = FormatFlags::OUTPUT_OBJECT;
  header.symbol_offset = sizeof(CoilOHeader);  // Will be updated later
  header.section_offset = sizeof(CoilOHeader); // Will be updated later
  header.meta_offset = 0;                    // No metadata by default
  header.file_size = sizeof(CoilOHeader);    // Will be updated later
  return header;
}

// Symbol implementation
std::vector<uint8_t> Symbol::encode() const {
  std::vector<uint8_t> result;
  result.reserve(2 + name.size() + 4 + 4 + 2 + 1);
  
  // Add the name length
  result.push_back(name_length & 0xFF);
  result.push_back((name_length >> 8) & 0xFF);
  
  // Add the name
  result.insert(result.end(), name.begin(), name.end());
  
  // Add attributes, value, section_index, and processor_type
  for (int i = 0; i < 4; i++) {
      result.push_back((attributes >> (i * 8)) & 0xFF);
  }
  
  for (int i = 0; i < 4; i++) {
      result.push_back((value >> (i * 8)) & 0xFF);
  }
  
  result.push_back(section_index & 0xFF);
  result.push_back((section_index >> 8) & 0xFF);
  
  result.push_back(processor_type);
  
  return result;
}

Symbol Symbol::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 2) {
      throw std::runtime_error("Insufficient data for Symbol");
  }
  
  Symbol symbol;
  const uint8_t* ptr = data.data() + offset;
  
  // Read name length
  symbol.name_length = ptr[0] | (ptr[1] << 8);
  ptr += 2;
  offset += 2;
  
  // Read name
  if (data.size() < offset + symbol.name_length) {
      throw std::runtime_error("Insufficient data for Symbol name");
  }
  symbol.name.assign(reinterpret_cast<const char*>(ptr), symbol.name_length);
  ptr += symbol.name_length;
  offset += symbol.name_length;
  
  // Read attributes, value, section_index, and processor_type
  if (data.size() < offset + 4 + 4 + 2 + 1) {
      throw std::runtime_error("Insufficient data for Symbol attributes");
  }
  
  symbol.attributes = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  symbol.value = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  symbol.section_index = ptr[0] | (ptr[1] << 8);
  ptr += 2;
  offset += 2;
  
  symbol.processor_type = *ptr++;
  offset += 1;
  
  return symbol;
}

// Section implementation
std::vector<uint8_t> Section::encode() const {
  std::vector<uint8_t> result;
  result.reserve(2 + 4 + 4 + 4 + 4 + 4 + 1 + data.size());
  
  // Add name_index, attributes, offset, size, address, alignment, processor_type
  result.push_back(name_index & 0xFF);
  result.push_back((name_index >> 8) & 0xFF);
  
  for (int i = 0; i < 4; i++) {
      result.push_back((attributes >> (i * 8)) & 0xFF);
  }
  
  for (int i = 0; i < 4; i++) {
      result.push_back((offset >> (i * 8)) & 0xFF);
  }
  
  for (int i = 0; i < 4; i++) {
      result.push_back((size >> (i * 8)) & 0xFF);
  }
  
  for (int i = 0; i < 4; i++) {
      result.push_back((address >> (i * 8)) & 0xFF);
  }
  
  for (int i = 0; i < 4; i++) {
      result.push_back((alignment >> (i * 8)) & 0xFF);
  }
  
  result.push_back(processor_type);
  
  // Add section data
  result.insert(result.end(), data.begin(), data.end());
  
  return result;
}

Section Section::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 2 + 4 + 4 + 4 + 4 + 4 + 1) {
      throw std::runtime_error("Insufficient data for Section");
  }
  
  Section section;
  const uint8_t* ptr = data.data() + offset;
  
  // Read name_index, attributes, offset, size, address, alignment, processor_type
  section.name_index = ptr[0] | (ptr[1] << 8);
  ptr += 2;
  offset += 2;
  
  section.attributes = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  section.offset = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  section.size = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  section.address = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  section.alignment = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  section.processor_type = *ptr++;
  offset += 1;
  
  // Read section data
  if (data.size() < offset + section.size) {
      throw std::runtime_error("Insufficient data for Section data");
  }
  
  section.data.assign(ptr, ptr + section.size);
  offset += section.size;
  
  return section;
}

// Relocation implementation
std::vector<uint8_t> Relocation::encode() const {
  std::vector<uint8_t> result;
  result.reserve(4 + 2 + 2 + 1 + 1);
  
  // Add offset, symbol_index, section_index, type, size
  for (int i = 0; i < 4; i++) {
      result.push_back((offset >> (i * 8)) & 0xFF);
  }
  
  result.push_back(symbol_index & 0xFF);
  result.push_back((symbol_index >> 8) & 0xFF);
  
  result.push_back(section_index & 0xFF);
  result.push_back((section_index >> 8) & 0xFF);
  
  result.push_back(type);
  result.push_back(size);
  
  return result;
}

Relocation Relocation::decode(const std::vector<uint8_t>& data, size_t& offset) {
  if (data.size() < offset + 4 + 2 + 2 + 1 + 1) {
      throw std::runtime_error("Insufficient data for Relocation");
  }
  
  Relocation relocation;
  const uint8_t* ptr = data.data() + offset;
  
  // Read offset, symbol_index, section_index, type, size
  relocation.offset = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
  ptr += 4;
  offset += 4;
  
  relocation.symbol_index = ptr[0] | (ptr[1] << 8);
  ptr += 2;
  offset += 2;
  
  relocation.section_index = ptr[0] | (ptr[1] << 8);
  ptr += 2;
  offset += 2;
  
  relocation.type = *ptr++;
  offset += 1;
  
  relocation.size = *ptr++;
  offset += 1;
  
  return relocation;
}

// CoilObject implementation
CoilObject::CoilObject() : header_(CoilHeader::createDefault()) {
}

uint16_t CoilObject::addSymbol(const Symbol& symbol) {
  symbols_.push_back(symbol);
  return static_cast<uint16_t>(symbols_.size() - 1);
}

uint16_t CoilObject::addSection(const Section& section) {
  sections_.push_back(section);
  return static_cast<uint16_t>(sections_.size() - 1);
}

void CoilObject::addRelocation(const Relocation& relocation) {
  relocations_.push_back(relocation);
}

const Symbol& CoilObject::getSymbol(uint16_t index) const {
  if (index >= symbols_.size()) {
      throw std::out_of_range("Symbol index out of range");
  }
  return symbols_[index];
}

const Section& CoilObject::getSection(uint16_t index) const {
  if (index >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  return sections_[index];
}

const Relocation& CoilObject::getRelocation(uint16_t index) const {
  if (index >= relocations_.size()) {
      throw std::out_of_range("Relocation index out of range");
  }
  return relocations_[index];
}

void CoilObject::updateSymbol(uint16_t index, const Symbol& symbol) {
  if (index >= symbols_.size()) {
      throw std::out_of_range("Symbol index out of range");
  }
  symbols_[index] = symbol;
}

void CoilObject::updateSection(uint16_t index, const Section& section) {
  if (index >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  sections_[index] = section;
}

void CoilObject::updateSectionData(uint16_t index, const std::vector<uint8_t>& data) {
  if (index >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  sections_[index].data = data;
  sections_[index].size = static_cast<uint32_t>(data.size());
}

void CoilObject::setSectionSize(uint16_t index, uint32_t size) {
  if (index >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  sections_[index].size = size;
}

void CoilObject::setSymbolSectionIndex(uint16_t symbolIndex, uint16_t sectionIndex) {
  if (symbolIndex >= symbols_.size()) {
      throw std::out_of_range("Symbol index out of range");
  }
  if (sectionIndex != 0xFFFF && sectionIndex >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  symbols_[symbolIndex].section_index = sectionIndex;
}

void CoilObject::clearSectionData(uint16_t sectionIndex) {
  if (sectionIndex >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  sections_[sectionIndex].data.clear();
  sections_[sectionIndex].size = 0;
}

uint16_t CoilObject::findSymbol(const std::string& name) const {
  for (size_t i = 0; i < symbols_.size(); i++) {
      if (symbols_[i].name == name) {
          return static_cast<uint16_t>(i);
      }
  }
  return UINT16_MAX; // Not found
}

uint16_t CoilObject::getSymbolCount() const {
  return static_cast<uint16_t>(symbols_.size());
}

uint16_t CoilObject::getSectionCount() const {
  return static_cast<uint16_t>(sections_.size());
}

uint16_t CoilObject::getRelocationCount() const {
  return static_cast<uint16_t>(relocations_.size());
}

std::vector<uint8_t> CoilObject::encode() const {
  // Calculate offsets and total size
  uint32_t offset = sizeof(CoilHeader);
  
  // Symbol table
  uint32_t symbol_offset = offset;
  uint32_t symbol_size = 4; // For count
  for (const auto& symbol : symbols_) {
      symbol_size += 2 + symbol.name.size() + 4 + 4 + 2 + 1;
  }
  offset += symbol_size;
  
  // Section table
  uint32_t section_offset = offset;
  uint32_t section_size = 4; // For count
  for (const auto& section : sections_) {
      section_size += 2 + 4 + 4 + 4 + 4 + 4 + 1 + section.data.size();
  }
  offset += section_size;
  
  // Relocation table
  uint32_t relocation_offset = 0;
  uint32_t relocation_size = 0;
  if (!relocations_.empty()) {
      relocation_offset = offset;
      relocation_size = 4; // For count
      for (const auto& relocation : relocations_) {
          relocation_size += 4 + 2 + 2 + 1 + 1;
      }
      offset += relocation_size;
  }
  
  // Total file size
  uint32_t file_size = offset;
  
  // Create result vector
  std::vector<uint8_t> result;
  result.reserve(file_size);
  
  // Update header
  CoilHeader header = header_;
  header.symbol_offset = symbol_offset;
  header.section_offset = section_offset;
  header.reloc_offset = relocation_offset;
  header.file_size = file_size;
  
  // Add header
  auto header_data = header.encode();
  result.insert(result.end(), header_data.begin(), header_data.end());
  
  // Add symbol table
  uint32_t symbol_count = static_cast<uint32_t>(symbols_.size());
  result.push_back(symbol_count & 0xFF);
  result.push_back((symbol_count >> 8) & 0xFF);
  result.push_back((symbol_count >> 16) & 0xFF);
  result.push_back((symbol_count >> 24) & 0xFF);
  
  for (const auto& symbol : symbols_) {
      auto symbol_data = symbol.encode();
      result.insert(result.end(), symbol_data.begin(), symbol_data.end());
  }
  
  // Add section table
  uint32_t section_count = static_cast<uint32_t>(sections_.size());
  result.push_back(section_count & 0xFF);
  result.push_back((section_count >> 8) & 0xFF);
  result.push_back((section_count >> 16) & 0xFF);
  result.push_back((section_count >> 24) & 0xFF);
  
  for (const auto& section : sections_) {
      auto section_data = section.encode();
      result.insert(result.end(), section_data.begin(), section_data.end());
  }
  
  // Add relocation table
  if (!relocations_.empty()) {
      uint32_t relocation_count = static_cast<uint32_t>(relocations_.size());
      result.push_back(relocation_count & 0xFF);
      result.push_back((relocation_count >> 8) & 0xFF);
      result.push_back((relocation_count >> 16) & 0xFF);
      result.push_back((relocation_count >> 24) & 0xFF);
      
      for (const auto& relocation : relocations_) {
          auto relocation_data = relocation.encode();
          result.insert(result.end(), relocation_data.begin(), relocation_data.end());
      }
  }
  
  return result;
}

CoilObject CoilObject::decode(const std::vector<uint8_t>& data) {
  CoilObject obj;
  size_t offset = 0;
  
  // Decode header
  obj.header_ = CoilHeader::decode(data, offset);
  
  // Validate header
  if (!obj.header_.isValid()) {
      throw std::runtime_error("Invalid COIL header");
  }
  
  // Decode symbol table
  offset = obj.header_.symbol_offset;
  if (data.size() < offset + 4) {
      throw std::runtime_error("Insufficient data for symbol count");
  }
  
  uint32_t symbol_count = data[offset] | (data[offset + 1] << 8) | 
                        (data[offset + 2] << 16) | (data[offset + 3] << 24);
  offset += 4;
  
  for (uint32_t i = 0; i < symbol_count; i++) {
      obj.symbols_.push_back(Symbol::decode(data, offset));
  }
  
  // Decode section table
  offset = obj.header_.section_offset;
  if (data.size() < offset + 4) {
      throw std::runtime_error("Insufficient data for section count");
  }
  
  uint32_t section_count = data[offset] | (data[offset + 1] << 8) | 
                          (data[offset + 2] << 16) | (data[offset + 3] << 24);
  offset += 4;
  
  for (uint32_t i = 0; i < section_count; i++) {
      obj.sections_.push_back(Section::decode(data, offset));
  }
  
  // Decode relocation table
  if (obj.header_.reloc_offset > 0) {
      offset = obj.header_.reloc_offset;
      if (data.size() < offset + 4) {
          throw std::runtime_error("Insufficient data for relocation count");
      }
      
      uint32_t relocation_count = data[offset] | (data[offset + 1] << 8) | 
                                (data[offset + 2] << 16) | (data[offset + 3] << 24);
      offset += 4;
      
      for (uint32_t i = 0; i < relocation_count; i++) {
          obj.relocations_.push_back(Relocation::decode(data, offset));
      }
  }
  
  return obj;
}

void CoilObject::addInstruction(uint16_t sectionIndex, uint8_t opcode, const std::vector<uint8_t>& operands) {
  if (sectionIndex >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  
  // Get section
  Section& section = sections_[sectionIndex];
  
  // Add opcode and operand count
  section.data.push_back(opcode);
  section.data.push_back(static_cast<uint8_t>(operands.size()));
  
  // Add operands
  section.data.insert(section.data.end(), operands.begin(), operands.end());
  
  // Update section size
  section.size = static_cast<uint32_t>(section.data.size());
}

void CoilObject::addInstruction(uint16_t sectionIndex, const Instruction& instruction) {
  if (sectionIndex >= sections_.size()) {
      throw std::out_of_range("Section index out of range");
  }
  
  // Encode instruction
  std::vector<uint8_t> encoded = instruction.encode();
  
  // Add to section data
  Section& section = sections_[sectionIndex];
  section.data.insert(section.data.end(), encoded.begin(), encoded.end());
  
  // Update section size
  section.size = static_cast<uint32_t>(section.data.size());
}

} // namespace coil