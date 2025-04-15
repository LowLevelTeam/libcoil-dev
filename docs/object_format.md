# COIL Object Format

## Overview

The COIL Object Format is a binary format designed for representing compiled COIL code. It draws inspiration from the ELF (Executable and Linkable Format) but is optimized specifically for COIL's instruction set and requirements, with a focus on simplicity and performance.

## Design Principles

- **Minimal Allocations**: Fixed-size structures where possible
- **Predictable Memory Usage**: Explicit management of all resources
- **Simple Interface**: C-compatible APIs with consistent patterns
- **Efficient Serialization**: Direct memory layout mapping to files
- **Flexible Embedding**: Easy to embed in other formats or memory regions

## File Structure

A COIL object file consists of:

1. **File Header**: Contains identification and general information
2. **Section Headers**: Describes each section in the file
3. **Sections**: Contains the actual data (code, data, symbols, etc.)

## File Header

The file header is defined by the `CoilHeader` structure:

```cpp
struct CoilHeader {
    std::array<uint8_t, obj::CI_NIDENT> ident;  // COIL identification bytes
    uint16_t type;         // Object file type
    uint8_t version;       // Object file version
    uint8_t reserved1;     // Reserved for future use
    uint32_t entry;        // Entry point offset
    uint32_t shoff;        // Section header offset
    uint16_t flags;        // Architecture-specific flags
    uint16_t ehsize;       // Header size
    uint16_t shentsize;    // Section header entry size
    uint16_t shnum;        // Number of section headers
    uint16_t shstrndx;     // Section name string table index
};
```

### Identification Bytes

The first 16 bytes of a COIL file identify it as a COIL object file:

- Bytes 0-4: Magic number (`0x7C`, 'C', 'O', 'I', 'L')
- Byte 5: Data encoding (1 = little-endian, 2 = big-endian)
- Byte 6: Format version
- Bytes 7-15: Reserved for future use

### File Types

COIL supports several file types:

| Value | Name | Description |
|-------|------|-------------|
| 0 | CT_NONE | No file type |
| 1 | CT_REL | Relocatable file |
| 2 | CT_EXEC | Executable file |
| 3 | CT_DYN | Shared object file |
| 4 | CT_LIB | Library file |

## Section Headers

Each section in a COIL file is described by a section header:

```cpp
struct CoilSectionHeader {
    uint32_t name;         // Section name (string table index)
    uint32_t type;         // Section type
    uint32_t flags;        // Section flags
    uint32_t offset;       // Section file offset
    uint32_t size;         // Section size in bytes
    uint16_t link;         // Link to another section
    uint16_t info;         // Additional section information
    uint16_t addralign;    // Section alignment
    uint16_t entsize;      // Entry size if section holds table
};
```

### Section Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | CST_NULL | Inactive section |
| 1 | CST_CODE | COIL code section |
| 2 | CST_DATA | Data section |
| 3 | CST_SYMTAB | Symbol table |
| 4 | CST_STRTAB | String table |
| 5 | CST_RELA | Relocation entries with addends |
| 6 | CST_HASH | Symbol hash table |
| 7 | CST_DYNAMIC | Dynamic linking information |
| 8 | CST_NOTE | Notes |
| 9 | CST_NOBITS | Occupies no space (BSS) |
| 10 | CST_REL | Relocation entries, no addends |
| 11 | CST_DYNSYM | Dynamic linker symbol table |
| 12 | CST_TYPE | Type definitions |
| 13 | CST_META | Metadata |
| 14 | CST_DEBUG | Debugging information |

### Section Flags

| Value | Name | Description |
|-------|------|-------------|
| 0x1 | CSF_WRITE | Writable section |
| 0x2 | CSF_ALLOC | Occupies memory during execution |
| 0x4 | CSF_EXEC | Executable section |
| 0x10 | CSF_MERGE | Might be merged |
| 0x20 | CSF_STRINGS | Contains null-terminated strings |
| 0x40 | CSF_CONST | Contains const data |
| 0x80 | CSF_COMPRESSED | Contains compressed data |

## Section Data

In the updated COIL library, sections are represented by the `SectionData` structure:

```cpp
struct SectionData {
    char name[64];           // Fixed-size name buffer (no heap allocation)
    CoilSectionHeader header;
    uint8_t* data;           // Owned externally or allocated as needed
    bool ownsData;           // Whether this section owns its data buffer
    
    // Helper methods for data access...
};
```

This design eliminates heap allocations for section names and clarifies data ownership.

## Symbol Table

Symbol tables (`CST_SYMTAB` and `CST_DYNSYM`) contain entries describing symbols:

```cpp
struct CoilSymbolEntry {
    uint32_t name;         // Symbol name (string table index)
    uint32_t value;        // Symbol value (offset or address)
    uint32_t size;         // Symbol size
    uint8_t info;          // Symbol type and binding
    uint8_t other;         // Symbol visibility
    uint16_t shndx;        // Section index
};
```

