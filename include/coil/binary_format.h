#ifndef COIL_BINARY_FORMAT_H
#define COIL_BINARY_FORMAT_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace coil {

/**
* Format flags for COIL files
*/
namespace FormatFlags {
  constexpr uint8_t OBJECT_FILE = 0x01;       // Object file (.coil)
  constexpr uint8_t OUTPUT_OBJECT = 0x02;     // Output object file (.coilo)
  constexpr uint8_t DEBUG_INFO = 0x04;        // Contains debug information
  constexpr uint8_t FORMAT_BIG_ENDIAN = 0x08;        // Big-endian encoding (default is little-endian)
}

/**
* Section attribute flags
*/
namespace SectionFlags {
  constexpr uint32_t EXECUTABLE = 0x01;       // Executable section
  constexpr uint32_t WRITABLE = 0x02;         // Writable section
  constexpr uint32_t READABLE = 0x04;         // Readable section
  constexpr uint32_t INITIALIZED = 0x08;      // Contains initialized data
  constexpr uint32_t UNINITIALIZED = 0x10;    // BSS section (uninitialized data)
  constexpr uint32_t LINKED = 0x20;           // Contains relocations
  constexpr uint32_t DISCARDABLE = 0x40;      // Can be removed
}

/**
* Symbol attribute flags
*/
namespace SymbolFlags {
  constexpr uint32_t GLOBAL = 0x0001;        // Global symbol (visible outside the file)
  constexpr uint32_t WEAK = 0x0002;          // Weak symbol (can be overridden)
  constexpr uint32_t LOCAL = 0x0004;         // Local symbol (file scope only)
  constexpr uint32_t FUNCTION = 0x0008;      // Function symbol
  constexpr uint32_t DATA = 0x0010;          // Data symbol
  constexpr uint32_t ABSOLUTE = 0x0020;      // Absolute symbol (fixed address)
  constexpr uint32_t COMMON = 0x0040;        // Common symbol (uninitialized)
  constexpr uint32_t EXPORTED = 0x0080;      // Exported symbol
}

/**
* Relocation types
*/
namespace RelocationType {
  constexpr uint8_t ABSOLUTE = 0x01;          // Absolute (fill with symbol value)
  constexpr uint8_t RELATIVE = 0x02;          // Relative (symbol value - current location)
  constexpr uint8_t PC_RELATIVE = 0x03;       // PC-relative (for branch instructions)
  constexpr uint8_t SECTION_RELATIVE = 0x04;  // Section-relative
  constexpr uint8_t SYMBOL_ADDEND = 0x05;     // Symbol+Addend
}

/**
* COIL Header structure
*/
struct CoilHeader {
  char     magic[4];        // "COIL"
  uint8_t  major;           // Major version
  uint8_t  minor;           // Minor version
  uint8_t  patch;           // Patch version
  uint8_t  flags;           // Format flags
  uint32_t symbol_offset;   // Offset to symbol table
  uint32_t section_offset;  // Offset to section table
  uint32_t reloc_offset;    // Offset to relocation table
  uint32_t debug_offset;    // Offset to debug info (0 if none)
  uint32_t file_size;       // Total file size
  
  // Validation method
  bool isValid() const;
  
  // Encode to binary
  std::vector<uint8_t> encode() const;
  
  // Decode from binary
  static CoilHeader decode(const std::vector<uint8_t>& data, size_t& offset);
  
  // Initialize with default values
  static CoilHeader createDefault();
};

/**
* COIL Output Header structure
*/
struct CoilOHeader {
  char     magic[4];        // "CILO"
  uint8_t  major;           // Major version
  uint8_t  minor;           // Minor version
  uint8_t  patch;           // Patch version
  uint8_t  flags;           // Format flags
  uint32_t symbol_offset;   // Offset to symbol table
  uint32_t section_offset;  // Offset to section table
  uint32_t meta_offset;     // Offset to metadata section
  uint32_t file_size;       // Total file size
  
  // Validation method
  bool isValid() const;
  
  // Encode to binary
  std::vector<uint8_t> encode() const;
  
