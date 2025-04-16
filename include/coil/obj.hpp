/**
* @file obj.hpp
* @brief COIL object format definitions
*/

#pragma once
#include "coil/types.hpp"
#include "coil/stream.hpp"
#include <cstddef>

namespace coil {

/**
* @brief Object file types
*/
enum class ObjType : u16 {
  None = 0,          ///< No specific type
  Relocatable = 1,   ///< Relocatable object
  Shared = 2,        ///< Shared library
};

/**
* @brief Section types
*/
enum class SectionType : u32 {
  None = 0,          ///< No specific type
  Code = 1,          ///< COIL code
  Data = 2,          ///< Initialized data
  Bss = 3,           ///< Uninitialized data
  SymTab = 4,        ///< Symbol table
  StrTab = 5,        ///< String table
  RelTable = 6,      ///< Relocation table
};

/**
* @brief Section flags
*/
enum class SectionFlag : u32 {
  None       = 0,        ///< No flags
  Writable   = 1 << 0,   ///< Section is writable
  Allocate   = 1 << 1,   ///< Section occupies memory at runtime
  Executable = 1 << 2,   ///< Section contains coil code (executable to make it explicit to developers getting into this codebase)
  Strings    = 1 << 3,   ///< Section contains strings
};

/**
* @brief Bitwise OR operator for section flags
*/
inline SectionFlag operator|(SectionFlag a, SectionFlag b) {
  return static_cast<SectionFlag>(
      static_cast<u32>(a) | static_cast<u32>(b)
  );
}

/**
* @brief Bitwise AND operator for section flags
*/
inline bool operator&(SectionFlag a, SectionFlag b) {
  return (static_cast<u32>(a) & static_cast<u32>(b)) != 0;
}

/**
* @brief Symbol types
*/
enum class SymbolType : u8 {
  None = 0,          ///< No specific type
  Object = 1,        ///< Data object
  Function = 2,      ///< Function
  Section = 3,       ///< Section
  File = 4,          ///< File
};

/**
* @brief Symbol binding
*/
enum class SymbolBinding : u8 {
  Local = 0,         ///< Local symbol
  Global = 1,        ///< Global symbol
  Weak = 2,          ///< Weak symbol
};

/**
* @brief Relocation types
*/
enum class RelocationType : u8 {
  None = 0,          ///< No specific type
  Abs32 = 1,         ///< 32-bit absolute
  Abs64 = 2,         ///< 64-bit absolute
  Rel32 = 3,         ///< 32-bit relative
};

/**
* @brief Section header
*/
struct SectionHeader {
  u32 name_offset;    ///< Offset into string table
  u32 type;           ///< Section type
  u32 flags;          ///< Section flags
  u32 addr;           ///< Virtual address
  u32 offset;         ///< File offset
  u32 size;           ///< Section size
  u16 link;           ///< Section link
  u16 info;           ///< Extra information
  u16 align;          ///< Alignment
  u16 entry_size;     ///< Entry size for tables
};

/**
* @brief Section data
*/
struct Section {
  SectionHeader header;    ///< Section header
  const u8* data;          ///< Section data (not owned)
  char name[32];           ///< Section name (fixed size)
};

/**
* @brief Symbol table entry
*/
struct Symbol {
  u32 name;           ///< String table offset of name
  u32 value;          ///< Symbol value
  u32 size;           ///< Symbol size
  u8  info;           ///< Type and binding
  u8  other;          ///< Reserved
  u16 section_index;  ///< Section index
  
  /**
    * @brief Set the symbol type and binding
    */
  void setTypeAndBinding(SymbolType type, SymbolBinding binding) {
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
  u32 offset;         ///< Offset within section
  u32 symbol_index;   ///< Symbol table index
  u8  type;           ///< Relocation type
  u8  size;           ///< Relocation size (8, 16, 32, 64)
  i32 addend;         ///< Addend value
};

// Forward declaration for SymbolTable and RelocationTable
struct SymbolTableEntry;
struct RelocationTableEntry;

/**
* @brief Maximum sections in an object
*/
constexpr size_t MAX_SECTIONS = 16;
constexpr size_t MAX_SYMBOLS = 1024;
constexpr size_t MAX_RELOCATIONS = 1024;
constexpr size_t INITIAL_STRTAB_SIZE = 1024;

/**
* @brief Object file
* 
* Represents a COIL object file with sections and symbols.
*/
class Object {
public:
  /**
   * @brief Initialize object with default values
   */
  Object();

  /**
    * @brief Create a new object file
    */
  static Object create(ObjType type);
  
  /**
    * @brief Load an object from a stream
    */
  static Result load(Stream& stream, Object& obj);
  
  /**
    * @brief Save the object to a stream
    */
  Result save(Stream& stream);
  
  /**
    * @brief Add a section to the object
    */
  Result addSection(const char* name, SectionType type, SectionFlag flags, 
                    const u8* data, u32 size);
  
  /**
    * @brief Get a section by name
    */
  const Section* getSection(const char* name) const;
  
  /**
    * @brief Get a section by index
    */
  const Section* getSection(u16 index) const;
  
  /**
    * @brief Add a symbol to the object
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
  Result addRelocation(u16 section_index, u32 offset, u32 symbol_index,
                      RelocationType type, u8 size, i32 addend = 0);
  
  /**
    * @brief Get object type
    */
  ObjType getType() const { return static_cast<ObjType>(type); }
  
  /**
    * @brief Get section count
    */
  u16 getSectionCount() const { return section_count; }
  
  /**
   * @brief Destructor to clean up temporary buffers
   */
  ~Object();

private:
  /**
   * @brief Add string to string table and return offset
   */
  u32 addStringToTable(const char* str);

  /**
   * @brief Find or create a section of the given type
   * @param name Section name
   * @param type Section type
   * @param flags Section flags
   * @return Index of the section, or MAX_SECTIONS if not found/created
   */
  u16 findOrCreateSection(const char* name, SectionType type, SectionFlag flags);

  /**
   * @brief Finalize symbol and string tables for saving
   */
  Result finalizeTablesForSave();

  // Object properties
  u16 type;               ///< Object type
  u16 section_count;      ///< Number of sections
  u16 str_table_index;    ///< String table index
  u16 sym_table_index;    ///< Symbol table index
  Section sections[MAX_SECTIONS]; ///< Sections

  // Working storage for building symbol and string tables
  struct SymbolEntry {
    Symbol symbol;
    const char* name;  // name pointer (not owned)
  };

  struct RelocationEntry {
    u16 section_index;
    Relocation relocation;
  };

  // These vectors will store symbols and relocations during object construction
  SymbolEntry symbols[MAX_SYMBOLS];
  u32 symbol_count;
  
  RelocationEntry relocations[MAX_RELOCATIONS];
  u32 relocation_count;

  // String table working buffer
  char strtab_buffer[INITIAL_STRTAB_SIZE];
  u32 strtab_size;
};

} // namespace coil