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
    u8 magic[4] = { COIL_MAGIC[0], COIL_MAGIC[1], COIL_MAGIC[2], COIL_MAGIC[3] };   ///< Magic number (COIL_MAGIC)
    u16 version = COIL_VERSION; ///< Format version
    u16 section_count = 0;      ///< Section Count
    u64 file_size = 0;          ///< Complete object size

    ObjectHeader() = default;
  };

  /**
  * @brief Section header
  */
  struct SectionHeader {
    u64 name = 0;     ///< Offset into string table for name
    u64 size = 0;             ///< Section size in bytes
    u16 flags = 0;            ///< Section flags
    u8 type = 0;              ///< Section type
  };

  /**
  * @brief Symbol table entry
  */
  struct Symbol {
    u64 name = 0;             ///< Symbol name (string table offset)
    u32 value = 0;            ///< Symbol value
    u16 section_index = 0;    ///< Section index
    u8 type = 0;              ///< Type information
    u8 binding = 0;           ///< Binding information
  };
  
  /**
  * @brief Section data container
  */
  struct Section {
    SectionHeader header;
    
    // Unionised vectors, maybe the worst thing to exist
    // I am tempted to just switch it to pointers and utilize malloc
    union {
    std::vector<u8> data;
    std::vector<Symbol> symbols;
    };

    explicit Section(SectionHeader info) : header(info) {
      if (info.type == (u8)SectionType::SymTab) {
        new (&symbols) std::vector<Symbol>();
      } else {
        new (&data) std::vector<u8>();
      }
    }

    // DO NOT USE THIS
    // THIS IS FOR RESIZE IN LOADING OF SECTIONs
    // THIS COULD CAUSE ISSUES IN SYMBOL TABLE SECTIONS
    // SERIOUSLY THIS FUNCTION IS REALLY INCREDIBLY DANGEROUS
    Section() {
      new (&data) std::vector<u8>();
    }

    ~Section() {
      if (header.type == (u8)SectionType::SymTab) {
        symbols.~vector<Symbol>();
      } else {
        data.~vector<u8>();
      }
    }

    // Copy constructor
    Section(const Section& other) : header(other.header) {
      if (header.type == (u8)SectionType::SymTab) {
        new (&symbols) std::vector<Symbol>(other.symbols);
      } else {
        new (&data) std::vector<u8>(other.data);
      }
    }

    // Move constructor
    Section(Section&& other) noexcept : header(std::move(other.header)) {
      if (header.type == (u8)SectionType::SymTab) {
        new (&symbols) std::vector<Symbol>(std::move(other.symbols));
      } else {
        new (&data) std::vector<u8>(std::move(other.data));
      }
    }
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
    Result save(Stream& stream);

    // -------------------------------- Section Functionality -------------------------------- //
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSectionIndex(const char *name, size_t namelen);

    /**
    * @brief Get a section by index
    * indicies start at 1 as 0 is used as an error code
    */
    Section* getSection(u16 index) { 
      if ((size_t)(index - 1) > this->sections.size()) { return nullptr; } // out of bounds
      return this->sections.data() + (index - 1); 
    }

    /**
    * @brief Put a section
    * data can be a nullptr if no data is needed or the section should be created empty
    */
    u16 putSection(u64 section_name, u16 flags, u8 type, u64 size, const u8 *data, u64 datasize);
    u16 putSection(const SectionHeader& info, const u8 *data, u64 datasize);

    // -------------------------------- Symbol Table Functionality -------------------------------- //
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSymbolIndex(const char *name, size_t namelen);

    /**
    * @brief Get a symbol by index
    * indicies start at 1 as 0 is used as an error code
    */
    Symbol* getSymbol(u16 index) { 
      if (!symtab) { return nullptr; } // symbol table not initalized
      if ((size_t)(index - 1) > symtab->symbols.size()) { return nullptr; } // out of bounds
      return symtab->symbols.data() + (index - 1);
    }

    /**
    * @brief Put a symbol in the symbol table
    */
    u16 putSymbol(u64 name, u32 value, u16 section_index, u8 type, u8 binding);
    u16 putSymbol(const Symbol& symbol);

    // -------------------------------- String Table Functionality -------------------------------- //
    /**
    * @brief get a string at the specified offset in the string table
    */
    const char *getString(u64 offset) {
      if (!strings) return nullptr; // string table not initalized
      if (strtab->symbols.size() < offset) { return nullptr; } // string table too small
      return strings + offset;
    }

    /**
    * @brief put string into the string table
    */
    Result putString(const char *str);

    // -------------------------------- Members -------------------------------- //
    ObjectHeader header;
    std::vector<Section> sections;
    Section *strtab = nullptr;
    Section *symtab = nullptr;
    const char *strings = nullptr; // strab->data.data() shorthand
  };
}; // namespace coil
