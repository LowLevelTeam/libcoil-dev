/**
 * @file obj.hpp
 * @brief COIL optimized object format definition
 * 
 * Defines a compact binary format for storing COIL code, inspired by ELF
 * but specialized for the needs of the Computer Oriented Intermediate Language (COIL).
 * Designed for optimal storage and linking of COIL code.
 */

#pragma once
#include "coil/types.hpp"
#include "coil/stream.hpp"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <span>

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
  Null = 0,         // Null section
  ProgBits = 1,     // Program space with data
  SymTab = 2,       // Symbol table
  StrTab = 3,       // String table
  RelTab = 4,       // Relocation entries
  NoBits = 5,       // Program space with no data (bss)
  Debug = 6         // Debug information
};

/**
 * @brief Section flags
 */
enum class SectionFlag : u16 {
  None = 0,          // No flags
  Write = 1 << 0,    // Writable
  Code = 1 << 1,     // Compile this section as COIL
  Merge = 1 << 2,    // Might be merged
  Alloc = 1 << 3,    // Occupies memory during execution
  TLS = 1 << 4       // Thread-local storage
};

// Bitwise operators for SectionFlag
inline SectionFlag operator|(SectionFlag a, SectionFlag b) {
  return static_cast<SectionFlag>(
    static_cast<u16>(a) | static_cast<u16>(b)
  );
}

inline bool operator&(SectionFlag a, SectionFlag b) {
  return (static_cast<u16>(a) & static_cast<u16>(b)) != 0;
}

/**
 * @brief Symbol types
 */
enum class SymbolType : u8 {
  NoType = 0,       // Type not specified
  Object = 1,       // Data object
  Func = 2,         // Function
  Section = 3,      // Section
  File = 4          // File
};

/**
 * @brief Symbol binding
 */
enum class SymbolBinding : u8 {
  Local = 0,        // Local symbol
  Global = 1,       // Global symbol
  Weak = 2          // Weak symbol
};

/**
 * @brief COIL Object file header
 * 
 * Fixed-size header at the beginning of every COIL object file
 */
struct ObjectHeader {
  u8 magic[4] = { COIL_MAGIC[0], COIL_MAGIC[1], COIL_MAGIC[2], COIL_MAGIC[3] };  // Magic number (COIL_MAGIC)
  u16 version = COIL_VERSION;    // Format version
  u16 section_count = 0;         // Section Count
  u64 file_size = 0;             // Complete object size
};

/**
 * @brief Section header
 */
struct SectionHeader {
  u64 name = 0;     // Offset into string table for name
  u64 size = 0;     // Section size in bytes
  u16 flags = 0;    // Section flags
  u8 type = 0;      // Section type
};

/**
 * @brief Symbol table entry
 */
struct Symbol {
  u64 name = 0;             // Symbol name (string table offset)
  u32 value = 0;            // Symbol value
  u16 section_index = 0;    // Section index
  u8 type = 0;              // Type information
  u8 binding = 0;           // Binding information
};

// Forward declarations
class BaseSection;
class DataSection;
class SymbolSection;
class Object;

/**
 * @brief Base section class - abstract type for section polymorphism
 */
class BaseSection {
public:
  explicit BaseSection(const SectionHeader& header);
  virtual ~BaseSection() = default;
  
  // No copy
  BaseSection(const BaseSection&) = delete;
  BaseSection& operator=(const BaseSection&) = delete;
  
  // Allow move
  BaseSection(BaseSection&&) = default;
  BaseSection& operator=(BaseSection&&) = default;
  
  /**
   * @brief Get section header
   */
  const SectionHeader& getHeader() const { return m_header; }
  
  /**
   * @brief Get mutable section header
   */
  SectionHeader& getMutHeader() { return m_header; }
  
  /**
   * @brief Set section header
   */
  void setHeader(const SectionHeader& h) { m_header = h; }
  
  /**
   * @brief Save section to stream
   */
  virtual void save(Stream& stream) const = 0;
  
  /**
   * @brief Load section from stream
   */
  virtual void load(Stream& stream) = 0;
  
  /**
   * @brief Get section size
   */
  virtual u64 getSize() const = 0;
  
  /**
   * @brief Get section type
   */
  virtual u8 getSectionType() const { return m_header.type; }
  
  /**
   * @brief Clone section
   */
  virtual std::unique_ptr<BaseSection> clone() const = 0;
  
protected:
  SectionHeader m_header;
};

/**
 * @brief Data section - holds raw binary data
 */
class DataSection : public BaseSection {
public:
  explicit DataSection(const SectionHeader& header);
  
  /**
   * @brief Save section to stream
   */
  void save(Stream& stream) const override;
  
  /**
   * @brief Load section from stream
   */
  void load(Stream& stream) override;
  
  /**
   * @brief Get section size
   */
  u64 getSize() const override { return m_data.size(); }
  
  /**
   * @brief Get section data
   */
  const std::vector<u8>& getData() const { return m_data; }
  
  /**
   * @brief Get mutable section data
   */
  std::vector<u8>& getData() { return m_data; }
  
  /**
   * @brief Clone section
   */
  std::unique_ptr<BaseSection> clone() const override;
  
private:
  std::vector<u8> m_data;
};

/**
 * @brief Symbol section - holds symbol table entries
 */
class SymbolSection : public BaseSection {
public:
  explicit SymbolSection(const SectionHeader& header);
  
  /**
   * @brief Save section to stream
   */
  void save(Stream& stream) const override;
  
