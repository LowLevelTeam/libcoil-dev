#include <coil/object_file.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstring>

namespace coil {

// ObjectHeader implementation
ObjectHeader::ObjectHeader() {
    // Set magic header
    magic[0] = 'C';
    magic[1] = 'O';
    magic[2] = 'I';
    magic[3] = 'L';
    
    // Set version (1.1.0)
    version = 0x010100;
    
    // Initialize other fields to zero
    flags = 0;
    target_pu = 0;
    target_arch = 0;
    target_mode = 0;
    entry_point = 0;
    section_count = 0;
    symbol_count = 0;
    reloc_count = 0;
    section_offset = 0;
    symbol_offset = 0;
    string_offset = 0;
    reloc_offset = 0;
    endianness = 0; // little-endian
    
    // Clear padding
    std::memset(padding, 0, sizeof(padding));
}

bool ObjectHeader::validate() const {
    // Check magic header
    if (magic[0] != 'C' || magic[1] != 'O' || magic[2] != 'I' || magic[3] != 'L') {
        return false;
    }
    
    // Check version compatibility (major version must match)
    uint8_t majorVersion = (version >> 16) & 0xFF;
    if (majorVersion != 1) {
        return false;
    }
    
    // Check for valid offsets
    if (section_count > 0 && section_offset == 0) {
        return false;
    }
    
    if (symbol_count > 0 && symbol_offset == 0) {
        return false;
    }
    
    if (reloc_count > 0 && reloc_offset == 0) {
        return false;
    }
    
    // String table is always required
    if (string_offset == 0) {
        return false;
    }
    
    // Endianness must be 0 or 1
    if (endianness > 1) {
        return false;
    }
    
    return true;
}

std::vector<uint8_t> ObjectHeader::encode() const {
    std::vector<uint8_t> result(sizeof(ObjectHeader));
    std::memcpy(result.data(), this, sizeof(ObjectHeader));
    return result;
}

ObjectHeader ObjectHeader::decode(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + sizeof(ObjectHeader) > data.size()) {
        throw std::out_of_range("Insufficient data for object header");
    }
    
    ObjectHeader header;
    std::memcpy(&header, data.data() + offset, sizeof(ObjectHeader));
    return header;
}

// SectionEntry implementation
SectionEntry::SectionEntry() 
    : type(0), flags(0), offset(0), size(0), addr(0), align(0), 
      name_idx(0), link(0), info(0) {
}

std::vector<uint8_t> SectionEntry::encode() const {
    std::vector<uint8_t> result(sizeof(SectionEntry));
    std::memcpy(result.data(), this, sizeof(SectionEntry));
    return result;
}

SectionEntry SectionEntry::decode(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + sizeof(SectionEntry) > data.size()) {
        throw std::out_of_range("Insufficient data for section entry");
    }
    
    SectionEntry entry;
    std::memcpy(&entry, data.data() + offset, sizeof(SectionEntry));
    return entry;
}

// SymbolEntry implementation
SymbolEntry::SymbolEntry() 
    : name_idx(0), section_idx(0), value(0), size(0), 
      type(0), bind(0), visibility(0), reserved(0) {
}

std::vector<uint8_t> SymbolEntry::encode() const {
    std::vector<uint8_t> result(sizeof(SymbolEntry));
    std::memcpy(result.data(), this, sizeof(SymbolEntry));
    return result;
}

SymbolEntry SymbolEntry::decode(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + sizeof(SymbolEntry) > data.size()) {
        throw std::out_of_range("Insufficient data for symbol entry");
    }
    
    SymbolEntry entry;
    std::memcpy(&entry, data.data() + offset, sizeof(SymbolEntry));
    return entry;
}

// RelocationEntry implementation
RelocationEntry::RelocationEntry() 
    : offset(0), symbol_idx(0), type(0), addend(0) {
}

std::vector<uint8_t> RelocationEntry::encode() const {
    std::vector<uint8_t> result(sizeof(RelocationEntry));
    std::memcpy(result.data(), this, sizeof(RelocationEntry));
    return result;
}

