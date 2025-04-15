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
#include <array>
#include <functional>

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
* @param buffer Output buffer (must not be null)
* @param size Buffer size (must be large enough to hold the result)
* @return void
*/
void getSectionFlagsString(uint32_t flags, char* buffer, size_t size);

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
  
  /**
  * @brief Initialize a header with default values
  * 
  * @param fileType Object file type (from obj::CT_* constants)
  * @param machine Machine type
  * @return CoilHeader Initialized header
  */
  static CoilHeader initialize(uint16_t fileType, uint16_t machine);
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
  * @return uint8_t Binding type (obj::CSB_*)
  */
  uint8_t getBinding() const { return info >> 4; }
  
  /**
  * @brief Get the type of the symbol
  * 
  * @return uint8_t Symbol type (obj::CST_*)
  */
  uint8_t getType() const { return info & 0xF; }
  
  /**
  * @brief Set the binding type of the symbol
  * 
  * @param binding Binding type (obj::CSB_*)
  */
  void setBinding(uint8_t binding) { info = (binding << 4) | getType(); }
  
  /**
  * @brief Set the type of the symbol
  * 
  * @param type Symbol type (obj::CST_*)
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
* @brief Section data structure with fixed-size name
*/
struct SectionData {
  char name[64];           // Fixed-size name buffer (no heap allocation)
  CoilSectionHeader header;
  uint8_t* data;           // Owned externally or allocated as needed
  bool ownsData;           // Whether this section owns its data buffer
  
  /**
  * @brief Initialize a new section data structure
  *
  * @param sectionName Section name (must not be null)
  * @param type Section type
  * @param flags Section flags
  * @param sectionData Section data (can be null for size=0)
  * @param size Data size
  * @param entrySize Entry size for table sections
  * @return SectionData Initialized section data
  */
  static SectionData create(const char* sectionName, 
                          uint32_t type, 
                          uint32_t flags,
                          const uint8_t* sectionData, 
                          uint32_t size, 
                          uint16_t entrySize = 0);
                            
  /**
  * @brief Free allocated data if owned
  */
  void freeData();
  
  /**
  * @brief Get a string from a string table section
  *
  * @param offset Offset into the string table
  * @return const char* String at the given offset, or nullptr if invalid
  */
  const char* getString(uint32_t offset) const;
  
  /**
  * @brief Get a symbol entry from a symbol table section
  *
  * @param index Symbol index
  * @return CoilSymbolEntry Symbol entry
  */
  CoilSymbolEntry getSymbol(uint32_t index) const;
  
  /**
  * @brief Set a symbol entry in a symbol table section
  *
  * @param index Symbol index
  * @param symbol Symbol entry to set
  */
  void setSymbol(uint32_t index, const CoilSymbolEntry& symbol);
  
  /**
  * @brief Get a relocation entry from a relocation section
  *
  * @param index Relocation index
  * @return CoilRelEntry Relocation entry
  */
  CoilRelEntry getRel(uint32_t index) const;
  
  /**
  * @brief Get a relocation entry with addend from a relocation section
  *
  * @param index Relocation index
  * @return CoilRelaEntry Relocation entry with addend
  */
  CoilRelaEntry getRela(uint32_t index) const;
  
  /**
  * @brief Set a relocation entry in a relocation section
  *
  * @param index Relocation index
  * @param rel Relocation entry to set
  */
  void setRel(uint32_t index, const CoilRelEntry& rel);
  
  /**
  * @brief Set a relocation entry with addend in a relocation section
  *
  * @param index Relocation index
  * @param rela Relocation entry with addend to set
  */
  void setRela(uint32_t index, const CoilRelaEntry& rela);
  
  /**
  * @brief Get the number of entries in a table section
  *
  * @return uint32_t Number of entries (0 if not a table section)
  */
  uint32_t getEntryCount() const;
};

/**
* @brief Maximum number of sections in a COIL object
*/
constexpr size_t MAX_SECTIONS = 32;

