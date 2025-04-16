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
    ProgBits = 1,     ///< Program space with data
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
    Code = 1 << 1,     ///< Executable code
    Merge = 1 << 2,    ///< Might be merged
    Alloc = 1 << 3,    ///< Occupies memory during execution
    TLS = 1 << 4       ///< Thread-local storage
  };

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
    u8 reserved1;         ///< Reserved
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
    std::vector<u8> data;
  };

  /**
  * @brief COIL Object file
  * 
  * Represents a COIL object file with sections, symbols, and a string table.
  */
  class Object {
  public:
    /**
    * @brief Create an empty object
    */
    Object();
    
    /**
    * @brief Initialize object with specified type
    * @param type The object type (relocatable, executable, etc)
    */
    void init();
    
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
    * @brief Add a section to the object
    * @param name Section name
    * @param type Section type
    * @param flags Section flags
    * @param data Section data
    * @param size Size of section data
    * @return Index of added section (0 on error)
    */
    u16 addSection(const char* name, SectionType type, SectionFlag flags, 
                  const void* data, size_t size);
    
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSectionIndex(const char* name, size_t namelen);
    
    /**
    * @brief Get a section by name (0 on error)
    */
    u16 getSectionIndex(const char* name);

    /**
    * @brief Get a section by index
    * indices start at 1 as 0 is used as an error code
    */
    Section* getSection(u16 index);
    
    /**
    * @brief Get a const section by index
    */
    const Section* getSection(u16 index) const;

    // -------------------------------- Symbol Table Functionality -------------------------------- //
    /**
    * @brief Add a symbol to the symbol table
    * @param name Symbol name
    * @param value Symbol value
    * @param section_index Index of section
    * @param type Symbol type
    * @param binding Symbol binding
    * @return Index of added symbol (0 on error)
    */
    u16 addSymbol(const char* name, u32 value, u16 section_index, 
                 SymbolType type, SymbolBinding binding);
    
    /**
    * @brief Get a symbol by name (0 on error)
    */
    u16 getSymbolIndex(const char* name, size_t namelen);
    
    /**
    * @brief Get a symbol by name (0 on error)
    */
    u16 getSymbolIndex(const char* name);

    /**
    * @brief Get a symbol by index
    * indices start at 1 as 0 is used as an error code
    */
    Symbol* getSymbol(u16 index);
    
    /**
    * @brief Get a const symbol by index
    */
    const Symbol* getSymbol(u16 index) const;

    // -------------------------------- String Table Functionality -------------------------------- //
    /**
    * @brief Add a string to the string table
    * @param str String to add
    * @return Offset into string table (0 on error)
    */
    u64 addString(const char* str);
    
    /**
    * @brief Get a string from the string table
    * @param offset Offset into string table
    * @return Pointer to string (null on error)
    */
    const char* getString(u64 offset) const;

  private:
    /**
    * @brief Initialize string table
    */
    Result initStringTable();
    
    /**
    * @brief Initialize symbol table
    */
    Result initSymbolTable();
    
    /**
    * @brief Convert raw symbol data to structured symbols
    */
    void setupSymbolTable();
    
    ObjectHeader header;
    std::vector<Section> sections;
    Section* strtab = nullptr;    // Shortcut to string table
    Symbol* symbols = nullptr;    // Pointer to symbol array
    size_t symbol_count = 0;      // Number of symbols
    size_t symbol_capacity = 0;   // Symbol array capacity
    std::vector<u8> symbol_data;  // Raw symbol data
  };
}; // namespace coil