RelocationEntry RelocationEntry::decode(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + sizeof(RelocationEntry) > data.size()) {
        throw std::out_of_range("Insufficient data for relocation entry");
    }
    
    RelocationEntry entry;
    std::memcpy(&entry, data.data() + offset, sizeof(RelocationEntry));
    return entry;
}

// Section implementation
Section::Section(const SectionEntry& entry, const std::vector<uint8_t>& data)
    : entry_(entry), data_(data) {
}

SectionType Section::getType() const {
    return static_cast<SectionType>(entry_.type);
}

uint32_t Section::getFlags() const {
    return entry_.flags;
}

uint64_t Section::getSize() const {
    return entry_.size;
}

uint64_t Section::getAddress() const {
    return entry_.addr;
}

uint64_t Section::getAlignment() const {
    return entry_.align;
}

uint32_t Section::getNameIndex() const {
    return entry_.name_idx;
}

const std::vector<uint8_t>& Section::getData() const {
    return data_;
}

bool Section::hasFlag(SectionFlag flag) const {
    return (entry_.flags & static_cast<uint32_t>(flag)) != 0;
}

void Section::setData(const std::vector<uint8_t>& data) {
    data_ = data;
    entry_.size = data.size();
}

void Section::appendData(const std::vector<uint8_t>& data) {
    data_.insert(data_.end(), data.begin(), data.end());
    entry_.size = data_.size();
}

const SectionEntry& Section::getEntry() const {
    return entry_;
}

// Symbol implementation
Symbol::Symbol(const SymbolEntry& entry)
    : entry_(entry) {
}

Symbol::Symbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
               uint64_t size, SymbolType type, SymbolBinding binding, 
               SymbolVisibility visibility) {
    entry_.name_idx = 0; // Will be set when added to ObjectFile
    entry_.section_idx = sectionIndex;
    entry_.value = value;
    entry_.size = size;
    entry_.type = static_cast<uint16_t>(type);
    entry_.bind = static_cast<uint16_t>(binding);
    entry_.visibility = static_cast<uint16_t>(visibility);
    entry_.reserved = 0;
}

uint32_t Symbol::getNameIndex() const {
    return entry_.name_idx;
}

uint32_t Symbol::getSectionIndex() const {
    return entry_.section_idx;
}

uint64_t Symbol::getValue() const {
    return entry_.value;
}

uint64_t Symbol::getSize() const {
    return entry_.size;
}

SymbolType Symbol::getType() const {
    return static_cast<SymbolType>(entry_.type);
}

SymbolBinding Symbol::getBinding() const {
    return static_cast<SymbolBinding>(entry_.bind);
}

SymbolVisibility Symbol::getVisibility() const {
    return static_cast<SymbolVisibility>(entry_.visibility);
}

void Symbol::setNameIndex(uint32_t index) {
    entry_.name_idx = index;
}

const SymbolEntry& Symbol::getEntry() const {
    return entry_;
}

// Relocation implementation
Relocation::Relocation(const RelocationEntry& entry)
    : entry_(entry) {
}

Relocation::Relocation(uint64_t offset, uint32_t symbolIndex, RelocationType type, int64_t addend) {
    entry_.offset = offset;
    entry_.symbol_idx = symbolIndex;
    entry_.type = static_cast<uint32_t>(type);
    entry_.addend = addend;
}

uint64_t Relocation::getOffset() const {
    return entry_.offset;
}

uint32_t Relocation::getSymbolIndex() const {
    return entry_.symbol_idx;
}

RelocationType Relocation::getType() const {
    return static_cast<RelocationType>(entry_.type);
}

int64_t Relocation::getAddend() const {
    return entry_.addend;
}

const RelocationEntry& Relocation::getEntry() const {
    return entry_;
}

// ObjectFile implementation
ObjectFile::ObjectFile() {
    valid_ = true;
    
    // Initialize string table with a null byte
    stringTable_.push_back(0);
    
    // Add an empty string at index 0
    stringMap_[""] = 0;
}

