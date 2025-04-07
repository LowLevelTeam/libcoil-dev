#ifndef COIL_OBJECT_FILE_H
#define COIL_OBJECT_FILE_H

#include <coil/binary_format.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace coil {

/**
 * @brief Header of a COIL object file
 */
struct ObjectHeader {
    char magic[4];           // "COIL"
    uint32_t version;        // Version in format 0xMMmmpp (Major, minor, patch)
    uint32_t flags;          // File flags
    uint32_t target_pu;      // Target processing unit
    uint32_t target_arch;    // Target architecture
    uint32_t target_mode;    // Target mode
    uint64_t entry_point;    // Entry point (or 0 if not executable)
    uint32_t section_count;  // Number of sections
    uint32_t symbol_count;   // Number of symbols
    uint32_t reloc_count;    // Number of relocations
    uint64_t section_offset; // Offset to section table
    uint64_t symbol_offset;  // Offset to symbol table
    uint64_t string_offset;  // Offset to string table
    uint64_t reloc_offset;   // Offset to relocation table
    uint8_t endianness;      // 0 = little-endian, 1 = big-endian
    uint8_t padding[7];      // Reserved for future use, must be 0
    
    // Constructor with default values
    ObjectHeader();
    
    // Validate header fields
    bool validate() const;
    
    // Convert to and from binary representation
    std::vector<uint8_t> encode() const;
    static ObjectHeader decode(const std::vector<uint8_t>& data, size_t offset = 0);
};

/**
 * @brief File flags for COIL object files
 */
enum class ObjectFileFlag : uint32_t {
    EXECUTABLE     = 0x0001, // Executable
    SHARED_OBJECT  = 0x0002, // Shared object
    POS_INDEPENDENT= 0x0004, // Position independent
    DEBUG_INFO     = 0x0008, // Debug information included
    RELOCATABLE    = 0x0010, // Relocatable object
    CPU_SPECIFIC   = 0x0020, // Contains CPU-specific code
    GPU_SPECIFIC   = 0x0040, // Contains GPU-specific code
    NPU_SPECIFIC   = 0x0080, // Contains NPU-specific code
    DSP_SPECIFIC   = 0x0100, // Contains DSP-specific code
};

/**
 * @brief Section types for COIL object files
 */
enum class SectionType : uint32_t {
    CODE           = 0x01, // Code section (.text)
    DATA           = 0x02, // Data section (.data)
    RODATA         = 0x03, // Read-only data (.rodata)
    BSS            = 0x04, // Uninitialized data (.bss)
    SYMTAB         = 0x05, // Symbol table (.symtab)
    STRTAB         = 0x06, // String table (.strtab)
    RELOC          = 0x07, // Relocation table (.rel)
    DEBUG          = 0x08, // Debug information (.debug)
    COMMENT        = 0x09, // Comment section (.comment)
    NOTE           = 0x0A, // Note section (.note)
    SPECIAL        = 0x0B, // Special section (implementation-defined)
};

/**
 * @brief Section flags for COIL object files
 */
enum class SectionFlag : uint32_t {
    WRITABLE       = 0x0001, // Writeable
    EXECUTABLE     = 0x0002, // Executable
    INITIALIZED    = 0x0004, // Initialized
    ALLOC          = 0x0008, // Occupies memory during execution
    MERGEABLE      = 0x0010, // Mergeable
    STRINGS        = 0x0020, // Contains null-terminated strings
    SYMTAB         = 0x0040, // Contains symbol table
    TLS            = 0x0080, // TLS section
    GROUP          = 0x0100, // Group section
};

/**
 * @brief Entry in the section table
 */
struct SectionEntry {
    uint32_t type;       // Section type (SectionType)
    uint32_t flags;      // Section flags (SectionFlag)
    uint64_t offset;     // Offset from file start
    uint64_t size;       // Section size in bytes
    uint64_t addr;       // Virtual address (if applicable)
    uint64_t align;      // Section alignment
    uint32_t name_idx;   // Index into string table
    uint32_t link;       // Related section (depends on type)
    uint32_t info;       // Additional information
    
    // Constructor with default values
    SectionEntry();
    
