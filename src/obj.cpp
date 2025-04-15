#include "coil/obj.hpp"
#include <cstring>
#include <algorithm>

namespace coil {

// Object format helper functions
namespace obj {

const char* getFileTypeName(uint16_t type) {
    switch (type) {
        case CT_NONE: return "None";
        case CT_REL:  return "Relocatable";
        case CT_EXEC: return "Executable";
        case CT_DYN:  return "Shared Object";
        case CT_LIB:  return "Library";
        default:      return "Unknown";
    }
}

const char* getMachineTypeName(uint16_t machine) {
    // Machine types can be expanded as needed
    switch (machine) {
        case 0:  return "None";
        case 1:  return "X86";
        case 2:  return "X86_64";
        case 3:  return "ARM";
        case 4:  return "ARM64";
        case 5:  return "RISCV";
        case 6:  return "WASM";
        default: return "Unknown";
    }
}

const char* getSectionTypeName(uint32_t type) {
    switch (type) {
        case CST_NULL:    return "Null";
        case CST_CODE:    return "Code";
        case CST_DATA:    return "Data";
        case CST_SYMTAB:  return "Symbol Table";
        case CST_STRTAB:  return "String Table";
        case CST_RELA:    return "Relocation with Addends";
        case CST_HASH:    return "Hash Table";
        case CST_DYNAMIC: return "Dynamic Linking Info";
        case CST_NOTE:    return "Notes";
        case CST_NOBITS:  return "No Bits (BSS)";
        case CST_REL:     return "Relocation";
        case CST_DYNSYM:  return "Dynamic Symbols";
        case CST_TYPE:    return "Type Definitions";
        case CST_META:    return "Metadata";
        case CST_DEBUG:   return "Debug Info";
        default:          return "Unknown";
    }
}

void getSectionFlagsString(uint32_t flags, char* buffer, size_t size) {
    if (!buffer || size == 0) return;
    
    buffer[0] = '\0';
    size_t pos = 0;
    
    struct FlagMapping {
        uint32_t flag;
        char symbol;
    };
    
    static const FlagMapping mappings[] = {
        { CSF_WRITE, 'W' },
        { CSF_ALLOC, 'A' },
        { CSF_EXEC,  'X' },
        { CSF_MERGE, 'M' },
        { CSF_STRINGS, 'S' },
        { CSF_CONST, 'C' },
        { CSF_COMPRESSED, 'Z' }
    };
    
    for (size_t i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
        if (flags & mappings[i].flag) {
            if (pos < size - 1) {
                buffer[pos++] = mappings[i].symbol;
                buffer[pos] = '\0';
            }
        }
    }
}

const char* getSymbolBindingName(uint8_t binding) {
    switch (binding) {
        case CSB_LOCAL:  return "Local";
        case CSB_GLOBAL: return "Global";
        case CSB_WEAK:   return "Weak";
        case CSB_EXTERN: return "External";
        default:         return "Unknown";
    }
}

const char* getSymbolTypeName(uint8_t type) {
    switch (type) {
        case CST_NOTYPE:    return "None";
        case CST_OBJECT:    return "Object";
        case CST_FUNC:      return "Function";
        case CST_SECTION:   return "Section";
        case CST_FILE:      return "File";
        case CST_COMMON:    return "Common";
        case CST_TYPE_DEF:  return "Type Definition";
        case CST_OPERATOR:  return "Operator";
        default:            return "Unknown";
    }
}

const char* getRelocationTypeName(uint16_t machine, uint32_t type) {
    // Basic relocation types that are common across architectures
    switch (type) {
        case CR_NONE:      return "None";
        case CR_DIRECT32:  return "Direct 32-bit";
        case CR_DIRECT64:  return "Direct 64-bit";
        case CR_PC32:      return "PC-relative 32-bit";
        case CR_PC64:      return "PC-relative 64-bit";
        case CR_GOT32:     return "GOT 32-bit";
        case CR_PLT32:     return "PLT 32-bit";
        case CR_COPY:      return "Copy";
        case CR_GLOB_DATA: return "Global Data";
        case CR_JMP_SLOT:  return "Jump Slot";
        default:           return "Unknown";
    }
    
    // Architecture-specific relocations could be added based on machine type
    (void)machine; // Avoid unused parameter warning
}

} // namespace obj

// SectionData implementation
SectionData SectionData::create(
    const char* sectionName, 
    uint32_t type, 
    uint32_t flags,
    const uint8_t* sectionData, 
    uint32_t size, 
    uint16_t entrySize) {
    
    SectionData section;
    
    // Set the name with safe string copy
    if (sectionName) {
        strncpy(section.name, sectionName, sizeof(section.name) - 1);
        section.name[sizeof(section.name) - 1] = '\0';
    } else {
        section.name[0] = '\0';
    }
    
    // Initialize header
    section.header.name = 0; // Will be set when saved
    section.header.type = type;
    section.header.flags = flags;
    section.header.offset = 0; // Will be set when saved
    section.header.size = size;
    section.header.link = 0;
    section.header.info = 0;
    section.header.addralign = 1; // Default alignment
    section.header.entsize = entrySize;
    
    // Handle data
    if (size > 0 && sectionData) {
        section.data = (uint8_t*)malloc(size);
        if (section.data) {
            memcpy(section.data, sectionData, size);
            section.ownsData = true;
        }
    } else {
        section.data = nullptr;
        section.ownsData = false;
    }
    
    return section;
}

void SectionData::freeData() {
    if (ownsData && data) {
        free(data);
        data = nullptr;
        ownsData = false;
    }
}

const char* SectionData::getString(uint32_t offset) const {
    // Ensure this is a string table section and offset is valid
    if (header.type != obj::CST_STRTAB || !data || offset >= header.size) {
        return nullptr;
    }
    
    // Return the string at the offset - we trust it's null-terminated
    // as that's a requirement for string tables
    return reinterpret_cast<const char*>(data + offset);
}

CoilSymbolEntry SectionData::getSymbol(uint32_t index) const {
    CoilSymbolEntry symbol;
    memset(&symbol, 0, sizeof(symbol));
    
    // Ensure this is a symbol table section and index is valid
    if ((header.type != obj::CST_SYMTAB && header.type != obj::CST_DYNSYM) || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return symbol;
    }
    
    // Copy the symbol data
    memcpy(&symbol, data + (index * header.entsize), sizeof(symbol));
    return symbol;
}

void SectionData::setSymbol(uint32_t index, const CoilSymbolEntry& symbol) {
    // Ensure this is a symbol table section and index is valid
    if ((header.type != obj::CST_SYMTAB && header.type != obj::CST_DYNSYM) || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return;
    }
    
    // Copy the symbol data
    memcpy(data + (index * header.entsize), &symbol, sizeof(symbol));
}

CoilRelEntry SectionData::getRel(uint32_t index) const {
    CoilRelEntry rel;
    memset(&rel, 0, sizeof(rel));
    
    // Ensure this is a relocation section and index is valid
    if (header.type != obj::CST_REL || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return rel;
    }
    
    // Copy the relocation data
    memcpy(&rel, data + (index * header.entsize), sizeof(rel));
    return rel;
}

CoilRelaEntry SectionData::getRela(uint32_t index) const {
    CoilRelaEntry rela;
    memset(&rela, 0, sizeof(rela));
    
    // Ensure this is a relocation section and index is valid
    if (header.type != obj::CST_RELA || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return rela;
    }
    
    // Copy the relocation data
    memcpy(&rela, data + (index * header.entsize), sizeof(rela));
    return rela;
}

void SectionData::setRel(uint32_t index, const CoilRelEntry& rel) {
    // Ensure this is a relocation section and index is valid
    if (header.type != obj::CST_REL || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return;
    }
    
    // Copy the relocation data
    memcpy(data + (index * header.entsize), &rel, sizeof(rel));
}

void SectionData::setRela(uint32_t index, const CoilRelaEntry& rela) {
    // Ensure this is a relocation section and index is valid
    if (header.type != obj::CST_RELA || 
        !data || header.entsize == 0 || 
        index * header.entsize >= header.size) {
        return;
    }
    
    // Copy the relocation data
    memcpy(data + (index * header.entsize), &rela, sizeof(rela));
}

uint32_t SectionData::getEntryCount() const {
    if (header.entsize == 0) {
        return 0;
    }
    return header.size / header.entsize;
}

// CoilHeader implementation
CoilHeader CoilHeader::initialize(uint16_t fileType, uint16_t machine) {
    CoilHeader header;
    
    // Set magic number
    header.ident[0] = obj::COILMAG0;
    header.ident[1] = obj::COILMAG1;
    header.ident[2] = obj::COILMAG2;
    header.ident[3] = obj::COILMAG3;
    header.ident[4] = obj::COILMAG4;
    
    // Set endianness based on host system
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        header.ident[5] = obj::COILDATA2LSB;
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        header.ident[5] = obj::COILDATA2MSB;
    #else
        // Default to little-endian if we can't determine at compile time
        #if defined(_WIN32) || defined(__x86_64__) || defined(__i386__)
            header.ident[5] = obj::COILDATA2LSB;
        #else
            // Runtime check
            union { uint16_t i; char c[2]; } endian = { 0x0102 };
            header.ident[5] = (endian.c[0] == 0x02) ? obj::COILDATA2LSB : obj::COILDATA2MSB;
        #endif
    #endif
    
    // Set version
    header.ident[6] = obj::COIL_VERSION;
    
    // Zero remaining ident bytes
    for (size_t i = 7; i < obj::CI_NIDENT; i++) {
        header.ident[i] = 0;
    }
    
    // Set header fields
    header.type = fileType;
    header.version = obj::COIL_VERSION;
    header.reserved1 = 0;
    header.entry = 0;
    header.shoff = 0; // Will be set when saved
    header.flags = machine;  // Store machine type in flags
    header.ehsize = sizeof(CoilHeader);
    header.shentsize = sizeof(CoilSectionHeader);
    header.shnum = 0;
    header.shstrndx = 0;
    
    return header;
}

// CoilObject implementation
CoilObject CoilObject::load(Stream* stream, const Context* context) {
    CoilObject obj;
    
    if (!stream || !context) {
        return obj;
    }
    
    obj.ctx = context;
    
    // Save the current stream position
    stream->resetReadPosition();
    
    // Read the header
    if (stream->read(&obj.header, sizeof(CoilHeader)) != sizeof(CoilHeader)) {
        if (context->errorManager) {
            context->errorManager->addError(
                ErrorCode::IO,
                stream->getReadPosition(),
                "Failed to read COIL header"
            );
        }
        return obj;
    }
    
    // Verify the magic number
    if (obj.header.ident[0] != obj::COILMAG0 ||
        obj.header.ident[1] != obj::COILMAG1 ||
        obj.header.ident[2] != obj::COILMAG2 ||
        obj.header.ident[3] != obj::COILMAG3 ||
        obj.header.ident[4] != obj::COILMAG4) {
        
        if (context->errorManager) {
            context->errorManager->addError(
                ErrorCode::Format,
                stream->getReadPosition(),
                "Invalid COIL file: wrong magic number"
            );
        }
        return obj;
    }
    
    // Verify the version
    if (obj.header.version > obj::COIL_VERSION) {
        if (context->errorManager) {
            context->errorManager->addWarning(
                ErrorCode::Format,
                stream->getReadPosition(),
                "COIL file version is newer than supported"
            );
        }
    }
    
    // Read section headers
    if (obj.header.shnum > 0) {
        if (obj.header.shnum > MAX_SECTIONS) {
            if (context->errorManager) {
                context->errorManager->addError(
                    ErrorCode::Format,
                    stream->getReadPosition(),
                    "Too many sections in COIL file"
                );
            }
            return obj;
        }
        
        // Seek to section header table
        StreamPosition currentPos = stream->getReadPosition();
        stream->resetReadPosition();
        
        // Skip to section header offset
        uint8_t skipBuf[1024];
        size_t toSkip = obj.header.shoff;
        while (toSkip > 0) {
            size_t chunk = std::min(toSkip, sizeof(skipBuf));
            size_t read = stream->read(skipBuf, chunk);
            if (read == 0) break;
            toSkip -= read;
        }
        
        // Read section headers
        for (uint16_t i = 0; i < obj.header.shnum; i++) {
            SectionData section;
            memset(&section, 0, sizeof(section));
            
            // Read section header
            if (stream->read(&section.header, sizeof(CoilSectionHeader)) != sizeof(CoilSectionHeader)) {
                if (context->errorManager) {
                    context->errorManager->addError(
                        ErrorCode::IO,
                        stream->getReadPosition(),
                        "Failed to read section header"
                    );
                }
                obj.cleanup();
                return obj;
            }
            
            // Save section in object
            obj.sections[obj.sectionCount++] = section;
        }
        
        // Read section names from string table if available
        if (obj.header.shstrndx < obj.sectionCount) {
            SectionData* strTab = &obj.sections[obj.header.shstrndx];
            
            // Read the string table data
            if (strTab->header.size > 0) {
                strTab->data = (uint8_t*)malloc(strTab->header.size);
                if (!strTab->data) {
                    if (context->errorManager) {
                        context->errorManager->addError(
                            ErrorCode::Memory,
                            stream->getReadPosition(),
                            "Failed to allocate memory for section name string table"
                        );
                    }
                    obj.cleanup();
                    return obj;
                }
                
                strTab->ownsData = true;
                
                // Seek to string table data
                stream->resetReadPosition();
                toSkip = strTab->header.offset;
                while (toSkip > 0) {
                    size_t chunk = std::min(toSkip, sizeof(skipBuf));
                    size_t read = stream->read(skipBuf, chunk);
                    if (read == 0) break;
                    toSkip -= read;
                }
                
                // Read string table data
                if (stream->read(strTab->data, strTab->header.size) != strTab->header.size) {
                    if (context->errorManager) {
                        context->errorManager->addError(
                            ErrorCode::IO,
                            stream->getReadPosition(),
                            "Failed to read section name string table data"
                        );
                    }
                    obj.cleanup();
                    return obj;
                }
                
                // Get section names
                for (size_t i = 0; i < obj.sectionCount; i++) {
                    SectionData* section = &obj.sections[i];
                    
                    // Skip empty name indices or string table itself
                    if (section->header.name == 0 || i == obj.header.shstrndx) {
                        continue;
                    }
                    
                    // Get name from string table
                    const char* name = reinterpret_cast<const char*>(
                        strTab->data + section->header.name);
                    
                    // Validate string bounds
                    if (section->header.name < strTab->header.size) {
                        size_t maxLen = strTab->header.size - section->header.name;
                        size_t len = 0;
                        while (len < maxLen && name[len] != '\0') {
                            len++;
                        }
                        
                        if (len < maxLen) {
                            // Valid name found
                            strncpy(section->name, name, sizeof(section->name) - 1);
                            section->name[sizeof(section->name) - 1] = '\0';
                        }
                    }
                }
            }
        }
        
        // Read section data
        for (size_t i = 0; i < obj.sectionCount; i++) {
            SectionData* section = &obj.sections[i];
            
            // Skip sections without data or already loaded string table
            if (section->header.size == 0 || section->data != nullptr || 
                section->header.type == obj::CST_NOBITS) {
                continue;
            }
            
            // Allocate memory for section data
            section->data = (uint8_t*)malloc(section->header.size);
            if (!section->data) {
                if (context->errorManager) {
                    context->errorManager->addError(
                        ErrorCode::Memory,
                        stream->getReadPosition(),
                        "Failed to allocate memory for section data"
                    );
                }
                obj.cleanup();
                return obj;
            }
            
            section->ownsData = true;
            
            // Seek to section data
            stream->resetReadPosition();
            toSkip = section->header.offset;
            while (toSkip > 0) {
                size_t chunk = std::min(toSkip, sizeof(skipBuf));
                size_t read = stream->read(skipBuf, chunk);
                if (read == 0) break;
                toSkip -= read;
            }
            
            // Read section data
            if (stream->read(section->data, section->header.size) != section->header.size) {
                if (context->errorManager) {
                    context->errorManager->addError(
                        ErrorCode::IO,
                        stream->getReadPosition(),
                        "Failed to read section data"
                    );
                }
                obj.cleanup();
                return obj;
            }
        }
    }
    
    return obj;
}

CoilObject CoilObject::create(uint16_t type, uint16_t machine, const Context* context) {
    CoilObject obj;
    obj.header = CoilHeader::initialize(type, machine);
    obj.sectionCount = 0;
    obj.ctx = context;
    
    // Create a default section string table as section 0
    SectionData strTable = SectionData::create(".shstrtab", obj::CST_STRTAB, obj::CSF_STRINGS, nullptr, 0, 0);
    
    // Initialize with a null byte for index 0 (empty string)
    uint8_t nullByte = 0;
    strTable.data = (uint8_t*)malloc(1);
    if (strTable.data) {
        strTable.data[0] = nullByte;
        strTable.header.size = 1;
        strTable.ownsData = true;
        
        // Add to object
        obj.sections[obj.sectionCount++] = strTable;
        obj.header.shstrndx = 0;
        obj.header.shnum = 1;
    } else if (context && context->errorManager) {
        context->errorManager->addError(
            ErrorCode::Memory,
            StreamPosition(),
            "Failed to allocate memory for section string table"
        );
    }
    
    return obj;
}

bool CoilObject::isCoilFile(Stream* stream) {
    if (!stream || !stream->isReadable()) {
        return false;
    }
    
    // Save the current stream position
    StreamPosition pos = stream->getReadPosition();
    
    // Completely reset the reading position to the start
    stream->resetReadPosition();
    
    // Read magic bytes
    uint8_t magic[5];
    bool result = false;
    
    if (stream->read(magic, sizeof(magic)) == sizeof(magic)) {
        result = (magic[0] == obj::COILMAG0 && 
                  magic[1] == obj::COILMAG1 && 
                  magic[2] == obj::COILMAG2 && 
                  magic[3] == obj::COILMAG3 && 
                  magic[4] == obj::COILMAG4);
    }
    
    // Restore the stream position
    stream->resetReadPosition();
    
    // Skip to original position
    uint8_t skipBuf[1024];
    size_t toSkip = pos.offset;
    while (toSkip > 0) {
        size_t chunk = std::min(toSkip, sizeof(skipBuf));
        size_t read = stream->read(skipBuf, chunk);
        if (read == 0) break;
        toSkip -= read;
    }
    
    return result;
}

const SectionData* CoilObject::getSection(uint16_t index) const {
    if (index >= sectionCount) {
        return nullptr;
    }
    return &sections[index];
}

const SectionData* CoilObject::getSectionByName(const char* name) const {
    if (!name) {
        return nullptr;
    }
    
    for (size_t i = 0; i < sectionCount; i++) {
        if (strcmp(sections[i].name, name) == 0) {
            return &sections[i];
        }
    }
    
    return nullptr;
}

SectionData* CoilObject::addSection(
    const char* name, 
    uint32_t type, 
    uint32_t flags, 
    const uint8_t* data, 
    uint32_t size, 
    uint16_t entsize) {
    
    if (!name || sectionCount >= MAX_SECTIONS) {
        if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
                ErrorCode::Argument,
                StreamPosition(),
                sectionCount >= MAX_SECTIONS ? 
                    "Too many sections in object file" : 
                    "Invalid section name"
            );
        }
        return nullptr;
    }
    
    // Check if a section with this name already exists
    for (size_t i = 0; i < sectionCount; i++) {
        if (strcmp(sections[i].name, name) == 0) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::Argument,
                    StreamPosition(),
                    "Section with this name already exists"
                );
            }
            return nullptr;
        }
    }
    
    // Add the section name to the string table (if we have one)
    uint32_t nameOffset = 0;
    
    if (header.shstrndx < sectionCount) {
        SectionData* strTab = &sections[header.shstrndx];
        if (strTab->header.type == obj::CST_STRTAB && strTab->data) {
            // Calculate required size
            size_t nameLen = strlen(name) + 1; // Include null terminator
            size_t newSize = strTab->header.size + nameLen;
            
            // Resize string table
            uint8_t* newData = (uint8_t*)realloc(strTab->data, newSize);
            if (!newData) {
                if (ctx && ctx->errorManager) {
                    ctx->errorManager->addError(
                        ErrorCode::Memory,
                        StreamPosition(),
                        "Failed to allocate memory for section name"
                    );
                }
                return nullptr;
            }
            
            // Update string table
            strTab->data = newData;
            nameOffset = strTab->header.size;
            
            // Add the name to the string table
            strcpy((char*)(strTab->data + nameOffset), name);
            strTab->header.size = (uint32_t)newSize;
        }
    }
    
    // Create the section
    SectionData section = SectionData::create(name, type, flags, data, size, entsize);
    
    // Set the name offset in the section header
    section.header.name = nameOffset;
    
    // Add to object
    sections[sectionCount] = section;
    sectionCount++;
    header.shnum = sectionCount;
    
    return &sections[sectionCount - 1];
}

