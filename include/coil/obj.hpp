#pragma once

#include <cstdint>
#include "coil/stream.hpp"

namespace coil {
  /**
  * @brief COIL flags for file formatting
  */
  enum FmtFlags : uint16_t {
    FMTF_OBJECT = (1 << 0),     ///< Contains a single object
    FMTF_OPTIMIZED = (1 << 1),  ///< COIL code has been optimized back into COIL normally for redistubutable purposes
    FMTF_DEBUG_INFO = (1 << 2), ///< Contains debug information
    FMTF_BIG_ENDIAN = (1 << 3), ///< Big-endian encoding (default is little-endian)
    FMTF_MANGLED = (1 << 4),    ///< COIL code has been mangled in attempt to keep the source code private
  };
  

  /**
  * @brief COIL section flags
  */
  enum CoilSectFlags : uint16_t {
    SECTF_WRIT  = (1 << 0), ///< Writable
    SECTF_ALOC  = (1 << 1), ///< Occupies memory during execution
    SECTF_EXEC  = (1 << 2), ///< Executable
    SECTF_MERG  = (1 << 3), ///< Might be merged
    SECTF_STRS  = (1 << 4), ///< Contains null-terminated strings
    SECTF_LINO  = (1 << 5), ///< Preserve order after combining
    SECTF_TLS   = (1 << 6), ///< Section holds thread-local data
    SECTF_COMPR = (1 << 7)  ///< Section with compressed data
  };

  /**
  * @brief COIL section types
  */
  enum CoilSectType : uint8_t {
    SECTT_NULL = 0,
    SECTT_PROG = 1,
    SECTT_SYMT = 2,
    SECTT_STRT = 3,
    SECTT_RELA = 4,
    SECTT_HASH = 5,
    SECTT_DYN  = 6,
    SECTT_REL  = 7,
    SECTT_DYNS = 8
  };
  
  /**
  * @brief COIL Symbol Flags
  */
  enum SymFlags : uint16_t {
    SYMF_GLOB = (1 << 0),  ///< Global symbol (visible outside the file)
    SYMF_WEAK = (1 << 1),  ///< Weak symbol (can be overridden)
    SYMF_LOCL = (1 << 2),  ///< Local symbol (file scope only)
    SYMF_FUNC = (1 << 3),  ///< Function symbol
    SYMF_DATA = (1 << 4),  ///< Data symbol
    SYMF_ABS  = (1 << 5),  ///< Absolute symbol (fixed address)
    SYMF_COMM = (1 << 6),  ///< Common symbol (uninitialized)
    SYMF_EXPO = (1 << 7),  ///< Exported symbol
    SYMF_SECT = (1 << 8),  ///< Section
    SYMF_FILE = (1 << 9),  ///< Source file
    SYMF_TLS  = (1 << 10), ///< TLS object
  };
  
  /**
  * @brief COIL Symbol Flags
  */
  enum RelocType : uint16_t {
    RLCT_ABS  = (1 << 0), ///< Absolute (fill with symbol value) 
    RLCT_REL  = (1 << 1), ///< Relative (symbol value - current location)
    RLCT_PREL = (1 << 2), ///< PC-relative (for branch instructions)
    RLCT_SREL = (1 << 3), ///< Section-relative
    RLCT_ADD  = (1 << 4), ///< Symbol+Addend
  };

  /**
   * @brief Object file header
   */
  struct CoilHeader {
    uint8_t magic[4] = {'C', 'O', 'I', 'L'}; // COIL magic
    uint8_t  major = 1;              // Major version
    uint8_t  minor = 0;              // Minor version
    uint8_t  patch = 0;              // Patch version
    uint8_t  flags = 0;              // Format flags
    uint32_t symbol_table_size = 0;  // Size of symbol table
    uint32_t section_table_size = 0; // Size of section table
    uint32_t reloc_table_size = 0;   // Size of relocation table
    uint32_t debug_offset = 0;       // Offset to debug info (0 if none)
    uint32_t file_size = 0;          // Total file size

    // Serialize
    void encode(StreamWriter &writer) const;

    // Deserialize
    void decode(StreamReader &reader);
  
    // Initialize with default values
    CoilHeader() = default;
  };

