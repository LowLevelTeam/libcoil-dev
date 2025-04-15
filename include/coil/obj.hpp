/**
* @file obj.hpp
* @brief COIL object format manipulation utilities for the COIL library.
* 
* This header provides utilities for reading, writing, and manipulating COIL
* object files. COIL object format is based on a modified ELF structure but
* optimized for the COIL instruction set and future compilation to native code.
* It supports zero-cost abstractions for manipulating various structures like
* headers, sections, symbols, and relocations.
*/

#pragma once
#include "coil/stream.hpp"
#include "coil/err.hpp"
#include "coil/instr.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>

namespace coil {

/**
* @brief COIL object format constants and flags
*/
namespace obj {

// COIL file identification
constexpr size_t CI_NIDENT = 16;        ///< Size of ident array
constexpr uint8_t COILMAG0 = 0x7C;      ///< COIL magic byte 0 
constexpr uint8_t COILMAG1 = 'C';       ///< COIL magic byte 1
constexpr uint8_t COILMAG2 = 'O';       ///< COIL magic byte 2
constexpr uint8_t COILMAG3 = 'I';       ///< COIL magic byte 3
constexpr uint8_t COILMAG4 = 'L';       ///< COIL magic byte 4

// COIL data encoding
constexpr uint8_t COILDATANONE = 0;     ///< Invalid data encoding
constexpr uint8_t COILDATA2LSB = 1;     ///< Little-endian
constexpr uint8_t COILDATA2MSB = 2;     ///< Big-endian

// COIL version
constexpr uint8_t COIL_VERSION = 1;     ///< Current COIL format version

// COIL file types
constexpr uint16_t CT_NONE = 0;         ///< No file type
constexpr uint16_t CT_REL = 1;          ///< Relocatable file
constexpr uint16_t CT_EXEC = 2;         ///< Executable file
constexpr uint16_t CT_DYN = 3;          ///< Shared object file
constexpr uint16_t CT_LIB = 4;          ///< Library file

// Section types
constexpr uint32_t CST_NULL = 0;        ///< Inactive section
constexpr uint32_t CST_CODE = 1;        ///< COIL code section
constexpr uint32_t CST_DATA = 2;        ///< Data section
constexpr uint32_t CST_SYMTAB = 3;      ///< Symbol table
constexpr uint32_t CST_STRTAB = 4;      ///< String table
constexpr uint32_t CST_RELA = 5;        ///< Relocation entries with addends
constexpr uint32_t CST_HASH = 6;        ///< Symbol hash table
constexpr uint32_t CST_DYNAMIC = 7;     ///< Dynamic linking information
constexpr uint32_t CST_NOTE = 8;        ///< Notes
constexpr uint32_t CST_NOBITS = 9;      ///< Occupies no space (BSS)
constexpr uint32_t CST_REL = 10;        ///< Relocation entries, no addends
constexpr uint32_t CST_DYNSYM = 11;     ///< Dynamic linker symbol table
constexpr uint32_t CST_TYPE = 12;       ///< Type definitions
constexpr uint32_t CST_META = 13;       ///< Metadata
constexpr uint32_t CST_DEBUG = 14;      ///< Debugging information

// Section flags
constexpr uint32_t CSF_WRITE = 0x1;      ///< Writable section
constexpr uint32_t CSF_ALLOC = 0x2;      ///< Occupies memory during execution
constexpr uint32_t CSF_EXEC = 0x4;       ///< Executable section
constexpr uint32_t CSF_MERGE = 0x10;     ///< Might be merged
constexpr uint32_t CSF_STRINGS = 0x20;   ///< Contains null-terminated strings
constexpr uint32_t CSF_CONST = 0x40;     ///< Contains const data
constexpr uint32_t CSF_COMPRESSED = 0x80; ///< Contains compressed data

// Symbol binding
constexpr uint8_t CSB_LOCAL = 0;         ///< Local symbol
constexpr uint8_t CSB_GLOBAL = 1;        ///< Global symbol
constexpr uint8_t CSB_WEAK = 2;          ///< Weak symbol
constexpr uint8_t CSB_EXTERN = 3;        ///< External symbol

// Symbol types
constexpr uint8_t CST_NOTYPE = 0;        ///< Symbol type is unspecified
constexpr uint8_t CST_OBJECT = 1;        ///< Symbol is a data object
constexpr uint8_t CST_FUNC = 2;          ///< Symbol is a code object
constexpr uint8_t CST_SECTION = 3;       ///< Symbol associated with a section
constexpr uint8_t CST_FILE = 4;          ///< Symbol's name is file name
constexpr uint8_t CST_COMMON = 5;        ///< Common data object
constexpr uint8_t CST_TYPE_DEF = 6;      ///< Type definition
constexpr uint8_t CST_OPERATOR = 7;      ///< Operator symbol

// Relocation types (common)
constexpr uint32_t CR_NONE = 0;          ///< No relocation
constexpr uint32_t CR_DIRECT32 = 1;      ///< Direct 32-bit
constexpr uint32_t CR_DIRECT64 = 2;      ///< Direct 64-bit
constexpr uint32_t CR_PC32 = 3;          ///< PC-relative 32-bit
constexpr uint32_t CR_PC64 = 4;          ///< PC-relative 64-bit
constexpr uint32_t CR_GOT32 = 5;         ///< 32-bit GOT entry
constexpr uint32_t CR_PLT32 = 6;         ///< 32-bit PLT address
constexpr uint32_t CR_COPY = 7;          ///< Copy symbol at runtime
constexpr uint32_t CR_GLOB_DATA = 8;     ///< Create GOT entry
constexpr uint32_t CR_JMP_SLOT = 9;      ///< Create PLT entry

/**
* @brief Get the name of a COIL file type
* 
* @param type COIL file type
* @return const char* String representation of the type
*/
const char* getFileTypeName(uint16_t type);

/**
* @brief Get the name of a COIL machine type
* 
* @param machine COIL machine type
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
std::string getSectionFlagsString(uint32_t flags);

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
* @brief Get the name of a relocation type
* 
* @param machine Machine type
* @param type Relocation type
* @return const char* String representation of the relocation type
*/
const char* getRelocationTypeName(uint16_t machine, uint32_t type);

} // namespace obj

// Forward declarations
class CoilObject;
class CoilSection;
class CoilSymbol;
class CoilRelocation;

/**
* @brief Represents a COIL file header
*/
struct CoilHeader {
    std::array<uint8_t, obj::CI_NIDENT> ident;  ///< COIL identification bytes
    uint16_t type;         ///< Object file type
    uint8_t version;       ///< Object file version
    uint8_t reserved1;     ///< Reserved for future use
    uint32_t entry;        ///< Entry point offset
    uint32_t shoff;        ///< Section header offset
    uint16_t flags;        ///< Architecture-specific flags
    uint16_t ehsize;       ///< Header size
    uint16_t shentsize;    ///< Section header entry size
    uint16_t shnum;        ///< Number of section headers
    uint16_t shstrndx;     ///< Section name string table index
    