bool CoilObject::save(Stream* stream) {
    if (!stream || !stream->isWritable()) {
        if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
                ErrorCode::Argument,
                StreamPosition(),
                "Invalid stream for saving COIL object"
            );
        }
        return false;
    }
    
    // Calculate file layout
    uint32_t currentOffset = sizeof(CoilHeader);
    
    // Section headers follow the file header
    header.shoff = currentOffset;
    currentOffset += header.shnum * sizeof(CoilSectionHeader);
    
    // Calculate section offsets
    for (uint16_t i = 0; i < header.shnum; i++) {
        SectionData* section = &sections[i];
        
        // Align data based on section alignment
        if (section->header.addralign > 1) {
            currentOffset = (currentOffset + section->header.addralign - 1) 
                          & ~((uint32_t)section->header.addralign - 1);
        }
        
        // Set section offset
        section->header.offset = currentOffset;
        
        // Advance offset for next section
        if (section->header.type != obj::CST_NOBITS) {
            currentOffset += section->header.size;
        }
    }
    
    // Reset the stream position to the beginning
    stream->resetWritePosition();
    
    // Write file header
    if (stream->write(&header, sizeof(CoilHeader)) != sizeof(CoilHeader)) {
        if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
                ErrorCode::IO,
                stream->getWritePosition(),
                "Failed to write COIL header"
            );
        }
        return false;
    }
    
    // Write section headers
    for (uint16_t i = 0; i < header.shnum; i++) {
        const SectionData* section = &sections[i];
        
        if (stream->write(&section->header, sizeof(CoilSectionHeader)) != sizeof(CoilSectionHeader)) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::IO,
                    stream->getWritePosition(),
                    "Failed to write section header"
                );
            }
            return false;
        }
    }
    
    // Write section data
    for (uint16_t i = 0; i < header.shnum; i++) {
        const SectionData* section = &sections[i];
        
        // Skip sections without data
        if (section->header.type == obj::CST_NOBITS || section->header.size == 0 || !section->data) {
            continue;
        }
        
        // Seek to section offset
        stream->resetWritePosition();
        uint32_t currentPos = stream->getWritePosition().offset;
        uint32_t targetOffset = section->header.offset;
        
        // Write padding bytes until reaching the proper offset
        if (currentPos < targetOffset) {
            size_t paddingSize = targetOffset - currentPos;
            uint8_t* padding = (uint8_t*)calloc(paddingSize, 1);
            if (!padding) {
                if (ctx && ctx->errorManager) {
                    ctx->errorManager->addError(
                        ErrorCode::Memory,
                        stream->getWritePosition(),
                        "Failed to allocate memory for section padding"
                    );
                }
                return false;
            }
            
            size_t bytesWritten = stream->write(padding, paddingSize);
            free(padding);
            
            if (bytesWritten != paddingSize) {
                if (ctx && ctx->errorManager) {
                    ctx->errorManager->addError(
                        ErrorCode::IO,
                        stream->getWritePosition(),
                        "Failed to write section padding"
                    );
                }
                return false;
            }
        }
        
        // Write section data
        if (stream->write(section->data, section->header.size) != section->header.size) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::IO,
                    stream->getWritePosition(),
                    "Failed to write section data"
                );
            }
            return false;
        }
    }
    
    // Flush the stream to ensure all data is written
    stream->close();
    
    return true;
}

