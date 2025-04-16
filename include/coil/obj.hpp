/**
* @file obj.hpp
* @brief COIL optimized object format definition
* 
* Defines a compact binary format for storing COIL code, inspired by ELF
* but specialized for the needs of the COIL intermediate language. Designed
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
constexpr u32 COIL_MAGIC = 0x434F494C;

/**
* @brief Current format version
*/
constexpr u16 COIL_VERSION = 0x0100; // Version 1.0

/**
* @brief Object file types
*/
enum class ObjType : u8 {
  Relocatable = 1,  ///< Relocatable object (.o)
  Executable = 2,   ///< Executable
  Shared = 3,       ///< Shared library (.so)
  Core = 4          ///< Core dump
};

/**
* @brief Section types
*/
enum class SectionType : u8 {
  Null = 0,         ///< Null section
  ProgBits = 1,     ///< Program data
  SymTab = 2,       ///< Symbol table
  RelTab = 3,       ///< Relocation entries
  Hash = 4,         ///< Symbol hash table
  Dynamic = 5,      ///< Dynamic linking information
  Note = 6,         ///< Notes
  NoBits = 7,       ///< Program space with no data (bss)
  Debug = 8         ///< Debug information
};

/**
* @brief Section flags
*/
enum class SectionFlag : u16 {
  None = 0,         ///< No flags
  Write = 1 << 0,   ///< Writable
  Alloc = 1 << 1,   ///< Occupies memory during execution
  Exec = 1 << 2,    ///< Executable
  Merge = 1 << 3,   ///< Might be merged
  Strings = 1 << 4, ///< Contains null-terminated strings
  Info = 1 << 5,    ///< Contains section indexes
  TLS = 1 << 6      ///< Thread-local storage
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
* @brief Relocation types
*/
enum class RelocationType : u8 {
  None = 0,         ///< No relocation
  Abs32 = 1,        ///< 32-bit absolute address
  Abs64 = 2,        ///< 64-bit absolute address
  Rel32 = 3,        ///< 32-bit PC-relative address
  Call = 4,         ///< Function call
  Got = 5,          ///< Global offset table
  Copy = 6          ///< Copy symbol at runtime
};

/**
* @brief COIL Object file header
* 
* Fixed-size header at the beginning of every COIL object file
*/
struct ObjectHeader {
  u32 magic;            ///< Magic number (COIL_MAGIC)
  u16 version;          ///< Format version
  u8 type;              ///< Object type
  u8 machine;           ///< Target machine (0 = any)
  u32 flags;            ///< Object flags
  u32 header_size;      ///< Size of this header
  u16 section_count;    ///< Number of sections
  u32 strtab_offset;    ///< Offset to string table
  u32 strtab_size;      ///< Size of string table
  u32 symtab_index;     ///< Index of symbol table section (or 0 if none)
};

/**
* @brief Section header
*/
struct SectionHeader {
  u32 name_offset;      ///< Offset into string table for name
  u8 type;              ///< Section type
  u16 flags;            ///< Section flags
  u32 addr;             ///< Virtual address in memory
  u32 offset;           ///< Offset in file
  u32 size;             ///< Section size in bytes
  u16 link;             ///< Link to another section
  u16 info;             ///< Additional information
  u16 align;            ///< Section alignment
  u16 entry_size;       ///< Entry size for tables
};

/**
* @brief Symbol table entry
*/
struct Symbol {
  u32 name;             ///< Symbol name (string table offset)
  u32 value;            ///< Symbol value
  u32 size;             ///< Symbol size
  u8 info;              ///< Type and binding information
  u8 other;             ///< Unused
  u16 section_index;    ///< Section index

  /**
    * @brief Set the symbol type and binding
    */
  void setInfo(SymbolType type, SymbolBinding binding) {
      info = (static_cast<u8>(binding) << 4) | static_cast<u8>(type);
  }

  /**
    * @brief Get the symbol type
    */
  SymbolType getType() const {
      return static_cast<SymbolType>(info & 0xF);
  }

  /**
    * @brief Get the symbol binding
    */
  SymbolBinding getBinding() const {
      return static_cast<SymbolBinding>(info >> 4);
  }
};

/**
* @brief Relocation entry
*/
struct Relocation {
  u32 offset;           ///< Offset within section
  u32 info;             ///< Symbol index and type
  i32 addend;           ///< Constant addend
  