ObjectFile::ObjectFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        valid_ = false;
        error_ = "Failed to open file: " + filename;
        return;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    // Read entire file
    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    
    if (!file) {
        valid_ = false;
        error_ = "Failed to read file: " + filename;
        return;
    }
    
    // Parse the binary data
    parseFromBinary(data);
}

ObjectFile::ObjectFile(const std::vector<uint8_t>& data) {
    parseFromBinary(data);
}

bool ObjectFile::isValid() const {
    return valid_;
}

std::string ObjectFile::getError() const {
    return error_;
}

const ObjectHeader& ObjectFile::getHeader() const {
    return header_;
}

const std::vector<Section>& ObjectFile::getSections() const {
    return sections_;
}

const std::vector<Symbol>& ObjectFile::getSymbols() const {
    return symbols_;
}

const std::vector<Relocation>& ObjectFile::getRelocations() const {
    return relocations_;
}

std::string ObjectFile::getString(uint32_t index) const {
    if (index >= stringTable_.size()) {
        return "";
    }
    
    // Find the null terminator
    size_t end = index;
    while (end < stringTable_.size() && stringTable_[end] != 0) {
        end++;
    }
    
    // Convert byte sequence to string
    return std::string(reinterpret_cast<const char*>(stringTable_.data() + index), end - index);
}

uint32_t ObjectFile::addString(const std::string& str) {
    // Check if this string already exists
    auto it = stringMap_.find(str);
    if (it != stringMap_.end()) {
        return it->second;
    }
    
    // Add the string to the string table
    uint32_t index = static_cast<uint32_t>(stringTable_.size());
    stringTable_.insert(stringTable_.end(), str.begin(), str.end());
    stringTable_.push_back(0); // Null terminator
    
    // Remember the index
    stringMap_[str] = index;
    
    return index;
}

uint32_t ObjectFile::addSection(const Section& section) {
    sections_.push_back(section);
    header_.section_count = static_cast<uint32_t>(sections_.size());
    return static_cast<uint32_t>(sections_.size() - 1);
}

void ObjectFile::removeSection(uint32_t index) {
    if (index < sections_.size()) {
        sections_.erase(sections_.begin() + index);
        header_.section_count = static_cast<uint32_t>(sections_.size());
    }
}

Section& ObjectFile::getSection(uint32_t index) {
    if (index >= sections_.size()) {
        throw std::out_of_range("Section index out of range");
    }
    return sections_[index];
}

uint32_t ObjectFile::findSection(const std::string& name) const {
    // Look up the name in the string table
    auto it = stringMap_.find(name);
    if (it == stringMap_.end()) {
        return static_cast<uint32_t>(-1); // Not found
    }
    
    uint32_t nameIndex = it->second;
    
    // Find the section with this name
    for (size_t i = 0; i < sections_.size(); i++) {
        if (sections_[i].getNameIndex() == nameIndex) {
            return static_cast<uint32_t>(i);
        }
    }
    
    return static_cast<uint32_t>(-1); // Not found
}

uint32_t ObjectFile::addSymbol(const Symbol& symbol) {
    symbols_.push_back(symbol);
    header_.symbol_count = static_cast<uint32_t>(symbols_.size());
    return static_cast<uint32_t>(symbols_.size() - 1);
}

void ObjectFile::removeSymbol(uint32_t index) {
    if (index < symbols_.size()) {
        symbols_.erase(symbols_.begin() + index);
        header_.symbol_count = static_cast<uint32_t>(symbols_.size());
    }
}

Symbol& ObjectFile::getSymbol(uint32_t index) {
    if (index >= symbols_.size()) {
        throw std::out_of_range("Symbol index out of range");
    }
    return symbols_[index];
}