    /**
    * @brief Check if this is a little-endian COIL file
    * 
    * @return true if little-endian, false if big-endian
    */
    bool isLittleEndian() const { return ident[5] == obj::COILDATA2LSB; }
};

/**
* @brief Represents a COIL section header
*/
struct CoilSectionHeader {
    uint32_t name;         ///< Section name (string table index)
    uint32_t type;         ///< Section type
    uint32_t flags;        ///< Section flags
    uint32_t offset;       ///< Section file offset
    uint32_t size;         ///< Section size in bytes
    uint16_t link;         ///< Link to another section
    uint16_t info;         ///< Additional section information
    uint16_t addralign;    ///< Section alignment
    uint16_t entsize;      ///< Entry size if section holds table
};

/**
* @brief Represents a COIL symbol table entry
*/
struct CoilSymbolEntry {
    uint32_t name;         ///< Symbol name (string table index)
    uint32_t value;        ///< Symbol value (offset or address)
    uint32_t size;         ///< Symbol size
    uint8_t info;          ///< Symbol type and binding
    uint8_t other;         ///< Symbol visibility
    uint16_t shndx;        ///< Section index
    
    /**
    * @brief Get the binding type of the symbol
    * 
    * @return uint8_t Binding type (CSB_*)
    */
    uint8_t getBinding() const { return info >> 4; }
    
    /**
    * @brief Get the type of the symbol
    * 
    * @return uint8_t Symbol type (CST_*)
    */
    uint8_t getType() const { return info & 0xF; }
    
    /**
    * @brief Set the binding type of the symbol
    * 
    * @param binding Binding type (CSB_*)
    */
    void setBinding(uint8_t binding) { info = (binding << 4) | getType(); }
    
    /**
    * @brief Set the type of the symbol
    * 
    * @param type Symbol type (CST_*)
    */
    void setType(uint8_t type) { info = (getBinding() << 4) | type; }
};

/**
* @brief Represents a COIL relocation entry without addend
*/
struct CoilRelEntry {
    uint32_t offset;     ///< Location to apply the relocation action
    uint32_t info;       ///< Symbol index and relocation type
    