/**
* @brief COIL object file structure
*/
struct CoilObject {
  CoilHeader header;
  SectionData sections[MAX_SECTIONS];
  size_t sectionCount;
  const Context* ctx;
  
  /**
  * @brief Load a COIL object from a stream
  * 
  * @param stream Input stream (must not be null)
  * @param context Library context (must not be null)
  * @return CoilObject Loaded object or empty object on error
  */
  static CoilObject load(Stream* stream, const Context* context);
  
  /**
  * @brief Create a new COIL object
  * 
  * @param type COIL file type
  * @param machine Target architecture
  * @param context Library context (must not be null)
  * @return CoilObject Created object
  */
  static CoilObject create(uint16_t type, uint16_t machine, const Context* context);
  
  /**
  * @brief Check if a file is a valid COIL object file
  * 
  * @param stream Input stream (must not be null)
  * @return bool True if valid COIL file
  */
  static bool isCoilFile(Stream* stream);
  
  /**
  * @brief Get a section by index
  * 
  * @param index Section index
  * @return const SectionData* Section pointer or nullptr if invalid
  */
  const SectionData* getSection(uint16_t index) const;
  
  /**
  * @brief Get a section by name
  * 
  * @param name Section name (must not be null)
  * @return const SectionData* Section pointer or nullptr if not found
  */
  const SectionData* getSectionByName(const char* name) const;
  
  /**
  * @brief Add a new section to the COIL object
  * 
  * @param name Section name (must not be null)
  * @param type Section type
  * @param flags Section flags
  * @param data Section data (can be null for size=0)
  * @param size Data size
  * @param entsize Entry size for table sections
  * @return SectionData* Added section or nullptr on error
  */
  SectionData* addSection(const char* name, uint32_t type, uint32_t flags, 
                        const uint8_t* data, uint32_t size, uint16_t entsize = 0);
  
  /**
  * @brief Save the COIL object to a stream
  * 
  * @param stream Output stream (must not be null)
  * @return bool Success
  */
  bool save(Stream* stream);
  
  /**
  * @brief Find a symbol by name
  * 
  * @param name Symbol name (must not be null)
  * @return std::pair<const SectionData*, const CoilSymbolEntry*> Section/symbol pair
  *         If the symbol is not found, both pointers will be nullptr.
  */
  std::pair<const SectionData*, const CoilSymbolEntry*> findSymbol(const char* name) const;
  
  /**
  * @brief Add a symbol to the symbol table
  * 
  * @param name Symbol name (must not be null)
  * @param value Symbol value
  * @param size Symbol size
  * @param type Symbol type
  * @param binding Symbol binding
  * @param sectionIndex Section index
  * @return bool Success
  */
  bool addSymbol(const char* name, uint32_t value, uint32_t size, 
                uint8_t type, uint8_t binding, uint16_t sectionIndex);
  
  /**
  * @brief Cleanup any owned resources
  * 
  * This releases any memory owned by the object's sections.
  */
  void cleanup();
};

/**
* @brief Helper for working with COIL string tables
*/
struct StringTable {
  // Maximum size of the string table
  static constexpr size_t MAX_SIZE = 65536;
  
  // String table data
  uint8_t data[MAX_SIZE];
  size_t size;
  
  /**
  * @brief Initialize from section data
  * 
  * @param section String table section (must be a valid STRTAB section)
  * @return StringTable Initialized string table
  */
  static StringTable fromSection(const SectionData& section);
  
  /**
  * @brief Create a new string table
  * 
  * @return StringTable New string table
  */
  static StringTable create();
  
  /**
  * @brief Get a string from the table
  * 
  * @param offset Offset into the string table
  * @return const char* String at the given offset, or nullptr if invalid
  */
  const char* getString(uint32_t offset) const;
  
  /**
  * @brief Add a string to the table
  * 
  * @param str String to add (must not be null)
  * @return uint32_t Offset of the added string, or 0 if the string table is full
  */
  uint32_t addString(const char* str);
};

} // namespace coil