uint32_t ObjectFile::findSymbol(const std::string& name) const {
    // Look up the name in the string table
    auto it = stringMap_.find(name);
    if (it == stringMap_.end()) {
        return static_cast<uint32_t>(-1); // Not found
    }
    
    uint32_t nameIndex = it->second;
    
    // Find the symbol with this name
    for (size_t i = 0; i < symbols_.size(); i++) {
        if (symbols_[i].getNameIndex() == nameIndex) {
            return static_cast<uint32_t>(i);
        }
    }
    
    return static_cast<uint32_t>(-1); // Not found
}

uint32_t ObjectFile::addRelocation(const Relocation& relocation) {
    relocations_.push_back(relocation);
    header_.reloc_count = static_cast<uint32_t>(relocations_.size());
    return static_cast<uint32_t>(relocations_.size() - 1);
}

void ObjectFile::removeRelocation(uint32_t index) {
    if (index < relocations_.size()) {
        relocations_.erase(relocations_.begin() + index);
        header_.reloc_count = static_cast<uint32_t>(relocations_.size());
    }
}

Relocation& ObjectFile::getRelocation(uint32_t index) {
    if (index >= relocations_.size()) {
        throw std::out_of_range("Relocation index out of range");
    }
    return relocations_[index];
}

uint32_t ObjectFile::addCodeSection(const std::string& name, const std::vector<Instruction>& instructions) {
    // Create a new section
    SectionEntry entry;
    entry.type = static_cast<uint32_t>(SectionType::CODE);
    entry.flags = static_cast<uint32_t>(SectionFlag::EXECUTABLE) | 
                 static_cast<uint32_t>(SectionFlag::INITIALIZED) |
                 static_cast<uint32_t>(SectionFlag::ALLOC);
    entry.align = 16; // 16-byte alignment for code
    entry.name_idx = addString(name);
    
    // Convert instructions to binary
    std::vector<uint8_t> data;
    for (const auto& instruction : instructions) {
        std::vector<uint8_t> encodedInstruction = instruction.encode();
        data.insert(data.end(), encodedInstruction.begin(), encodedInstruction.end());
    }
    
    entry.size = data.size();
    
    // Create and add the section
    Section section(entry, data);
    return addSection(section);
}

uint32_t ObjectFile::addDataSection(const std::string& name, const std::vector<uint8_t>& data, bool readOnly) {
    // Create a new section
    SectionEntry entry;
    
    if (readOnly) {
        entry.type = static_cast<uint32_t>(SectionType::RODATA);
        entry.flags = static_cast<uint32_t>(SectionFlag::INITIALIZED) |
                     static_cast<uint32_t>(SectionFlag::ALLOC);
    } else {
        entry.type = static_cast<uint32_t>(SectionType::DATA);
        entry.flags = static_cast<uint32_t>(SectionFlag::WRITABLE) |
                     static_cast<uint32_t>(SectionFlag::INITIALIZED) |
                     static_cast<uint32_t>(SectionFlag::ALLOC);
    }
    
    entry.align = 8; // 8-byte alignment for data
    entry.name_idx = addString(name);
    entry.size = data.size();
    
    // Create and add the section
    Section section(entry, data);
    return addSection(section);
}

uint32_t ObjectFile::addBssSection(const std::string& name, uint64_t size) {
    // Create a new section
    SectionEntry entry;
    entry.type = static_cast<uint32_t>(SectionType::BSS);
    entry.flags = static_cast<uint32_t>(SectionFlag::WRITABLE) |
                 static_cast<uint32_t>(SectionFlag::ALLOC);
    entry.align = 8; // 8-byte alignment for BSS
    entry.name_idx = addString(name);
    entry.size = size;
    
    // BSS sections have no data
    std::vector<uint8_t> emptyData;
    
    // Create and add the section
    Section section(entry, emptyData);
    return addSection(section);
}

void ObjectFile::setEntryPoint(uint64_t address) {
    header_.entry_point = address;
}

void ObjectFile::setTargetPlatform(uint32_t pu, uint32_t arch, uint32_t mode) {
    header_.target_pu = pu;
    header_.target_arch = arch;
    header_.target_mode = mode;
}