  /**
   * @brief Symbol entry in symbol table
   */
  struct Symbol {
    uint16_t name_length = 0;   // Length of symbol name
    char *name;                 // Symbol name
    uint16_t attributes = 0;    // Symbol attributes
    uint16_t section_index = 0; // Section index
    uint8_t  device = 0;        // Target (defined in configuration)

    // Serialize
    void encode(StreamWriter &writer) const;
    
    // Deserialize
    Symbol(StreamReader &reader);
    void decode(StreamReader &reader);
  
    // Initialize with default values
    Symbol() = default;
  };

  /**
   * @brief Section entry in section table
   */
  struct Section {
    uint16_t name_index = 0;      // Symbol table index for name
    uint32_t attributes = 0;      // Section attributes
    uint32_t offset = 0;          // Offset from file start
    uint32_t size = 0;            // Size in bytes
    uint32_t address = 0;         // Virtual address
    uint32_t alignment = 0;       // Required alignment
    uint8_t  processor_type = 0;  // Target processor
    uint8_t *data;                // Section data (memory owned by object)
  
    // Serialize
    void encode(StreamWriter &writer) const;
    
    // Deserialize
    Section(StreamReader &reader);
    void decode(StreamReader &reader);
    
    // Default constructor
    Section() = default;
  };

  /**
   * @brief Relocation entry in relocation table
   */
  struct Relocation {
    uint32_t offset;          // Offset within section
    uint16_t symbol_index;    // Symbol table index
    uint16_t section_index;   // Section table index
    uint8_t  type;            // Relocation type
    uint8_t  size;            // Relocation size
    
    // Serialize
    void encode(StreamWriter &writer) const;
    
    // Deserialize
    Relocation(StreamReader &reader);
    void decode(StreamReader &reader);
    
    // Default constructor
    Relocation() : offset(0), symbol_index(0), section_index(0), type(0), size(0) {}
  };

  /**
   * @brief Symbol table container
   */
  class SymbolTable {
  public:
    void *_symbols;

    // Add a symbol to the table
    uint16_t addSymbol(const Symbol& symbol);

    // Get a symbol by index
    Symbol getSymbol(uint16_t index) const; // symbol is deserialized and copied

    // Update a symbol by index (non-const version for modifying)
    void updateSymbol(uint16_t index, const Symbol& symbol);

    // Get symbol index by name
    uint16_t findSymbol(const std::string& name) const;
    
    // Get symbol count
    uint16_t getSymbolCount() const;

    // Serialize
    void encode(StreamWriter &writer) const;

    // Deserialize
    void decode(StreamReader &reader);
  };

  /**
   * @brief Section table container
   */
  class SectionTable {
  public:
    void *_sections;

    // Add a section to the table
    uint16_t addSection(const Section& section);

    // Get a Section by index
    Section getSection(uint16_t index) const;

    // Update a Section by index (non-const version for modifying)
    void updateSection(uint16_t index, const Section& section);

    // Get Section index by name
    uint16_t findSection(const std::string& name) const;
    
    // Get Section count
    uint16_t getSectionCount() const;

    // Set a section's size
    void setSectionSize(uint16_t index, uint32_t size);
  
    // Serialize
    void encode(StreamWriter &writer) const;

    // Deserialize
    void decode(StreamReader &reader);
  };

  /**
   * @brief Relocation table container
   */
  class RelocationTable {
  public:
    void *_relocations;

    // Add a relocation to the table
    uint16_t addRelocation(const Relocation& relocation);

    // Get a Relocation by index
    Relocation getRelocation(uint16_t index) const;

    // Update a Relocation by index (non-const version for modifying)
    void updateRelocation(uint16_t index, const Relocation& relocation);

    // Get Relocation index by name
    uint16_t findRelocation(const std::string& name) const;
    
    // Get Relocation count
    uint16_t getRelocationCount() const;

    // Serialize
    void encode(StreamWriter &writer) const;

    // Deserialize
    void decode(StreamReader &reader);
  };

  /**
   * @brief Complete object file representation
   */
  class CoilObject {
  public:
    CoilHeader header;
    SymbolTable symbolTable;
    SectionTable sectionTable;
    RelocationTable relocTable;

    // Default constructor
    CoilObject() = default;

    // Serialize the object to a stream
    void encode(StreamWriter &writer) const;
      
    // Deserialize the object from a stream
    void decode(StreamReader &reader);
    
    // Constructor from stream
    CoilObject(StreamReader &reader);
  };
};