    // Convert to and from binary representation
    std::vector<uint8_t> encode() const;
    static SectionEntry decode(const std::vector<uint8_t>& data, size_t offset = 0);
};

/**
 * @brief Symbol types
 */
enum class SymbolType : uint16_t {
    NOTYPE         = 0x00, // No type
    FUNCTION       = 0x01, // Function
    DATA           = 0x02, // Data object
    SECTION        = 0x03, // Section
    FILE           = 0x04, // File
    COMMON         = 0x05, // Common block
    TLS            = 0x06, // TLS entity
};

/**
 * @brief Symbol binding types
 */
enum class SymbolBinding : uint16_t {
    LOCAL          = 0x00, // Local (not visible outside the object file)
    GLOBAL         = 0x01, // Global (visible to all objects being combined)
    WEAK           = 0x02, // Weak (visible to all objects but with lower precedence)
    UNIQUE         = 0x03, // Unique (merged with same-named symbols)
};

/**
 * @brief Symbol visibility
 */
enum class SymbolVisibility : uint16_t {
    DEFAULT        = 0x00, // Default (as defined by binding type)
    INTERNAL       = 0x01, // Internal (treated as if it has local binding)
    HIDDEN         = 0x02, // Hidden (visible but not exposed to other components)
    PROTECTED      = 0x03, // Protected (visible but cannot be overridden)
};

/**
 * @brief Entry in the symbol table
 */
struct SymbolEntry {
    uint32_t name_idx;    // Index into string table
    uint32_t section_idx; // Section containing the symbol (0=undefined)
    uint64_t value;       // Symbol value (offset or address)
    uint64_t size;        // Symbol size in bytes
    uint16_t type;        // Symbol type (SymbolType)
    uint16_t bind;        // Symbol binding (SymbolBinding)
    uint16_t visibility;  // Symbol visibility (SymbolVisibility)
    uint16_t reserved;    // Reserved for future use
    
    // Constructor with default values
    SymbolEntry();
    
    // Convert to and from binary representation
    std::vector<uint8_t> encode() const;
    static SymbolEntry decode(const std::vector<uint8_t>& data, size_t offset = 0);
};

/**
 * @brief Relocation types (architecture-specific)
 */
enum class RelocationType : uint32_t {
    ABS32          = 0x01, // Absolute 32-bit address
    ABS64          = 0x02, // Absolute 64-bit address
    PCREL32        = 0x03, // PC-relative 32-bit offset
    PCREL64        = 0x04, // PC-relative 64-bit offset
    GOTREL         = 0x05, // GOT entry relative
    PLTREL         = 0x06, // PLT entry relative
};

/**
 * @brief Entry in the relocation table
 */
struct RelocationEntry {
    uint64_t offset;      // Location to apply relocation
    uint32_t symbol_idx;  // Symbol table index
    uint32_t type;        // Relocation type (RelocationType)
    int64_t addend;       // Constant to add to symbol value
    
    // Constructor with default values
    RelocationEntry();
    
    // Convert to and from binary representation
    std::vector<uint8_t> encode() const;
    static RelocationEntry decode(const std::vector<uint8_t>& data, size_t offset = 0);
};

/**
 * @brief Section representation in a COIL object file
 */
class Section {
public:
    // Constructor with section entry and data
    Section(const SectionEntry& entry, const std::vector<uint8_t>& data);
    
    // Accessors
    SectionType getType() const;
    uint32_t getFlags() const;
    uint64_t getSize() const;
    uint64_t getAddress() const;
    uint64_t getAlignment() const;
    uint32_t getNameIndex() const;
    const std::vector<uint8_t>& getData() const;
    
    // Check if a flag is set
    bool hasFlag(SectionFlag flag) const;
    
    // Modify section data
    void setData(const std::vector<uint8_t>& data);
    void appendData(const std::vector<uint8_t>& data);
    
    // Get section entry
    const SectionEntry& getEntry() const;
    
private:
    SectionEntry entry_;
    std::vector<uint8_t> data_;
};

/**
 * @brief Symbol representation in a COIL object file
 */