bool ObjectFile::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::vector<uint8_t> binary = getBinary();
    file.write(reinterpret_cast<const char*>(binary.data()), binary.size());
    
    return file.good();
}

std::vector<uint8_t> ObjectFile::getBinary() const {
    // Update offsets before generating the binary
    const_cast<ObjectFile*>(this)->updateOffsets();
    
    // Calculate the total size
    size_t totalSize = sizeof(ObjectHeader);
    
    // Add sizes of tables
    size_t sectionTableSize = sections_.size() * sizeof(SectionEntry);
    size_t symbolTableSize = symbols_.size() * sizeof(SymbolEntry);
    size_t relocTableSize = relocations_.size() * sizeof(RelocationEntry);
    size_t stringTableSize = stringTable_.size();
    
    // Add sizes of section data
    size_t sectionsDataSize = 0;
    for (const auto& section : sections_) {
        sectionsDataSize += section.getData().size();
    }
    
    totalSize += sectionTableSize + symbolTableSize + relocTableSize + stringTableSize + sectionsDataSize;
    
    // Create the binary buffer
    std::vector<uint8_t> binary(totalSize);
    
    // Copy the header
    std::memcpy(binary.data(), &header_, sizeof(ObjectHeader));
    
    // Copy the section table
    size_t offset = header_.section_offset;
    for (const auto& section : sections_) {
        SectionEntry entry = section.getEntry();
        std::memcpy(binary.data() + offset, &entry, sizeof(SectionEntry));
        offset += sizeof(SectionEntry);
    }
    
    // Copy the symbol table
    offset = header_.symbol_offset;
    for (const auto& symbol : symbols_) {
        SymbolEntry entry = symbol.getEntry();
        std::memcpy(binary.data() + offset, &entry, sizeof(SymbolEntry));
        offset += sizeof(SymbolEntry);
    }
    
    // Copy the relocation table
    offset = header_.reloc_offset;
    for (const auto& reloc : relocations_) {
        RelocationEntry entry = reloc.getEntry();
        std::memcpy(binary.data() + offset, &entry, sizeof(RelocationEntry));
        offset += sizeof(RelocationEntry);
    }
    
    // Copy the string table
    offset = header_.string_offset;
    std::memcpy(binary.data() + offset, stringTable_.data(), stringTable_.size());
    
    // Copy section data
    for (const auto& section : sections_) {
        SectionEntry entry = section.getEntry();
        if (entry.size > 0) {
            const std::vector<uint8_t>& data = section.getData();
            std::memcpy(binary.data() + entry.offset, data.data(), data.size());
        }
    }
    
    return binary;
}

