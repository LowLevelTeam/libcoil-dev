/**
* @file obj.hpp
* @brief COIL optimized object format definition
* 
* Defines a compact binary format for storing COIL code, inspired by ELF
* but specialized for the needs of the Computer Oriented intermediate language (COIL). Designed
* for optimal storage and linking of COIL code.
*/

#pragma once
#include "coil/types.hpp"
#include "coil/stream.hpp"
#include <vector>
#include <cstring>

namespace coil {
  /**
  * @brief Magic number for COIL object files
  * "COIL" in ASCII
  */
  constexpr u8 COIL_MAGIC[] = { 'C', 'O', 'I', 'L'};

  /**
  * @brief Current format version
  */
  constexpr u16 COIL_VERSION = 0x0100; // Version 1.0

  /**
  * @brief Section types
  */
  enum class SectionType : u8 {
    Null = 0,         ///< Null section
    ProgBits = 1,     ///< Progam space with data
    SymTab = 2,       ///< Symbol table
    StrTab = 3,       ///< String table
    RelTab = 4,       ///< Relocation entries
    NoBits = 5,       ///< Program space with no data (bss)
    Debug = 6         ///< Debug information
  };

  /**
  * @brief Section flags
  */
  enum class SectionFlag : u16 {
    None = 0,          ///< No flags
    Write = 1 << 0,    ///< Writable
    Code = 1 << 1,     ///< Compile this section as COIL
    Merge = 1 << 2,    ///< Might be merged
    Alloc = 1 << 3,    ///< Occupies memory during execution
    TLS = 1 << 4       ///< Thread-local storage
  };

  // This is why people hate C++
  /**
  * @brief Bitwise OR operator for section flags
  */
  inline SectionFlag operator|(SectionFlag a, SectionFlag b) {
    return static_cast<SectionFlag>(
        static_cast<u16>(a) | static_cast<u16>(b)
    );
  }

  /**
  * @brief Bitwise AND operator for section flags
  */
  inline bool operator&(SectionFlag a, SectionFlag b) {
    return (static_cast<u16>(a) & static_cast<u16>(b)) != 0;
  }

  /**
  * @brief Symbol types
  */
  enum class SymbolType : u8 {
    NoType = 0,       ///< Type not specified
    Object = 1,       ///< Data object
    Func = 2,         ///< Function
    Section = 3,      ///< Section
    File = 4          ///< File
  };
  
  /**
  * @brief Symbol binding
  */
  enum class SymbolBinding : u8 {
    Local = 0,        ///< Local symbol
    Global = 1,       ///< Global symbol
    Weak = 2          ///< Weak symbol
  };

  /**
  * @brief COIL Object file header
  * 
  * Fixed-size header at the beginning of every COIL object file
  */
  struct ObjectHeader {
    u8 magic[4] = COIL_MAGIC;   ///< Magic number (COIL_MAGIC)
    u16 version = COIL_VERSION; ///< Format version
    u16 section_count = 0;      ///< Section Count
    u32 reserved = 0;           ///< Reserved
    u32 flags = 0;              ///< Object flags
    u64 file_size = 0;          ///< Complete object size

    ObjectHeader() = default;
  };

  /**
  * @brief Section header
  */
  struct SectionHeader {
    u64 name_offset;      ///< Offset into string table for name
    u64 size;             ///< Section size in bytes
    u32 reserved;         ///< Reserved
    u16 flags;            ///< Section flags
    u8 type;              ///< Section type
  };

  /**
  * @brief Symbol table entry
  */
  struct Symbol {
    u64 name;             ///< Symbol name (string table offset)
    u32 value;            ///< Symbol value
    u16 section_index;    ///< Section index
    u8 type;              ///< Type information
    u8 binding;           ///< Binding information
  };
  
  /**
  * @brief Section data container
  */
  class Section {
    SectionHeader header;
    std::vector<uint8_t> data;
  };

  /**
  * @brief COIL Object file
  * 
  * Represents a COIL object file with sections, symbols, and a string table.
  */
  class Object {
    /**
    * @brief Create an empty object
    */
    Object() = default;

    // -------------------------------- Stream Functionality -------------------------------- //

    /**
    * @brief Load an object from a stream
    */
    Result load(Stream& stream);
    
    /**
    * @brief Save an object to a stream
    */
    Result save(Stream& stream)

    // -------------------------------- Section Functionality -------------------------------- //
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSectionIndex(const char *name, size_t namelen);

    /**
    * @brief Get a section by index
    * indicies start at 1 as 0 is used as an error code
    */
    Section* getSection(u16 index) { return this->sections.data() + (index - 1); }

    // -------------------------------- Symbol Table Functionality -------------------------------- //
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSymbolIndex(const char *name, size_t namelen);

    /**
    * @brief Get a symbol by index
    * indicies start at 1 as 0 is used as an error code
    */
    Symbol* getSymbol(u16 index) { return symbols + (index - 1); }

    // -------------------------------- String Table Functionality -------------------------------- //
    

    // -------------------------------- Members -------------------------------- //
    ObjectHeader header;
    std::vector<Section> sections;
    Section *strtab = nullptr; // shortcut to string table
    Symbol *symbols = nullptr;
    size_t symbol_count = 0;
  };
}; // namespace coil
