/**
* @file obj.hpp
* @brief ELF object format manipulation utilities for the COIL library.
* 
* This header provides utilities for reading, writing, and manipulating ELF
* (Executable and Linkable Format) object files. It supports both 32-bit and
* 64-bit ELF formats and provides zero-cost abstractions for manipulating
* various ELF structures like headers, sections, symbols, and relocations.
*/

#pragma once
#include "coil/stream.hpp"
#include "coil/err.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>

namespace coil {

/**
* @brief ELF format constants and flags
*/
namespace elf {

// ELF file identification
constexpr size_t EI_NIDENT = 16;  ///< Size of ident array
constexpr uint8_t ELFMAG0 = 0x7F;  ///< ELF magic byte 0
constexpr uint8_t ELFMAG1 = 'E';   ///< ELF magic byte 1
constexpr uint8_t ELFMAG2 = 'L';   ///< ELF magic byte 2
constexpr uint8_t ELFMAG3 = 'F';   ///< ELF magic byte 3

// ELF class types (32/64 bit)
constexpr uint8_t ELFCLASSNONE = 0; ///< Invalid class
constexpr uint8_t ELFCLASS32 = 1;   ///< 32-bit objects
constexpr uint8_t ELFCLASS64 = 2;   ///< 64-bit objects

// ELF data encoding
constexpr uint8_t ELFDATANONE = 0; ///< Invalid data encoding
constexpr uint8_t ELFDATA2LSB = 1; ///< Little-endian
constexpr uint8_t ELFDATA2MSB = 2; ///< Big-endian

// ELF file types
constexpr uint16_t ET_NONE = 0;    ///< No file type
constexpr uint16_t ET_REL = 1;     ///< Relocatable file
constexpr uint16_t ET_EXEC = 2;    ///< Executable file
constexpr uint16_t ET_DYN = 3;     ///< Shared object file
constexpr uint16_t ET_CORE = 4;    ///< Core file

// ELF machine types (architectures)
constexpr uint16_t EM_NONE = 0;    ///< No machine
constexpr uint16_t EM_386 = 3;     ///< Intel 80386
constexpr uint16_t EM_ARM = 40;    ///< ARM
constexpr uint16_t EM_X86_64 = 62; ///< AMD x86-64
constexpr uint16_t EM_AARCH64 = 183; ///< ARM AARCH64

// Section types
constexpr uint32_t SHT_NULL = 0;          ///< Inactive section
constexpr uint32_t SHT_PROGBITS = 1;      ///< Program defined data
constexpr uint32_t SHT_SYMTAB = 2;        ///< Symbol table
constexpr uint32_t SHT_STRTAB = 3;        ///< String table
constexpr uint32_t SHT_RELA = 4;          ///< Relocation entries with addends
constexpr uint32_t SHT_HASH = 5;          ///< Symbol hash table
constexpr uint32_t SHT_DYNAMIC = 6;       ///< Dynamic linking information
constexpr uint32_t SHT_NOTE = 7;          ///< Notes
constexpr uint32_t SHT_NOBITS = 8;        ///< Occupies no space (BSS)
constexpr uint32_t SHT_REL = 9;           ///< Relocation entries, no addends
constexpr uint32_t SHT_DYNSYM = 11;       ///< Dynamic linker symbol table

// Section flags
constexpr uint64_t SHF_WRITE = 0x1;       ///< Writable section
constexpr uint64_t SHF_ALLOC = 0x2;       ///< Occupies memory during execution
constexpr uint64_t SHF_EXECINSTR = 0x4;   ///< Executable section
constexpr uint64_t SHF_MERGE = 0x10;      ///< Might be merged
constexpr uint64_t SHF_STRINGS = 0x20;    ///< Contains null-terminated strings

// Symbol binding
constexpr uint8_t STB_LOCAL = 0;          ///< Local symbol
constexpr uint8_t STB_GLOBAL = 1;         ///< Global symbol
constexpr uint8_t STB_WEAK = 2;           ///< Weak symbol

// Symbol types
constexpr uint8_t STT_NOTYPE = 0;         ///< Symbol type is unspecified
constexpr uint8_t STT_OBJECT = 1;         ///< Symbol is a data object
constexpr uint8_t STT_FUNC = 2;           ///< Symbol is a code object
constexpr uint8_t STT_SECTION = 3;        ///< Symbol associated with a section
constexpr uint8_t STT_FILE = 4;           ///< Symbol's name is file name

// Relocation types (x86_64 - just a subset for example)
constexpr uint32_t R_X86_64_NONE = 0;     ///< No relocation
constexpr uint32_t R_X86_64_64 = 1;       ///< Direct 64 bit
constexpr uint32_t R_X86_64_PC32 = 2;     ///< PC relative 32 bit signed
constexpr uint32_t R_X86_64_GOT32 = 3;    ///< 32 bit GOT entry
constexpr uint32_t R_X86_64_PLT32 = 4;    ///< 32 bit PLT address

// Relocation types (ARM - just a subset for example)
constexpr uint32_t R_ARM_NONE = 0;        ///< No relocation
constexpr uint32_t R_ARM_PC24 = 1;        ///< PC relative 24 bit
constexpr uint32_t R_ARM_ABS32 = 2;       ///< Direct 32 bit
constexpr uint32_t R_ARM_REL32 = 3;       ///< PC relative 32 bit

/**
* @brief Get the name of an ELF file type
* 
* @param type ELF file type
* @return const char* String representation of the type
*/
const char* getFileTypeName(uint16_t type);

/**
* @brief Get the name of an ELF machine type
* 
* @param machine ELF machine type
* @return const char* String representation of the machine type
*/
const char* getMachineTypeName(uint16_t machine);

/**
* @brief Get the name of a section type
* 
* @param type Section type
* @return const char* String representation of the section type
*/
const char* getSectionTypeName(uint32_t type);

/**
* @brief Get a string representation of section flags
* 
* @param flags Section flags
* @return std::string String representation of the flags
*/
std::string getSectionFlagsString(uint64_t flags);

/**
* @brief Get the name of a symbol binding
* 
* @param binding Symbol binding
* @return const char* String representation of the binding
*/
const char* getSymbolBindingName(uint8_t binding);

/**
* @brief Get the name of a symbol type
* 
* @param type Symbol type
* @return const char* String representation of the type
*/
const char* getSymbolTypeName(uint8_t type);

/**
* @brief Get the name of a relocation type for a specific architecture
* 
* @param machine Machine type
* @param type Relocation type
* @return const char* String representation of the relocation type
*/
const char* getRelocationTypeName(uint16_t machine, uint32_t type);

} // namespace elf

// Forward declarations
class ElfObject;
class ElfSection;
class ElfSymbol;
class ElfRelocation;

/**
* @brief Represents an ELF file header
*/
struct ElfHeader {
    std::array<uint8_t, elf::EI_NIDENT> ident;  ///< ELF identification bytes
    uint16_t type;         ///< Object file type
    uint16_t machine;      ///< Machine type
    uint32_t version;      ///< Object file version
    uint64_t entry;        ///< Entry point
    uint64_t phoff;        ///< Program header offset
    uint64_t shoff;        ///< Section header offset
    uint32_t flags;        ///< Processor-specific flags
    uint16_t ehsize;       ///< ELF header size
    uint16_t phentsize;    ///< Program header entry size
    uint16_t phnum;        ///< Number of program headers
    uint16_t shentsize;    ///< Section header entry size
    uint16_t shnum;        ///< Number of section headers
    uint16_t shstrndx;     ///< Section name string table index
    