std::pair<const SectionData*, const CoilSymbolEntry*> CoilObject::findSymbol(const char* name) const {
    if (!name) {
        return {nullptr, nullptr};
    }
    
    // Find the symbol table and string table sections
    const SectionData* symTab = nullptr;
    const SectionData* strTab = nullptr;
    uint16_t strTabIndex = 0;
    
    for (size_t i = 0; i < sectionCount; i++) {
        if (sections[i].header.type == obj::CST_SYMTAB) {
            symTab = &sections[i];
            strTabIndex = symTab->header.link;
            break;
        }
    }
    
    if (!symTab) {
        // Try dynamic symbol table
        for (size_t i = 0; i < sectionCount; i++) {
            if (sections[i].header.type == obj::CST_DYNSYM) {
                symTab = &sections[i];
                strTabIndex = symTab->header.link;
                break;
            }
        }
    }
    
    if (!symTab || strTabIndex >= sectionCount) {
        return {nullptr, nullptr};
    }
    
    // Get the string table
    strTab = &sections[strTabIndex];
    if (strTab->header.type != obj::CST_STRTAB || !strTab->data) {
        return {nullptr, nullptr};
    }
    
    // Search the symbol table
    uint32_t symbolCount = symTab->getEntryCount();
    for (uint32_t i = 0; i < symbolCount; i++) {
        CoilSymbolEntry symbol = symTab->getSymbol(i);
        
        // Skip symbols with empty names
        if (symbol.name == 0) {
            continue;
        }
        
        // Get the symbol name from the string table
        const char* symName = strTab->getString(symbol.name);
        if (!symName) {
            continue;
        }
        
        // Compare names
        if (strcmp(symName, name) == 0) {
            // Use static storage for returning the symbol
            static CoilSymbolEntry foundSymbol;
            foundSymbol = symbol;
            return {symTab, &foundSymbol};
        }
    }
    
    return {nullptr, nullptr};
}