  /**
    * @brief Set the relocation info
    */
  void setInfo(u32 symbol_index, RelocationType type) {
      info = (symbol_index << 8) | static_cast<u8>(type);
  }
  
  /**
    * @brief Get the symbol index
    */
  u32 getSymbolIndex() const {
      return info >> 8;
  }
  
  /**
    * @brief Get the relocation type
    */
  RelocationType getType() const {
      return static_cast<RelocationType>(info & 0xFF);
  }
};

/**
* @brief Section data container
*/
class Section {
public:
  Section();
  Section(const char* name, SectionType type, SectionFlag flags);
  ~Section();

  /**
    * @brief Set section data
    */
  void setData(const u8* data, u32 size);

  /**
    * @brief Add data to the section
    */
  void addData(const u8* data, u32 size);

  /**
    * @brief Get section name
    */
  const char* getName() const { return name; }
  
  /**
    * @brief Set section name
    */
  void setName(const char* section_name) {
      std::strncpy(name, section_name, sizeof(name) - 1);
      name[sizeof(name) - 1] = '\0';
  }

  /**
    * @brief Get section header
    */
  const SectionHeader& getHeader() const { return header; }
  
  /**
    * @brief Get mutable section header
    */
  SectionHeader& getMutableHeader() { return header; }
  
  /**
    * @brief Get section data
    */
  const std::vector<u8>& getData() const { return data; }
  
  /**
    * @brief Get mutable section data
    */
  std::vector<u8>& getMutableData() { return data; }
  
  /**
    * @brief Set name offset
    */
  void setNameOffset(u32 offset) { header.name_offset = offset; }
  
  /**
    * @brief Clear section data
    */
  void clear() { data.clear(); }

private:
  char name[32];        ///< Section name (for convenience)
  SectionHeader header; ///< Section header
  std::vector<u8> data; ///< Section data
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
    * @brief Create an object with type
    */
  static Object create(ObjType type);

  /**
    * @brief Load an object from a stream
    */
  static Result load(Stream& stream, Object& obj);

  /**
    * @brief Save the object to a stream
    */
  Result save(Stream& stream) const;

  /**
    * @brief Add a section to the object
    */
  Result addSection(const char* name, SectionType type, SectionFlag flags);

  /**
    * @brief Get a section by name
    */
  Section* getSection(const char* name);

  /**
    * @brief Get a section by index
    */
  Section* getSection(u16 index);

  /**
    * @brief Get section count
    */
  u16 getSectionCount() const { return static_cast<u16>(sections.size()); }

  /**
    * @brief Set section data
    */
  Result setSectionData(const char* name, const u8* data, u32 size);

  /**
    * @brief Add a symbol
    */
  Result addSymbol(const char* name, u32 value, u32 size, 
                  SymbolType type, SymbolBinding binding, u16 section_index);

  /**
    * @brief Find a symbol by name
    */
  const Symbol* findSymbol(const char* name) const;

  /**
    * @brief Add a relocation to a section
    */
  Result addRelocation(const char* section_name, u32 offset, const char* symbol_name,
                      RelocationType type, i32 addend = 0);

  /**
    * @brief Get object type
    */
  ObjType getType() const { return static_cast<ObjType>(header.type); }

  /**
    * @brief Add string to string table
    */
  u32 addString(const char* str);

  /**
    * @brief Get string from string table
    */
  const char* getString(u32 offset) const;

  /**
    * @brief Debug function to print object contents
    * @param detailed If true, print detailed information including data contents
    */
  void debugPrint(bool detailed = false) const;

private:
  /**
    * @brief Find section index by name
    */
  u16 findSectionIndex(const char* name) const;

  /**
    * @brief Find or create symbol table
    */
  Section* findOrCreateSymbolTable();

  /**
    * @brief Find or create relocation table for section
    */
  Section* findOrCreateRelocationTable(u16 section_index);

  /**
    * @brief Find symbol index by name
    */
  u32 findSymbolIndex(const char* name) const;

  /**
    * @brief Update section offsets before saving
    */
  void updateSectionOffsets() const;

  ObjectHeader header;          ///< Object file header
  std::vector<Section> sections;///< Sections
  std::vector<char> string_table;///< String table
  
  // Cache of symbol table for faster lookups
  std::vector<Symbol> symbols_cache;
};

} // namespace coil