    /**
    * @brief Check if this is a 64-bit ELF file
    * 
    * @return true if 64-bit, false if 32-bit
    */
    bool is64Bit() const { return ident[4] == elf::ELFCLASS64; }
    
    /**
    * @brief Check if this is a little-endian ELF file
    * 
    * @return true if little-endian, false if big-endian
    */
    bool isLittleEndian() const { return ident[5] == elf::ELFDATA2LSB; }
};

/**
* @brief Represents an ELF section header
*/
struct ElfSectionHeader {
    uint32_t name;        ///< Section name (string table index)
    uint32_t type;        ///< Section type
    uint64_t flags;       ///< Section flags
    uint64_t addr;        ///< Section virtual address at execution
    uint64_t offset;      ///< Section file offset
    uint64_t size;        ///< Section size in bytes
    uint32_t link;        ///< Link to another section
    uint32_t info;        ///< Additional section information
    uint64_t addralign;   ///< Section alignment
    uint64_t entsize;     ///< Entry size if section holds table
};

/**
* @brief Represents an ELF symbol table entry
*/
struct ElfSymbolEntry {
    uint32_t name;        ///< Symbol name (string table index)
    uint8_t  info;        ///< Symbol type and binding
    uint8_t  other;       ///< Symbol visibility
    uint16_t shndx;       ///< Section index
    uint64_t value;       ///< Symbol value
    uint64_t size;        ///< Symbol size
    