void ObjectFile::parseFromBinary(const std::vector<uint8_t>& data) {
    try {
        // Ensure the data is large enough for the header
        if (data.size() < sizeof(ObjectHeader)) {
            throw std::runtime_error("File too small for COIL object header");
        }
        
        // Read the header
        header_ = ObjectHeader::decode(data, 0);
        
        // Validate the header
        if (!header_.validate()) {
            throw std::runtime_error("Invalid COIL object header");
        }
        
        // Read the string table first, as it's needed for names
        if (header_.string_offset + 1 > data.size()) {
            throw std::runtime_error("Invalid string table offset");
        }
        
        // Find the end of string table (assume it extends to the next section or end of file)
        size_t stringTableEnd = data.size();
        if (header_.section_offset > header_.string_offset) stringTableEnd = std::min(stringTableEnd, header_.section_offset);
        if (header_.symbol_offset > header_.string_offset) stringTableEnd = std::min(stringTableEnd, header_.symbol_offset);
        if (header_.reloc_offset > header_.string_offset) stringTableEnd = std::min(stringTableEnd, header_.reloc_offset);
        
        // Read the string table
        stringTable_.assign(data.begin() + header_.string_offset, data.begin() + stringTableEnd);
        
        // Build the string map (for all null-terminated strings in the table)
        stringMap_.clear();
        for (size_t i = 0; i < stringTable_.size(); i++) {
            if (i == 0 || stringTable_[i - 1] == 0) {
                std::string str;
                size_t j = i;
                while (j < stringTable_.size() && stringTable_[j] != 0) {
                    str.push_back(static_cast<char>(stringTable_[j]));
                    j++;
                }
                stringMap_[str] = static_cast<uint32_t>(i);
            }
        }
        
        // Read the section table
        sections_.clear();
        for (uint32_t i = 0; i < header_.section_count; i++) {
            size_t entryOffset = header_.section_offset + i * sizeof(SectionEntry);
            if (entryOffset + sizeof(SectionEntry) > data.size()) {
                throw std::runtime_error("Invalid section entry offset");
            }
            
            SectionEntry entry = SectionEntry::decode(data, entryOffset);
            
            // Read section data
            std::vector<uint8_t> sectionData;
            if (entry.size > 0) {
                if (entry.offset + entry.size > data.size()) {
                    throw std::runtime_error("Invalid section data offset or size");
                }
                sectionData.assign(data.begin() + entry.offset, data.begin() + entry.offset + entry.size);
            }
            
            // Create and add the section
            sections_.emplace_back(entry, sectionData);
        }
        
        // Read the symbol table
        symbols_.clear();
        for (uint32_t i = 0; i < header_.symbol_count; i++) {
            size_t entryOffset = header_.symbol_offset + i * sizeof(SymbolEntry);
            if (entryOffset + sizeof(SymbolEntry) > data.size()) {
                throw std::runtime_error("Invalid symbol entry offset");
            }
            
            SymbolEntry entry = SymbolEntry::decode(data, entryOffset);
            
            // Create and add the symbol
            symbols_.emplace_back(entry);
        }
        
        // Read the relocation table
        relocations_.clear();
        for (uint32_t i = 0; i < header_.reloc_count; i++) {
            size_t entryOffset = header_.reloc_offset + i * sizeof(RelocationEntry);
            if (entryOffset + sizeof(RelocationEntry) > data.size()) {
                throw std::runtime_error("Invalid relocation entry offset");
            }
            
            RelocationEntry entry = RelocationEntry::decode(data, entryOffset);
            
            // Create and add the relocation
            relocations_.emplace_back(entry);
        }
        
        valid_ = true;
    } catch (const std::exception& e) {
        valid_ = false;
        error_ = e.what();
        
        // Clear any partially read data
        sections_.clear();
        symbols_.clear();
        relocations_.clear();
        stringTable_.clear();
        stringMap_.clear();
    }
}

void ObjectFile::updateOffsets() {
    // Calculate offsets for tables
    size_t offset = sizeof(ObjectHeader);
    
    // Section table follows the header
    header_.section_offset = offset;
    offset += sections_.size() * sizeof(SectionEntry);
    
    // Symbol table follows the section table
    header_.symbol_offset = offset;
    offset += symbols_.size() * sizeof(SymbolEntry);
    
    // Relocation table follows the symbol table
    header_.reloc_offset = offset;
    offset += relocations_.size() * sizeof(RelocationEntry);
    
    // String table follows the relocation table
    header_.string_offset = offset;
    offset += stringTable_.size();
    
    // Section data follows all tables
    for (auto& section : sections_) {
        // Align offset to section alignment
        const auto& entry = section.getEntry();
        offset = (offset + entry.align - 1) & ~(entry.align - 1);
        
        // Update section offset
        SectionEntry newEntry = entry;
        newEntry.offset = offset;
        
        // Create new section with updated entry
        Section newSection(newEntry, section.getData());
        section = newSection;
        
        // Move offset past this section's data
        offset += section.getData().size();
    }
    
    // Update counts
    header_.section_count = static_cast<uint32_t>(sections_.size());
    header_.symbol_count = static_cast<uint32_t>(symbols_.size());
    header_.reloc_count = static_cast<uint32_t>(relocations_.size());
}

} // namespace coil