    /**
    * @brief Get the symbol index of the relocation
    * 
    * @return uint16_t Symbol index
    */
    uint16_t getSymbol() const { return info >> 16; }
    
    /**
    * @brief Get the relocation type
    * 
    * @return uint16_t Relocation type
    */
    uint16_t getType() const { return info & 0xFFFF; }
    
    /**
    * @brief Set the symbol index of the relocation
    * 
    * @param symbol Symbol index
    */
    void setSymbol(uint16_t symbol) { info = (static_cast<uint32_t>(symbol) << 16) | getType(); }
    
    /**
    * @brief Set the relocation type
    * 
    * @param type Relocation type
    */
    void setType(uint16_t type) { info = (static_cast<uint32_t>(getSymbol()) << 16) | type; }
};

/**
* @brief Represents a COIL relocation entry with addend
*/
struct CoilRelaEntry {
    uint32_t offset;     ///< Location to apply the relocation action
    uint32_t info;       ///< Symbol index and relocation type
    int32_t  addend;     ///< Constant addend used to compute value to be stored
    
    /**
    * @brief Get the symbol index of the relocation
    * 
    * @return uint16_t Symbol index
    */
    uint16_t getSymbol() const { return info >> 16; }
    
    /**
    * @brief Get the relocation type
    * 
    * @return uint16_t Relocation type
    */
    uint16_t getType() const { return info & 0xFFFF; }
    
    /**
    * @brief Set the symbol index of the relocation
    * 
    * @param symbol Symbol index
    */
    void setSymbol(uint16_t symbol) { info = (static_cast<uint32_t>(symbol) << 16) | getType(); }
    
    /**
    * @brief Set the relocation type
    * 
    * @param type Relocation type
    */
    void setType(uint16_t type) { info = (static_cast<uint32_t>(getSymbol()) << 16) | type; }
};

/**
* @brief Represents a COIL section
*/
class CoilSection {
public:
    /**
    * @brief Construct a new COIL section
    * 
    * @param parent Parent COIL object
    * @param header Section header
    * @param data Section data (can be nullptr for CST_NOBITS)
    * @param name Section name
    */
    CoilSection(CoilObject& parent, const CoilSectionHeader& header, const uint8_t* data, std::string name);
    
    /**
    * @brief Get the section name
    * 
    * @return const std::string& Section name
    */
    const std::string& getName() const { return name_; }
    
    /**
    * @brief Get the section header
    * 
    * @return const CoilSectionHeader& Section header
    */
    const CoilSectionHeader& getHeader() const { return header_; }
    
    /**
    * @brief Get the section data
    * 
    * @return const uint8_t* Section data (can be nullptr for CST_NOBITS)
    */
    const uint8_t* getData() const { return data_.get(); }
    
    /**
    * @brief Get the size of the section
    * 
    * @return uint32_t Section size
    */
    uint32_t getSize() const { return header_.size; }
    
    /**
    * @brief Get the section type
    * 
    * @return uint32_t Section type
    */
    uint32_t getType() const { return header_.type; }
    
    /**
    * @brief Get the section flags
    * 
    * @return uint32_t Section flags
    */
    uint32_t getFlags() const { return header_.flags; }
    
    /**
    * @brief Check if the section has a specific flag
    * 
    * @param flag Flag to check
    * @return true if the flag is set
    */
    bool hasFlag(uint32_t flag) const { return (header_.flags & flag) != 0; }
    
    /**
    * @brief Set the section data
    * 
    * @param data New data buffer
    * @param size Size of the data
    */
    void setData(const uint8_t* data, uint32_t size);
    
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
    * @return CoilSymbolEntry Symbol at the given index
    */
    CoilSymbolEntry getSymbol(uint32_t index) const;
    
    /**
    * @brief Get a relocation from a relocation section
    * 
    * @param index Relocation index
    * @return CoilRelEntry or CoilRelaEntry depending on section type
    */
    CoilRelEntry getRel(uint32_t index) const;
    CoilRelaEntry getRela(uint32_t index) const;
    
    /**
    * @brief Get the number of entries in a table section
    * 
    * @return uint32_t Number of entries
    */
    uint32_t getEntryCount() const;
    
    /**
    * @brief Get the parent COIL object
    * 
    * @return CoilObject& Parent object
    */
    CoilObject& getParent() { return parent_; }
    
    /**
    * @brief Set a symbol in a symbol table section
    * 
    * @param index Symbol index
    * @param symbol New symbol data
    */
    void setSymbol(uint32_t index, const CoilSymbolEntry& symbol);
    