  /**
   * @brief Load section from stream
   */
  void load(Stream& stream) override;
  
  /**
   * @brief Get section size
   */
  u64 getSize() const override { return m_symbols.size() * sizeof(Symbol); }
  
  /**
   * @brief Get symbols
   */
  const std::vector<Symbol>& getSymbols() const { return m_symbols; }
  
  /**
   * @brief Get mutable symbols
   */
  std::vector<Symbol>& getSymbols() { return m_symbols; }
  
  /**
   * @brief Add a symbol
   * @return Index of the added symbol (1-based, 0 indicates error)
   */
  u16 addSymbol(const Symbol& symbol);
  
  /**
   * @brief Get symbol by index (1-based index)
   * @return Pointer to symbol or nullptr if not found
   */
  const Symbol* getSymbol(u16 index) const;
  
  /**
   * @brief Get mutable symbol by index (1-based index)
   * @return Pointer to symbol or nullptr if not found
   */
  Symbol* getSymbol(u16 index);
  
  /**
   * @brief Clone section
   */
  std::unique_ptr<BaseSection> clone() const override;
  
private:
  std::vector<Symbol> m_symbols;
};

/**
 * @brief Factory function to create appropriate section type
 */
std::unique_ptr<BaseSection> createSection(const SectionHeader& header);

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
   * @brief Create a new COIL object
   * @return Newly created object
   */
  static Object create();
  
  // -------------------------------- Stream Functionality -------------------------------- //
  
  /**
   * @brief Load an object from a stream
   */
  void load(Stream& stream);
  
  /**
   * @brief Save an object to a stream
   */
  void save(Stream& stream) const;
  
  // -------------------------------- Section Functionality -------------------------------- //
  
  /**
   * @brief Get a section index by name
   * @return Section index (1-based, 0 indicates not found)
   */
  u16 getSectionIndex(std::string_view name) const;
  
  /**
   * @brief Get a section by name
   * @return Pointer to section or nullptr if not found
   */
  BaseSection* getSection(std::string_view name);
  
  /**
   * @brief Get a section by index
   * @return Pointer to section or nullptr if not found
   */
  BaseSection* getSection(u16 index);
  
  /**
   * @brief Get a const section by index
   * @return Const pointer to section or nullptr if not found
   */
  const BaseSection* getSection(u16 index) const;
  
  /**
   * @brief Add a section
   * @return Index of new section (1-based, 0 indicates error)
   */
  u16 addSection(u64 name_offset, u16 flags, u8 type, u64 size, std::span<const u8> data = {});
  
  /**
   * @brief Add a section
   * @return Index of new section (1-based, 0 indicates error)
   */
  u16 addSection(const SectionHeader& header, std::span<const u8> data = {});
  
  /**
   * @brief Get the number of sections
   */
  u16 getSectionCount() const { return m_header.section_count; }
  
  // -------------------------------- Symbol Table Functionality -------------------------------- //
  
  /**
   * @brief Initialize symbol table if not already present
   */
  void initSymbolTable();
  
  /**
   * @brief Get symbol table section
   * @return Pointer to symbol table section or nullptr if not present
   */
  SymbolSection* getSymbolTable() { return m_symtab; }
  
  /**
   * @brief Get const symbol table section
   * @return Const pointer to symbol table section or nullptr if not present
   */
  const SymbolSection* getSymbolTable() const { return m_symtab; }
  
  /**
   * @brief Get a symbol index by name
   * @return Symbol index (1-based, 0 indicates not found)
   */
  u16 getSymbolIndex(std::string_view name) const;
  
  /**
   * @brief Get a symbol by index
   * @return Pointer to symbol or nullptr if not found
   */
  Symbol* getSymbol(u16 index);
  
  /**
   * @brief Get a const symbol by index
   * @return Const pointer to symbol or nullptr if not found
   */
  const Symbol* getSymbol(u16 index) const;
  
  /**
   * @brief Add a symbol
   * @return Index of new symbol (1-based, 0 indicates error)
   */
  u16 addSymbol(u64 name, u32 value, u16 section_index, u8 type, u8 binding);
  
  /**
   * @brief Add a symbol
   * @return Index of new symbol (1-based, 0 indicates error)
   */
  u16 addSymbol(const Symbol& symbol);
  
  // -------------------------------- String Table Functionality -------------------------------- //
  
  /**
   * @brief Initialize string table if not already present
   */
  void initStringTable();
  
  /**
   * @brief Get string table section
   * @return Pointer to string table section or nullptr if not present
   */
  DataSection* getStringTable() { return m_strtab; }
  
  /**
   * @brief Get const string table section
   * @return Const pointer to string table section or nullptr if not present
   */
  const DataSection* getStringTable() const { return m_strtab; }
  
  /**
   * @brief Get a string from the string table
   * @return String or empty string if invalid
   */
  std::string_view getString(u64 offset) const;
  
  /**
   * @brief Add a string to the string table
   * @return Offset of the string in table, or 0 on error
   */
  u64 addString(std::string_view str);
  
  /**
   * @brief Get object header
   */
  const ObjectHeader& getHeader() const { return m_header; }
  
private:
  ObjectHeader m_header;
  std::vector<std::unique_ptr<BaseSection>> m_sections;
  DataSection* m_strtab = nullptr;    // Pointer to string table (owned by sections vector)
  SymbolSection* m_symtab = nullptr;  // Pointer to symbol table (owned by sections vector)
};

} // namespace coil