  // Decode from binary
  static CoilOHeader decode(const std::vector<uint8_t>& data, size_t& offset);
  
  // Initialize with default values
  static CoilOHeader createDefault();
};

/**
* Symbol structure
*/
struct Symbol {
  uint16_t name_length;     // Length of symbol name
  std::string name;         // Symbol name
  uint32_t attributes;      // Symbol attributes
  uint32_t value;           // Symbol value
  uint16_t section_index;   // Section index
  uint8_t  processor_type;  // Target processor
  
  // Encode to binary
  std::vector<uint8_t> encode() const;
  
  // Decode from binary
  static Symbol decode(const std::vector<uint8_t>& data, size_t& offset);
};

/**
* Section structure
*/
struct Section {
  uint16_t name_index;      // Symbol table index for name
  uint32_t attributes;      // Section attributes
  uint32_t offset;          // Offset from file start
  uint32_t size;            // Size in bytes
  uint32_t address;         // Virtual address
  uint32_t alignment;       // Required alignment
  uint8_t  processor_type;  // Target processor
  std::vector<uint8_t> data; // Section data
  
  // Encode to binary
  std::vector<uint8_t> encode() const;
  
  // Decode from binary
  static Section decode(const std::vector<uint8_t>& data, size_t& offset);
};

/**
* Relocation structure
*/
struct Relocation {
  uint32_t offset;          // Offset within section
  uint16_t symbol_index;    // Symbol table index
  uint16_t section_index;   // Section table index
  uint8_t  type;            // Relocation type
  uint8_t  size;            // Relocation size
  
  // Encode to binary
  std::vector<uint8_t> encode() const;
  
  // Decode from binary
  static Relocation decode(const std::vector<uint8_t>& data, size_t& offset);
};

/**
* COIL Object that holds all components of a COIL file
*/
class CoilObject {
public:
  CoilObject();
  
  // Add a symbol to the object
  uint16_t addSymbol(const Symbol& symbol);
  
  // Add a section to the object
  uint16_t addSection(const Section& section);
  
  // Add a relocation to the object
  void addRelocation(const Relocation& relocation);
  
  // Get a symbol by index
  const Symbol& getSymbol(uint16_t index) const;
  
  // Get a section by index
  const Section& getSection(uint16_t index) const;
  
  // Get a relocation by index
  const Relocation& getRelocation(uint16_t index) const;
  
  // Update a symbol by index (non-const version for modifying)
  void updateSymbol(uint16_t index, const Symbol& symbol);
  
  // Update a section by index (non-const version for modifying)
  void updateSection(uint16_t index, const Section& section);
  
  // Update a section's data
  void updateSectionData(uint16_t index, const std::vector<uint8_t>& data);
  
  // Set a section's size
  void setSectionSize(uint16_t index, uint32_t size);
  
  // Update a symbol's section index
  void setSymbolSectionIndex(uint16_t symbolIndex, uint16_t sectionIndex);
  
  // Get symbol index by name
  uint16_t findSymbol(const std::string& name) const;
  
  // Get symbol count
  uint16_t getSymbolCount() const;
  
  // Get section count
  uint16_t getSectionCount() const;
  
  // Get relocation count
  uint16_t getRelocationCount() const;
  
  // Encode the entire object to binary
  std::vector<uint8_t> encode() const;
  
  // Decode a binary into this object
  static CoilObject decode(const std::vector<uint8_t>& data);
  
  // Add instruction to a section by creating binary encoding and appending to section data
  void addInstruction(uint16_t sectionIndex, uint8_t opcode, const std::vector<uint8_t>& operands);
  
  // Add instruction to a section (convenience version that encodes an Instruction object)
  void addInstruction(uint16_t sectionIndex, const class Instruction& instruction);
  
  // Clear a section's data
  void clearSectionData(uint16_t sectionIndex);
  
private:
  CoilHeader header_;
  std::vector<Symbol> symbols_;
  std::vector<Section> sections_;
  std::vector<Relocation> relocations_;
};

} // namespace coil

#endif // COIL_BINARY_FORMAT_H