### Symbol Binding Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | CSB_LOCAL | Local symbol |
| 1 | CSB_GLOBAL | Global symbol |
| 2 | CSB_WEAK | Weak symbol |
| 3 | CSB_EXTERN | External symbol |

### Symbol Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | CST_NOTYPE | Symbol type is unspecified |
| 1 | CST_OBJECT | Symbol is a data object |
| 2 | CST_FUNC | Symbol is a code object |
| 3 | CST_SECTION | Symbol associated with a section |
| 4 | CST_FILE | Symbol's name is file name |
| 5 | CST_COMMON | Common data object |
| 6 | CST_TYPE_DEF | Type definition |
| 7 | CST_OPERATOR | Operator symbol |

## Relocation Entries

Relocation entries describe how to modify code or data when linking or loading:

### Relocation Without Addend

```cpp
struct CoilRelEntry {
    uint32_t offset;     // Location to apply the relocation action
    uint32_t info;       // Symbol index and relocation type
};
```

### Relocation With Addend

```cpp
struct CoilRelaEntry {
    uint32_t offset;     // Location to apply the relocation action
    uint32_t info;       // Symbol index and relocation type
    int32_t  addend;     // Constant addend used to compute value
};
```

### Common Relocation Types

| Value | Name | Description |
|-------|------|-------------|
| 0 | CR_NONE | No relocation |
| 1 | CR_DIRECT32 | Direct 32-bit |
| 2 | CR_DIRECT64 | Direct 64-bit |
| 3 | CR_PC32 | PC-relative 32-bit |
| 4 | CR_PC64 | PC-relative 64-bit |
| 5 | CR_GOT32 | 32-bit GOT entry |
| 6 | CR_PLT32 | 32-bit PLT address |
| 7 | CR_COPY | Copy symbol at runtime |
| 8 | CR_GLOB_DATA | Create GOT entry |
| 9 | CR_JMP_SLOT | Create PLT entry |

## String Tables

String tables (`CST_STRTAB`) contain null-terminated strings referenced by other sections. The first byte of a string table is always null to ensure that index 0 represents an empty string.

In the updated COIL library, string tables are represented by the `StringTable` structure:

```cpp
struct StringTable {
    // Maximum size of the string table
    static constexpr size_t MAX_SIZE = 65536;
    
    // String table data
    uint8_t data[MAX_SIZE];
    size_t size;
    
    // Methods for string operations...
};
```

This fixed-size design prevents unpredictable heap allocations.

## Special Sections

COIL defines several special sections:

| Name | Type | Description |
|------|------|-------------|
| .text | CST_CODE | Contains executable code |
| .data | CST_DATA | Contains initialized data |
| .bss | CST_NOBITS | Contains uninitialized data |
| .symtab | CST_SYMTAB | Symbol table |
| .strtab | CST_STRTAB | String table for symbol names |
| .shstrtab | CST_STRTAB | String table for section names |
| .rela.text | CST_RELA | Relocations for .text |
| .rela.data | CST_RELA | Relocations for .data |
| .types | CST_TYPE | Type definitions |
| .meta | CST_META | Metadata |
| .debug | CST_DEBUG | Debugging information |

## COIL Object Structure

The main `CoilObject` structure represents a complete object file:

```cpp
struct CoilObject {
    CoilHeader header;
    SectionData sections[MAX_SECTIONS];  // Fixed-size array (no dynamic allocation)
    size_t sectionCount;
    const Context* ctx;
    
    // Methods for object manipulation...
};
```

## Programming Interface

The COIL library provides a streamlined API for working with object files:

```cpp
// Create a new object file
CoilObject obj = CoilObject::create(coil::obj::CT_REL, 0, &ctx);

// Add a section
const char* code = "Example code data";
obj.addSection(".text", obj::CST_CODE, obj::CSF_EXEC, 
              (const uint8_t*)code, strlen(code), 0);

// Add a symbol
obj.addSymbol("main", 0, 64, obj::CST_FUNC, obj::CSB_GLOBAL, 1);

// Save to file
FileStream stream = FileStream::open("output.obj", "wb", &ctx);
obj.save(&stream);
```

## Serialization Format

When stored on disk, a COIL object file has the following format:

1. File header (fixed size)
2. Section headers (array of fixed-size headers)
3. Section data (variable size, at offsets specified in headers)

The serialization is designed to be simple and efficient, making it easy to read and write COIL objects with minimal overhead.

## Memory Efficiency

The COIL object format is designed to be memory-efficient:

1. Fixed-size arrays for sections instead of dynamic containers
2. Fixed-size buffers for common strings like section names
3. Clear ownership semantics for section data
4. Explicit buffer management with no hidden allocations

## Future Enhancements

The COIL object format is designed for extensibility. Planned enhancements include:

1. **Advanced Linking**: More sophisticated relocation types
2. **Debugger Support**: Enhanced debug information sections
3. **Optimization Metadata**: Sections for guiding optimizations
4. **Version Control**: Better handling of symbol versioning
5. **Security Features**: Signing and verification of object files