bool CoilObject::addSymbol(
    const char* name, 
    uint32_t value, 
    uint32_t size, 
    uint8_t type, 
    uint8_t binding, 
    uint16_t sectionIndex) {
    
    if (!name || sectionIndex >= sectionCount) {
        if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
                ErrorCode::Argument,
                StreamPosition(),
                "Invalid arguments for adding symbol"
            );
        }
        return false;
    }
    
    // Find or create the symbol table section
    SectionData* symTab = nullptr;
    size_t symTabIndex = 0;
    
    for (size_t i = 0; i < sectionCount; i++) {
        if (sections[i].header.type == obj::CST_SYMTAB) {
            symTab = &sections[i];
            symTabIndex = i;
            break;
        }
    }
    
    if (!symTab) {
        // Create a new symbol table section
        symTab = addSection(".symtab", obj::CST_SYMTAB, 0, nullptr, 0, sizeof(CoilSymbolEntry));
        if (!symTab) {
            return false;
        }
        symTabIndex = sectionCount - 1;
    }
    
    // Find or create the string table section for symbols
    SectionData* strTab = nullptr;
    size_t strTabIndex = 0;
    
    // Initialize with an empty string table if link is not set or valid
    // Always create a new string table for symbols if none exists
    bool createStringTable = true;
    
    // First, try to use an existing string table via link
    if (symTab->header.link < sectionCount) {
        SectionData* linkSection = &sections[symTab->header.link];
        if (linkSection->header.type == obj::CST_STRTAB) {
            strTab = linkSection;
            strTabIndex = symTab->header.link;
            createStringTable = false;
        }
    }
    
    // If no valid link, check for an existing .strtab section
    if (createStringTable) {
        for (size_t i = 0; i < sectionCount; i++) {
            if (sections[i].header.type == obj::CST_STRTAB && 
                strcmp(sections[i].name, ".strtab") == 0) {
                strTab = &sections[i];
                strTabIndex = i;
                createStringTable = false;
                break;
            }
        }
    }
    
    // If no suitable string table found, create a new one
    if (createStringTable) {
        // Create a new string table section for symbols
        strTab = addSection(".strtab", obj::CST_STRTAB, obj::CSF_STRINGS, nullptr, 0, 0);
        if (!strTab) {
            return false;
        }
        strTabIndex = sectionCount - 1;
        
        // Initialize with a null byte for index 0 (empty string)
        strTab->data = (uint8_t*)malloc(1);
        if (!strTab->data) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::Memory,
                    StreamPosition(),
                    "Failed to allocate memory for symbol string table"
                );
            }
            return false;
        }
        strTab->data[0] = 0;
        strTab->header.size = 1;
        strTab->ownsData = true;
    }
    
    // Link the symbol table to the string table
    symTab->header.link = (uint16_t)strTabIndex;
    
    // Add the symbol name to the string table
    size_t nameLen = strlen(name) + 1; // Include null terminator
    size_t newSize = strTab->header.size + nameLen;
    
    // Resize string table
    uint8_t* newData = (uint8_t*)realloc(strTab->data, newSize);
    if (!newData) {
        if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
                ErrorCode::Memory,
                StreamPosition(),
                "Failed to allocate memory for symbol name"
            );
        }
        return false;
    }
    
    // Update string table
    strTab->data = newData;
    uint32_t nameOffset = strTab->header.size;
    
    // Add the name to the string table
    strcpy((char*)(strTab->data + nameOffset), name);
    strTab->header.size = (uint32_t)newSize;
    
    // Create the symbol entry
    CoilSymbolEntry symbol;
    symbol.name = nameOffset;
    symbol.value = value;
    symbol.size = size;
    symbol.setType(type);
    symbol.setBinding(binding);
    symbol.other = 0;
    symbol.shndx = sectionIndex;
    
    // Add the symbol to the symbol table
    uint32_t symIndex = symTab->getEntryCount();
    uint32_t newTableSize = (symIndex + 1) * sizeof(CoilSymbolEntry);
    
    // Initialize the symbol table data if it's null
    if (!symTab->data) {
        symTab->data = (uint8_t*)malloc(newTableSize);
        if (!symTab->data) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::Memory,
                    StreamPosition(),
                    "Failed to allocate memory for symbol table"
                );
            }
            return false;
        }
        symTab->ownsData = true;
    } else {
        // Resize symbol table
        uint8_t* newTabData = (uint8_t*)realloc(symTab->data, newTableSize);
        if (!newTabData) {
            if (ctx && ctx->errorManager) {
                ctx->errorManager->addError(
                    ErrorCode::Memory,
                    StreamPosition(),
                    "Failed to allocate memory for symbol entry"
                );
            }
            return false;
        }
        symTab->data = newTabData;
    }
    
    // Update symbol table size
    symTab->header.size = newTableSize;
    
    // Add the symbol
    memcpy(symTab->data + (symIndex * sizeof(CoilSymbolEntry)), &symbol, sizeof(CoilSymbolEntry));
    
    return true;
}