class Symbol {
public:
    // Constructor with symbol entry
    explicit Symbol(const SymbolEntry& entry);
    
    // Constructor with full information
    Symbol(const std::string& name, uint32_t sectionIndex, uint64_t value, 
           uint64_t size, SymbolType type, SymbolBinding binding, 
           SymbolVisibility visibility);
    
    // Accessors
    uint32_t getNameIndex() const;
    uint32_t getSectionIndex() const;
    uint64_t getValue() const;
    uint64_t getSize() const;
    SymbolType getType() const;
    SymbolBinding getBinding() const;
    SymbolVisibility getVisibility() const;
    
    // Set symbol name index (for internal use)
    void setNameIndex(uint32_t index);
    
    // Get symbol entry
    const SymbolEntry& getEntry() const;
    
private:
    SymbolEntry entry_;
};

/**
 * @brief Relocation representation in a COIL object file
 */
class Relocation {
public:
    // Constructor with relocation entry
    explicit Relocation(const RelocationEntry& entry);
    
    // Constructor with full information
    Relocation(uint64_t offset, uint32_t symbolIndex, RelocationType type, int64_t addend);
    
    // Accessors
    uint64_t getOffset() const;
    uint32_t getSymbolIndex() const;
    RelocationType getType() const;
    int64_t getAddend() const;
    
    // Get relocation entry
    const RelocationEntry& getEntry() const;
    
private:
    RelocationEntry entry_;
};

/**
 * @brief COIL object file representation
 */
class ObjectFile {
public:
    // Constructor to create an empty object file
    ObjectFile();
    
    // Constructor to load from file
    explicit ObjectFile(const std::string& filename);
    
    // Constructor to load from memory
    explicit ObjectFile(const std::vector<uint8_t>& data);
    
    // Check if file is valid
    bool isValid() const;
    
    // Get error message
    std::string getError() const;
    
    // Accessors for file components
    const ObjectHeader& getHeader() const;
    const std::vector<Section>& getSections() const;
    const std::vector<Symbol>& getSymbols() const;
    const std::vector<Relocation>& getRelocations() const;
    
    // Get string from string table
    std::string getString(uint32_t index) const;
    
    // String table manipulation
    uint32_t addString(const std::string& str);
    
    // Section manipulation
    uint32_t addSection(const Section& section);
    void removeSection(uint32_t index);
    Section& getSection(uint32_t index);
    uint32_t findSection(const std::string& name) const;
    
    // Symbol manipulation
    uint32_t addSymbol(const Symbol& symbol);
    void removeSymbol(uint32_t index);
    Symbol& getSymbol(uint32_t index);
    uint32_t findSymbol(const std::string& name) const;
    
    // Relocation manipulation
    uint32_t addRelocation(const Relocation& relocation);
    void removeRelocation(uint32_t index);
    Relocation& getRelocation(uint32_t index);
    
    // Add code section with instructions
    uint32_t addCodeSection(const std::string& name, const std::vector<Instruction>& instructions);
    
    // Add data section with raw bytes
    uint32_t addDataSection(const std::string& name, const std::vector<uint8_t>& data, bool readOnly = false);
    
    // Add BSS section (uninitialized data)
    uint32_t addBssSection(const std::string& name, uint64_t size);
    
    // Set entry point
    void setEntryPoint(uint64_t address);
    
    // Set target platform
    void setTargetPlatform(uint32_t pu, uint32_t arch, uint32_t mode);
    
    // Save to file
    bool saveToFile(const std::string& filename) const;
    
    // Get binary representation
    std::vector<uint8_t> getBinary() const;
    
private:
    // Helper methods
    void parseFromBinary(const std::vector<uint8_t>& data);
    void updateOffsets();
    
    bool valid_ = false;
    std::string error_;
    
    ObjectHeader header_;
    std::vector<Section> sections_;
    std::vector<Symbol> symbols_;
    std::vector<Relocation> relocations_;
    std::vector<uint8_t> stringTable_;
    
    // Map to track string positions in string table
    std::unordered_map<std::string, uint32_t> stringMap_;
};

} // namespace coil

#endif // COIL_OBJECT_FILE_H
