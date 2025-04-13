#pragma once


namespace coil {
  enum FmtFlags : uint16_t {
    OBJECT = (1 << 0),            // Contains a single object
    OPTIMIZED = (1 << 1),         // COIL code has been optimized back into COIL normally for redistubutable purposes
    DEBUG_INFO = (1 << 2),        // Contains debug information
    FORMAT_BIG_ENDIAN = (1 << 3), // Big-endian encoding (default is little-endian)
  };
  enum SectFlags : uint16_t {
    SECTF_EXEC = (1 << 0), // Executable section
    SECTF_READ = (1 << 1), // Readable section
    SECTF_WRIT = (1 << 2), // Writable section
    SECTF_INIT = (1 << 3), // Initalized section
    SECTF_LINK = (1 << 4), // Contains relocations
  };
  enum SymFlags : uint16_t {
    SYMF_GLOB = (1 << 0), // Global symbol (visible outside the file)
    SYMF_WEAK = (1 << 1), // Weak symbol (can be overridden)
    SYMF_LOCL = (1 << 2), // Local symbol (file scope only)
    SYMF_FUNC = (1 << 3), // Function symbol
    SYMF_DATA = (1 << 4), // Data symbol
    SYMF_ABS  = (1 << 5), // Absolute symbol (fixed address)
    SYMF_COMM = (1 << 6), // Common symbol (uninitialized)
    SYMF_EXPO = (1 << 7), // Exported symbol
  };
  enum RelocType : uint16_t {
    RLCT_ABS  = (1 << 0), // Absolute (fill with symbol value) 
    RLCT_REL  = (1 << 1), // Relative (symbol value - current location)
    RLCT_PREL = (1 << 2), // PC-relative (for branch instructions)
    RLCT_SREL = (1 << 3), // Section-relative
    RLCT_ADD  = (1 << 4), // Symbol+Addend
  };

  struct CoilHeader {
    uint8_t magic[4] = {'C', 'O', 'I', 'L'}; // COIl
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
    Header() = default;
  };
  struct Symbol {
    uint16_t name_length = 0;   // Length of symbol name
    std::string name;           // Symbol name
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
  struct Section {
    uint16_t name_index = 0;      // Symbol table index for name
    uint32_t attributes = 0;      // Section attributes
    uint32_t offset = 0;          // Offset from file start
    uint32_t size = 0;            // Size in bytes
    uint32_t address = 0;         // Virtual address
    uint32_t alignment = 0;       // Required alignment
    uint8_t  processor_type = 0;  // Target processor
    std::vector<uint8_t> data;    // Section data (memory owned by object)
  
    // Serialize
    void encode(StreamWriter &writer) const;
    
    // Deserialize
    Section(StreamReader &reader);
    void decode(StreamReader &reader);
  };
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
  };

  struct SymbolTable {
    std::vector<Symbol> _symbols;

    // Add a symbol to the table
    uint16_t addSymbol(const Symbol& symbol);

    // Get a symbol by index
    const Symbol& getSymbol(uint16_t index) const;

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
  struct SectionTable {
    std::vector<Section> _sections;

    // Add a symbol to the table
    uint16_t addSection(const Section& section);

    // Get a Section by index
    const Section& getSection(uint16_t index) const;

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
  struct RelocationTable {
    std::vector<Relocation> _relocations;

    // Add a symbol to the table
    uint16_t addRelocation(const Relocation& relocation);

    // Get a Relocation by index
    const Relocation& getRelocation(uint16_t index) const;

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

  class CoilObject {
  public:
    CoilObject() = default;

    // Serialize
    void encode(StreamWriter &writer) const;
      
    // Deserialize
    void decode(StreamReader &reader);
    CoilObject(StreamReader &reader);

    CoilHeader header;
    SymbolTable symbolTable;
    SectionTable sectionTable;
    RelocationTable relocTable;
  };
};