void CoilObject::cleanup() {
    for (size_t i = 0; i < sectionCount; i++) {
        sections[i].freeData();
    }
    sectionCount = 0;
}

// StringTable implementation
StringTable StringTable::fromSection(const SectionData& section) {
    StringTable table;
    memset(&table, 0, sizeof(table));
    
    if (section.header.type != obj::CST_STRTAB || !section.data) {
        return table;
    }
    
    // Copy string table data (up to MAX_SIZE)
    size_t copySize = std::min((size_t)section.header.size, MAX_SIZE);
    memcpy(table.data, section.data, copySize);
    table.size = copySize;
    
    return table;
}

StringTable StringTable::create() {
    StringTable table;
    memset(&table, 0, sizeof(table));
    
    // Initialize with a null byte for index 0 (empty string)
    table.data[0] = 0;
    table.size = 1;
    
    return table;
}

const char* StringTable::getString(uint32_t offset) const {
    if (offset >= size) {
        return nullptr;
    }
    
    return reinterpret_cast<const char*>(data + offset);
}

uint32_t StringTable::addString(const char* str) {
    if (!str) {
        return 0; // Empty string is at offset 0
    }
    
    size_t len = strlen(str) + 1; // Include null terminator
    
    // Check if string already exists in the table
    for (size_t offset = 0; offset < size; ) {
        const char* existing = reinterpret_cast<const char*>(data + offset);
        
        // Bounds check the null terminator search
        size_t maxLen = size - offset;
        size_t existingLen = 0;
        while (existingLen < maxLen && existing[existingLen] != '\0') {
            existingLen++;
        }
        
        if (existingLen < maxLen && existingLen == len - 1) {
            // Found string of same length (minus null terminator)
            if (memcmp(existing, str, len - 1) == 0) {
                return (uint32_t)offset;
            }
        }
        
        // Skip to next string
        offset += (existingLen < maxLen) ? existingLen + 1 : 1;
    }
    
    // Check if the string will fit
    if (size + len > MAX_SIZE) {
        return 0;
    }
    
    // Add the string
    uint32_t offset = (uint32_t)size;
    memcpy(data + size, str, len);
    size += len;
    
    return offset;
}

} // namespace coil