    /**
    * @brief Get the binding type of the symbol
    * 
    * @return uint8_t Binding type (STB_*)
    */
    uint8_t getBinding() const { return info >> 4; }
    
    /**
    * @brief Get the type of the symbol
    * 
    * @return uint8_t Symbol type (STT_*)
    */
    uint8_t getType() const { return info & 0xF; }
    
    /**
    * @brief Set the binding type of the symbol
    * 
    * @param binding Binding type (STB_*)
    */
    void setBinding(uint8_t binding) { info = (binding << 4) | getType(); }
    
    /**
    * @brief Set the type of the symbol
    * 
    * @param type Symbol type (STT_*)
    */
    void setType(uint8_t type) { info = (getBinding() << 4) | type; }
};

/**
* @brief Represents an ELF relocation entry without addend
*/
struct ElfRelEntry {
    uint64_t offset;     ///< Location to apply the relocation action
    uint64_t info;       ///< Symbol table index and relocation type
    
    /**
    * @brief Get the symbol index of the relocation
    * 
    * @return uint32_t Symbol index
    */
    uint32_t getSymbol() const { return info >> 32; }
    
    /**
    * @brief Get the relocation type
    * 
    * @return uint32_t Relocation type
    */
    uint32_t getType() const { return info & 0xFFFFFFFF; }
    
    /**
    * @brief Set the symbol index of the relocation
    * 
    * @param symbol Symbol index
    */
    void setSymbol(uint32_t symbol) { info = (static_cast<uint64_t>(symbol) << 32) | getType(); }
    
    /**
    * @brief Set the relocation type
    * 
    * @param type Relocation type
    */
    void setType(uint32_t type) { info = (static_cast<uint64_t>(getSymbol()) << 32) | type; }
};

/**
* @brief Represents an ELF relocation entry with addend
*/
struct ElfRelaEntry {
    uint64_t offset;     ///< Location to apply the relocation action
    uint64_t info;       ///< Symbol table index and relocation type
    int64_t  addend;     ///< Constant addend used to compute value to be stored
    
    /**
    * @brief Get the symbol index of the relocation
    * 
    * @return uint32_t Symbol index
    */
    uint32_t getSymbol() const { return info >> 32; }
    
    /**
    * @brief Get the relocation type
    * 
    * @return uint32_t Relocation type
    */
    uint32_t getType() const { return info & 0xFFFFFFFF; }
    
    /**
    * @brief Set the symbol index of the relocation
    * 
    * @param symbol Symbol index
    */
    void setSymbol(uint32_t symbol) { info = (static_cast<uint64_t>(symbol) << 32) | getType(); }
    
    /**
    * @brief Set the relocation type
    * 
    * @param type Relocation type
    */
    void setType(uint32_t type) { info = (static_cast<uint64_t>(getSymbol()) << 32) | type; }
};

/**
* @brief Represents an ELF section
*/
class ElfSection {
public:
    /**
    * @brief Construct a new ELF section
    * 
    * @param parent Parent ELF object
    * @param header Section header
    * @param data Section data (can be nullptr for SHT_NOBITS)
    * @param name Section name
    */
    ElfSection(ElfObject& parent, const ElfSectionHeader& header, const uint8_t* data, std::string name);
    
    /**
    * @brief Get the section name
    * 
    * @return const std::string& Section name
    */
    const std::string& getName() const { return name_; }
    
    /**
    * @brief Get the section header
    * 
    * @return const ElfSectionHeader& Section header
    */
    const ElfSectionHeader& getHeader() const { return header_; }
    
