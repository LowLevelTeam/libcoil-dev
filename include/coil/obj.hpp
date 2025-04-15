/**
 * @file obj.hpp
 * @brief COIL object format manipulation utilities.
 * 
 * This header provides utilities for reading, writing, and manipulating COIL
 * object files. The format is designed to represent COIL instructions and data
 * efficiently while maintaining simple interfaces and minimal allocations.
 */

 #pragma once
 #include "coil/stream.hpp"
 #include "coil/err.hpp"
 #include <cstdint>
 #include <array>
 
 namespace coil {
 
 /**
  * @brief COIL object format constants and flags
  */
 namespace obj {
 
 // COIL file identification
 constexpr size_t CI_NIDENT = 8;          ///< Size of ident array
 constexpr uint8_t COILMAG0 = 0x7C;       ///< COIL magic byte 0 
 constexpr uint8_t COILMAG1 = 'C';        ///< COIL magic byte 1
 constexpr uint8_t COILMAG2 = 'O';        ///< COIL magic byte 2
 constexpr uint8_t COILMAG3 = 'I';        ///< COIL magic byte 3
 constexpr uint8_t COILMAG4 = 'L';        ///< COIL magic byte 4
 
 // COIL version
 constexpr uint8_t COIL_VERSION = 1;      ///< Current COIL format version
 
 // COIL file types
 constexpr uint16_t CT_NONE = 0;          ///< No file type
 constexpr uint16_t CT_REL = 1;           ///< Relocatable file
 constexpr uint16_t CT_EXEC = 2;          ///< Executable file
 constexpr uint16_t CT_SHARED = 3;        ///< Shared object file
 constexpr uint16_t CT_LIB = 4;           ///< Library file
 
 // Section types
 constexpr uint32_t CST_NULL = 0;         ///< Inactive section
 constexpr uint32_t CST_CODE = 1;         ///< COIL code section
 constexpr uint32_t CST_DATA = 2;         ///< Data section
 constexpr uint32_t CST_SYMTAB = 3;       ///< Symbol table
 constexpr uint32_t CST_STRTAB = 4;       ///< String table
 constexpr uint32_t CST_REL = 5;          ///< Relocation entries
 constexpr uint32_t CST_NOBITS = 6;       ///< Occupies no space (BSS)
 constexpr uint32_t CST_META = 7;         ///< Metadata
 
 // Section flags
 constexpr uint32_t CSF_WRITE = 0x1;      ///< Writable section
 constexpr uint32_t CSF_ALLOC = 0x2;      ///< Occupies memory during execution
 constexpr uint32_t CSF_PROC = 0x4;       ///< Processable by COIL
 constexpr uint32_t CSF_STRINGS = 0x8;    ///< Contains null-terminated strings
 constexpr uint32_t CSF_CONST = 0x10;     ///< Contains const data
 
 // Symbol binding
 constexpr uint8_t CSB_LOCAL = 0;         ///< Local symbol
 constexpr uint8_t CSB_GLOBAL = 1;        ///< Global symbol
 constexpr uint8_t CSB_WEAK = 2;          ///< Weak symbol
 constexpr uint8_t CSB_EXTERN = 3;        ///< External symbol
 
 // Symbol types
 constexpr uint8_t CST_NOTYPE = 0;        ///< Symbol type is unspecified
 constexpr uint8_t CST_OBJECT = 1;        ///< Symbol is a data object
 constexpr uint8_t CST_FUNC = 2;          ///< Symbol is a code object
 constexpr uint8_t CST_SECTION = 3;       ///< Symbol associated with a section
 constexpr uint8_t CST_FILE = 4;          ///< Symbol's name is file name
 
 // Relocation types
 constexpr uint16_t CR_NONE = 0;          ///< No relocation
 constexpr uint16_t CR_DIRECT32 = 1;      ///< Direct 32-bit
 constexpr uint16_t CR_DIRECT64 = 2;      ///< Direct 64-bit
 constexpr uint16_t CR_REL32 = 3;         ///< Relative 32-bit
 constexpr uint16_t CR_REL64 = 4;         ///< Relative 64-bit
 
 // Utility functions to get string representations
 const char* getFileTypeName(uint16_t type);
 const char* getSectionTypeName(uint32_t type);
 const char* getSectionFlagsString(uint32_t flags, char* buffer, size_t size);
 const char* getSymbolBindingName(uint8_t binding);
 const char* getSymbolTypeName(uint8_t type);
 const char* getRelocationTypeName(uint16_t type);
 
 } // namespace obj
 
 /**
  * @brief Represents a COIL file header
  */
 struct CoilHeader {
   uint8_t ident[obj::CI_NIDENT];  ///< Identification bytes
   uint16_t type;                  ///< Object file type
   uint16_t version;               ///< Version
   uint32_t entry;                 ///< Entry point offset
   uint32_t shoff;                 ///< Section header offset
   uint16_t shnum;                 ///< Number of section headers
   uint16_t shstrndx;              ///< Section name string table index
   
   /**
    * @brief Initialize a header with default values
    * 
    * @param fileType Object file type
    * @return CoilHeader Initialized header
    */
   static CoilHeader initialize(uint16_t fileType);
 };
 
 /**
  * @brief Represents a COIL section header
  */
 struct CoilSectionHeader {
   uint32_t name;       ///< Section name (string table index)
   uint32_t type;       ///< Section type
   uint32_t flags;      ///< Section flags
   uint32_t offset;     ///< Section file offset
   uint32_t size;       ///< Section size in bytes
   uint16_t link;       ///< Link to another section
   uint16_t info;       ///< Additional section information
   uint16_t align;      ///< Section alignment
   uint16_t entsize;    ///< Entry size if section holds table
 };
 
 /**
  * @brief Represents a COIL symbol table entry
  */
 struct CoilSymbol {
   uint32_t name;       ///< Symbol name (string table index)
   uint32_t value;      ///< Symbol value (offset in section)
   uint32_t size;       ///< Symbol size
   uint8_t info;        ///< Symbol type and binding
   uint8_t other;       ///< Reserved
   uint16_t shndx;      ///< Section index
   
   /**
    * @brief Get the binding type
    * @return Binding type (obj::CSB_*)
    */
   uint8_t getBinding() const { return info >> 4; }
   
   /**
    * @brief Get the symbol type
    * @return Symbol type (obj::CST_*)
    */
   uint8_t getType() const { return info & 0xF; }
   
   /**
    * @brief Set the binding type
    * @param binding Binding type (obj::CSB_*)
    */
   void setBinding(uint8_t binding) { info = (binding << 4) | getType(); }
   
   /**
    * @brief Set the symbol type
    * @param type Symbol type (obj::CST_*)
    */
   void setType(uint8_t type) { info = (getBinding() << 4) | type; }
 };
 
 /**
  * @brief Represents a COIL relocation entry
  */
 struct CoilRelocation {
   uint32_t offset;     ///< Location to apply relocation
   uint32_t info;       ///< Symbol index and relocation type
   
   /**
    * @brief Get the symbol index
    * @return Symbol index
    */
   uint16_t getSymbol() const { return info >> 16; }
   
   /**
    * @brief Get the relocation type
    * @return Relocation type (obj::CR_*)
    */
   uint16_t getType() const { return info & 0xFFFF; }
   
   /**
    * @brief Set the symbol index
    * @param symbol Symbol index
    */
   void setSymbol(uint16_t symbol) { info = (static_cast<uint32_t>(symbol) << 16) | getType(); }
   
   /**
    * @brief Set the relocation type
    * @param type Relocation type (obj::CR_*)
    */
   void setType(uint16_t type) { info = (static_cast<uint32_t>(getSymbol()) << 16) | type; }
 };
 
 /**
  * @brief Section data with fixed-size name
  */
 struct SectionData {
   char name[32];               ///< Section name (fixed size)
   CoilSectionHeader header;    ///< Section header
   uint8_t* data;               ///< Section data (owned if ownsData is true)
   bool ownsData;               ///< Whether this section owns its data
   
   /**
    * @brief Create a new section
    * 
    * @param sectionName Section name
    * @param type Section type
    * @param flags Section flags
    * @param sectionData Section data (can be NULL)
    * @param size Section size
    * @param entrySize Entry size for table sections
    * @return SectionData Initialized section
    */
   static SectionData create(const char* sectionName, 
                            uint32_t type, 
                            uint32_t flags,
                            const uint8_t* sectionData, 
                            uint32_t size, 
                            uint16_t entrySize = 0);
   
   /**
    * @brief Free section data if owned
    */
   void freeData();
   
   /**
    * @brief Get a string from a string table section
    * 
    * @param offset Offset in the string table
    * @return const char* String at offset or NULL if invalid
    */
   const char* getString(uint32_t offset) const;
   
   /**
    * @brief Get a symbol from a symbol table section
    * 
    * @param index Symbol index
    * @return CoilSymbol Symbol at index
    */
   CoilSymbol getSymbol(uint32_t index) const;
   
   /**
    * @brief Set a symbol in a symbol table section
    * 
    * @param index Symbol index
    * @param symbol Symbol to set
    */
   void setSymbol(uint32_t index, const CoilSymbol& symbol);
   
   /**
    * @brief Get a relocation from a relocation section
    * 
    * @param index Relocation index
    * @return CoilRelocation Relocation at index
    */
   CoilRelocation getRelocation(uint32_t index) const;
   
   /**
    * @brief Set a relocation in a relocation section
    * 
    * @param index Relocation index
    * @param rel Relocation to set
    */
   void setRelocation(uint32_t index, const CoilRelocation& rel);
   
   /**
    * @brief Get the number of entries in a table section
    * 
    * @return uint32_t Number of entries
    */
   uint32_t getEntryCount() const;
 };
 
 /**
  * @brief Maximum number of sections in a COIL object
  */
 constexpr size_t MAX_SECTIONS = 16;
 
 /**
  * @brief COIL object file
  */
 struct CoilObject {
   CoilHeader header;
   SectionData sections[MAX_SECTIONS];
   size_t sectionCount;
   const Context* ctx;
   
   /**
    * @brief Load a COIL object from a stream
    * 
    * @param stream Input stream
    * @param context Library context
    * @return CoilObject Loaded object or empty object on error
    */
   static CoilObject load(Stream* stream, const Context* context);
   
   /**
    * @brief Create a new COIL object
    * 
    * @param type Object file type
    * @param context Library context
    * @return CoilObject Created object
    */
   static CoilObject create(uint16_t type, const Context* context);
   
   /**
    * @brief Check if a file is a valid COIL object
    * 
    * @param stream Input stream
    * @return bool True if valid COIL file
    */
   static bool isCoilFile(Stream* stream);
   
   /**
    * @brief Get a section by index
    * 
    * @param index Section index
    * @return const SectionData* Section or NULL if invalid
    */
   const SectionData* getSection(uint16_t index) const;
   
   /**
    * @brief Get a section by name
    * 
    * @param name Section name
    * @return const SectionData* Section or NULL if not found
    */
   const SectionData* getSectionByName(const char* name) const;
   
   /**
    * @brief Add a section to the object
    * 
    * @param name Section name
    * @param type Section type
    * @param flags Section flags
    * @param data Section data
    * @param size Data size
    * @param entsize Entry size for table sections
    * @return SectionData* Added section or NULL on error
    */
   SectionData* addSection(const char* name, 
                          uint32_t type, 
                          uint32_t flags,
                          const uint8_t* data, 
                          uint32_t size, 
                          uint16_t entsize = 0);
   
   /**
    * @brief Save the object to a stream
    * 
    * @param stream Output stream
    * @return bool Success
    */
   bool save(Stream* stream);
   
   /**
    * @brief Find a symbol by name
    * 
    * @param name Symbol name
    * @return std::pair<const SectionData*, CoilSymbol> Section/symbol or NULL if not found
    */
   std::pair<const SectionData*, CoilSymbol> findSymbol(const char* name) const;
   
   /**
    * @brief Add a symbol to the object
    * 
    * @param name Symbol name
    * @param value Symbol value (offset in section)
    * @param size Symbol size
    * @param type Symbol type
    * @param binding Symbol binding
    * @param sectionIndex Section index
    * @return bool Success
    */
   bool addSymbol(const char* name, 
                 uint32_t value, 
                 uint32_t size,
                 uint8_t type, 
                 uint8_t binding, 
                 uint16_t sectionIndex);
   
   /**
    * @brief Clean up object resources
    */
   void cleanup();
 };
 
 /**
  * @brief String table helper
  */
 struct StringTable {
   // Maximum size of a string table
   static constexpr size_t MAX_SIZE = 4096;
   
   // String table data
   uint8_t data[MAX_SIZE];
   size_t size;
   
   /**
    * @brief Create a string table from a section
    * 
    * @param section String table section
    * @return StringTable Initialized string table
    */
   static StringTable fromSection(const SectionData& section);
   
   /**
    * @brief Create a new empty string table
    * 
    * @return StringTable New string table
    */
   static StringTable create();
   
   /**
    * @brief Get a string from the table
    * 
    * @param offset Offset in the table
    * @return const char* String at offset or NULL if invalid
    */
   const char* getString(uint32_t offset) const;
   
   /**
    * @brief Add a string to the table
    * 
    * @param str String to add
    * @return uint32_t Offset of the string or 0 if error
    */
   uint32_t addString(const char* str);
 };
 
 } // namespace coil