    /**
    * @brief Set a relocation in a relocation section
    * 
    * @param index Relocation index
    * @param rel New relocation data
    */
    void setRel(uint32_t index, const CoilRelEntry& rel);
    void setRela(uint32_t index, const CoilRelaEntry& rela);

private:
    CoilObject& parent_;
    CoilSectionHeader header_;
    std::shared_ptr<uint8_t[]> data_;
    std::string name_;
};

/**
* @brief Class for manipulating COIL object files
*/
class CoilObject {
public:
    /**
    * @brief Load a COIL object from a stream
    * 
    * @param stream Input stream
    * @param ctx Library context
    * @return CoilObject* Loaded object or nullptr on error
    */
    static CoilObject load(Stream& stream, const Context& ctx);
    
    /**
    * @brief Create a new COIL object
    * 
    * @param type COIL file type
    * @param machine Target architecture
    * @param ctx Library context
    * @return CoilObject* Created object or nullptr on error
    */
    static CoilObject create(uint16_t type, uint16_t machine, const Context& ctx);
    
    /**
    * @brief Check if a file is a valid COIL object file
    * 
    * @param stream Input stream
    * @return bool True if valid COIL file
    */
    static bool isCoilFile(Stream& stream);
    
    /**
    * @brief Get the COIL header
    * 
    * @return const CoilHeader& Header
    */
    const CoilHeader& getHeader() const { return header_; }
    
    /**
    * @brief Get a section by index
    * 
    * @param index Section index
    * @return const CoilSection& Section
    */
    const CoilSection& getSection(uint16_t index) const;
    
    /**
    * @brief Get a section by name
    * 
    * @param name Section name
    * @return const CoilSection* Section or nullptr if not found
    */
    const CoilSection* getSectionByName(const std::string& name) const;
    
    /**
    * @brief Get all sections
    * 
    * @return const std::vector<CoilSection>& Vector of sections
    */
    const std::vector<std::unique_ptr<CoilSection>>& getSections() const { return sections_; }
    
    /**
    * @brief Add a new section to the COIL object
    * 
    * @param name Section name
    * @param type Section type
    * @param flags Section flags
    * @param data Section data
    * @param size Data size
    * @param entsize Entry size for table sections
    * @return CoilSection* Added section or nullptr on error
    */
    CoilSection* addSection(const std::string& name, uint32_t type, uint32_t flags, 
                          const uint8_t* data, uint32_t size, uint16_t entsize = 0);
    
    /**
    * @brief Save the COIL object to a stream
    * 
    * @param stream Output stream
    * @return bool Success
    */
    bool save(Stream& stream);
    
    /**
    * @brief Iterate through all symbols in the COIL file
    * 
    * @param callback Function to call for each symbol
    */
    void forEachSymbol(std::function<void(const CoilSection&, const CoilSymbolEntry&, const std::string&)> callback) const;
    
    /**
    * @brief Iterate through all relocations in the COIL file
    * 
    * @param callback Function to call for each relocation
    */
    void forEachRelocation(std::function<void(const CoilSection&, const CoilSection&, const CoilRelEntry&)> relCallback,
                          std::function<void(const CoilSection&, const CoilSection&, const CoilRelaEntry&)> relaCallback) const;
    
    /**
    * @brief Find a symbol by name
    * 
    * @param name Symbol name
    * @return std::pair<const CoilSection*, const CoilSymbolEntry*> Symbol and containing section, or {nullptr, nullptr} if not found
    */
    std::pair<const CoilSection*, const CoilSymbolEntry*> findSymbol(const std::string& name) const;
    
    /**
    * @brief Add a symbol to the symbol table
    * 
    * @param name Symbol name
    * @param value Symbol value
    * @param size Symbol size
    * @param type Symbol type
    * @param binding Symbol binding
    * @param sectionIndex Section index
    * @return bool Success
    */
    bool addSymbol(const std::string& name, uint32_t value, uint32_t size, 
                  uint8_t type, uint8_t binding, uint16_t sectionIndex);
    
    /**
    * @brief Get the library context
    * 
    * @return const Context& Context
    */
    const Context& getContext() const { return ctx_; }
    
    /**
    * @brief Destructor
    */
    ~CoilObject() = default;
    
private:
    CoilObject(const Context& ctx);
    
    bool loadSections(Stream& stream);
    bool loadSectionData(Stream& stream);
    bool loadSectionNames();
    
    CoilHeader header_;
    std::vector<std::unique_ptr<CoilSection>> sections_;
    const Context& ctx_;
};

/**
* @brief Helper for working with COIL string tables
*/
class CoilStringTable {
public:
    /**
    * @brief Construct a new string table from a COIL section
    * 
    * @param section String table section
    */
    explicit CoilStringTable(const CoilSection& section);
    
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

} // namespace coil