    /**
    * @brief Get the section data
    * 
    * @return const uint8_t* Section data (can be nullptr for SHT_NOBITS)
    */
    const uint8_t* getData() const { return data_.get(); }
    
    /**
    * @brief Get the size of the section
    * 
    * @return uint64_t Section size
    */
    uint64_t getSize() const { return header_.size; }
    
    /**
    * @brief Get the section type
    * 
    * @return uint32_t Section type
    */
    uint32_t getType() const { return header_.type; }
    
    /**
    * @brief Get the section flags
    * 
    * @return uint64_t Section flags
    */
    uint64_t getFlags() const { return header_.flags; }
    
    /**
    * @brief Check if the section has a specific flag
    * 
    * @param flag Flag to check
    * @return true if the flag is set
    */
    bool hasFlag(uint64_t flag) const { return (header_.flags & flag) != 0; }
    
    /**
    * @brief Set the section data
    * 
    * @param data New data buffer
    * @param size Size of the data
    */
    void setData(const uint8_t* data, uint64_t size);
    
    /**
    * @brief Get a string from a string table section
    * 
    * @param offset Offset into the string table
    * @return std::string String at the given offset
    */
    std::string getString(uint32_t offset) const;
    
    /**
    * @brief Get a symbol from a symbol table section
    * 
    * @param index Symbol index
    * @return ElfSymbolEntry Symbol at the given index
    */
    ElfSymbolEntry getSymbol(uint32_t index) const;
    
    /**
    * @brief Get a relocation from a relocation section
    * 
    * @param index Relocation index
    * @return ElfRelEntry or ElfRelaEntry depending on section type
    */
    ElfRelEntry getRel(uint32_t index) const;
    ElfRelaEntry getRela(uint32_t index) const;
    
    /**
    * @brief Get the number of entries in a table section
    * 
    * @return uint32_t Number of entries
    */
    uint32_t getEntryCount() const;
    
    /**
    * @brief Get the parent ELF object
    * 
    * @return ElfObject& Parent object
    */
    ElfObject& getParent() { return parent_; }
    
    /**
    * @brief Set a symbol in a symbol table section
    * 
    * @param index Symbol index
    * @param symbol New symbol data
    */
    void setSymbol(uint32_t index, const ElfSymbolEntry& symbol);
    
    /**
    * @brief Set a relocation in a relocation section
    * 
    * @param index Relocation index
    * @param rel New relocation data
    */
    void setRel(uint32_t index, const ElfRelEntry& rel);
    void setRela(uint32_t index, const ElfRelaEntry& rela);
    
private:
    ElfObject& parent_;
    ElfSectionHeader header_;
    std::shared_ptr<uint8_t[]> data_;
    std::string name_;
};

/**
* @brief Class for manipulating ELF object files
*/
class ElfObject {
public:
    /**
    * @brief Load an ELF object from a stream
    * 
    * @param stream Input stream
    * @param ctx Library context
    * @return ElfObject* Loaded object or nullptr on error
    */
    static ElfObject* load(Stream& stream, const Context& ctx);
    
    /**
    * @brief Create a new ELF object
    * 
    * @param is64Bit Whether to create a 64-bit ELF file
    * @param isLittleEndian Whether to create a little-endian ELF file
    * @param type ELF file type
    * @param machine Machine type
    * @param ctx Library context
    * @return ElfObject* Created object or nullptr on error
    */
    static ElfObject* create(bool is64Bit, bool isLittleEndian, uint16_t type, 
                            uint16_t machine, const Context& ctx);
    
    /**
    * @brief Get the ELF header
    * 
    * @return const ElfHeader& Header
    */
    const ElfHeader& getHeader() const { return header_; }
    
    /**
    * @brief Get a section by index
    * 
    * @param index Section index
    * @return const ElfSection& Section
    */
    const ElfSection& getSection(uint16_t index) const;
    
    /**
    * @brief Get a section by name
    * 
    * @param name Section name
    * @return const ElfSection* Section or nullptr if not found
    */
    const ElfSection* getSectionByName(const std::string& name) const;
    
    /**
    * @brief Get all sections
    * 
    * @return const std::vector<ElfSection>& Vector of sections
    */
    const std::vector<std::unique_ptr<ElfSection>>& getSections() const { return sections_; }
    
    /**
    * @brief Add a new section to the ELF object
    * 
    * @param name Section name
    * @param type Section type
    * @param flags Section flags
    * @param data Section data
    * @param size Data size
    * @param entsize Entry size for table sections
    * @return ElfSection* Added section or nullptr on error
    */
    ElfSection* addSection(const std::string& name, uint32_t type, uint64_t flags, 
                          const uint8_t* data, uint64_t size, uint64_t entsize = 0);
    
    /**
    * @brief Save the ELF object to a stream
    * 
    * @param stream Output stream
    * @return bool Success
    */
    bool save(Stream& stream);
    
    /**
    * @brief Iterate through all symbols in the ELF file
    * 
    * @param callback Function to call for each symbol
    */
    void forEachSymbol(std::function<void(const ElfSection&, const ElfSymbolEntry&, const std::string&)> callback) const;
    
    /**
    * @brief Iterate through all relocations in the ELF file
    * 
    * @param callback Function to call for each relocation
    */
    void forEachRelocation(std::function<void(const ElfSection&, const ElfSection&, const ElfRelEntry&)> relCallback,
                          std::function<void(const ElfSection&, const ElfSection&, const ElfRelaEntry&)> relaCallback) const;
    
    /**
    * @brief Find a symbol by name
    * 
    * @param name Symbol name
    * @return std::pair<const ElfSection*, const ElfSymbolEntry*> Symbol and containing section, or {nullptr, nullptr} if not found
    */
    std::pair<const ElfSection*, const ElfSymbolEntry*> findSymbol(const std::string& name) const;
    
    /**
    * @brief Get the library context
    * 
    * @return const Context& Context
    */
    const Context& getContext() const { return ctx_; }
    
    /**
    * @brief Destructor
    */
    ~ElfObject() = default;
    
private:
    ElfObject(const Context& ctx);
    
    bool loadSections(Stream& stream);
    bool loadSectionData(Stream& stream);
    bool loadSectionNames();
    
    ElfHeader header_;
    std::vector<std::unique_ptr<ElfSection>> sections_;
    const Context& ctx_;
};

/**
* @brief Helper for working with ELF string tables
*/
class ElfStringTable {
public:
    /**
    * @brief Construct a new string table from an ELF section
    * 
    * @param section String table section
    */
    explicit ElfStringTable(const ElfSection& section);
    
    /**
    * @brief Get a string from the table
    * 
    * @param offset Offset into the string table
    * @return std::string String at the given offset
    */
    std::string getString(uint32_t offset) const;
    
    /**
    * @brief Add a string to the table
    * 
    * @param str String to add
    * @return uint32_t Offset of the added string
    */
    uint32_t addString(const std::string& str);
    
    /**
    * @brief Get the current size of the string table
    * 
    * @return uint32_t Size in bytes
    */
    uint32_t getSize() const { return static_cast<uint32_t>(data_.size()); }
    
    /**
    * @brief Get the data buffer
    * 
    * @return const uint8_t* String table data
    */
    const uint8_t* getData() const { return data_.data(); }
    
private:
    std::vector<uint8_t> data_;
};

/**
* @brief Utility functions for ELF manipulation
*/
namespace obj_util {

/**
* @brief Get a string representation of an ELF type
* 
* @param header ELF header
* @return std::string String representation
*/
std::string getElfTypeString(const ElfHeader& header);

/**
* @brief Dump basic information about an ELF file
* 
* @param obj ELF object
* @param logger Logger to use
*/
void dumpElfInfo(const ElfObject& obj, Logger& logger);

/**
* @brief Disassemble a section containing code
* 
* @param section Code section
* @param machine Machine type
* @return std::string Disassembly output
*/
std::string disassembleSection(const ElfSection& section, uint16_t machine);

/**
* @brief Check if an ELF file is compatible with the current platform
* 
* @param obj ELF object
* @return bool True if compatible
*/
bool isCompatible(const ElfObject& obj);

/**
* @brief Apply relocations to a section
* 
* @param obj ELF object
* @param targetSection Section to apply relocations to
* @param relocationSection Relocation section
* @return bool Success
*/
bool applyRelocations(ElfObject& obj, ElfSection& targetSection, const ElfSection& relocationSection);

} // namespace obj_util

} // namespace coil