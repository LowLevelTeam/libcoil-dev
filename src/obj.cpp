#include "coil/obj.hpp"
#include <cstring>
#include <algorithm>

namespace coil {

// Object format utility functions
namespace obj {

const char* getFileTypeName(uint16_t type) {
  switch (type) {
    case CT_NONE: return "None";
    case CT_REL: return "Relocatable";
    case CT_EXEC: return "Executable";
    case CT_SHARED: return "Shared";
    case CT_LIB: return "Library";
    default: return "Unknown";
  }
}

const char* getSectionTypeName(uint32_t type) {
  switch (type) {
    case CST_NULL: return "Null";
    case CST_CODE: return "Code";
    case CST_DATA: return "Data";
    case CST_SYMTAB: return "SymTab";
    case CST_STRTAB: return "StrTab";
    case CST_REL: return "Rel";
    case CST_NOBITS: return "NoBits";
    case CST_META: return "Meta";
    default: return "Unknown";
  }
}

const char* getSectionFlagsString(uint32_t flags, char* buffer, size_t size) {
  if (!buffer || size == 0) {
    return "";
  }
  
  buffer[0] = '\0';
  size_t pos = 0;
  
  struct FlagMapping {
    uint32_t flag;
    char symbol;
  };
  
  static const FlagMapping mappings[] = {
    { CSF_WRITE, 'W' },
    { CSF_ALLOC, 'A' },
    { CSF_PROC, 'P' },
    { CSF_STRINGS, 'S' },
    { CSF_CONST, 'C' }
  };
  
  for (size_t i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
    if (flags & mappings[i].flag) {
      if (pos < size - 1) {
        buffer[pos++] = mappings[i].symbol;
        buffer[pos] = '\0';
      }
    }
  }
  
  return buffer;
}

const char* getSymbolBindingName(uint8_t binding) {
  switch (binding) {
    case CSB_LOCAL: return "Local";
    case CSB_GLOBAL: return "Global";
    case CSB_WEAK: return "Weak";
    case CSB_EXTERN: return "Extern";
    default: return "Unknown";
  }
}

const char* getSymbolTypeName(uint8_t type) {
  switch (type) {
    case CST_NOTYPE: return "None";
    case CST_OBJECT: return "Object";
    case CST_FUNC: return "Function";
    case CST_SECTION: return "Section";
    case CST_FILE: return "File";
    default: return "Unknown";
  }
}

const char* getRelocationTypeName(uint16_t type) {
  switch (type) {
    case CR_NONE: return "None";
    case CR_DIRECT32: return "Direct32";
    case CR_DIRECT64: return "Direct64";
    case CR_REL32: return "Rel32";
    case CR_REL64: return "Rel64";
    default: return "Unknown";
  }
}

} // namespace obj

// CoilHeader implementation
CoilHeader CoilHeader::initialize(uint16_t fileType) {
  CoilHeader header = {};
  
  // Set magic number
  header.ident[0] = obj::COILMAG0;
  header.ident[1] = obj::COILMAG1;
  header.ident[2] = obj::COILMAG2;
  header.ident[3] = obj::COILMAG3;
  header.ident[4] = obj::COILMAG4;
  // Reserve remaining ident bytes for future use
  
  header.type = fileType;
  header.version = obj::COIL_VERSION;
  header.entry = 0;
  header.shoff = 0; // Will be set when saved
  header.shnum = 0;
  header.shstrndx = 0;
  
  return header;
}

// SectionData implementation
SectionData SectionData::create(
    const char* sectionName,
    uint32_t type,
    uint32_t flags,
    const uint8_t* sectionData,
    uint32_t size,
    uint16_t entrySize) {
  
  SectionData section = {};
  
  // Set name with safe string copy
  if (sectionName) {
    strncpy(section.name, sectionName, sizeof(section.name) - 1);
    section.name[sizeof(section.name) - 1] = '\0';
  }
  
  // Initialize header
  section.header.type = type;
  section.header.flags = flags;
  section.header.offset = 0; // Will be set when saved
  section.header.size = size;
  section.header.link = 0;
  section.header.info = 0;
  section.header.align = 1; // Default alignment
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
  if (header.type != obj::CST_STRTAB || !data || offset >= header.size) {
    return nullptr;
  }
  
  return reinterpret_cast<const char*>(data + offset);
}

CoilSymbol SectionData::getSymbol(uint32_t index) const {
  CoilSymbol symbol = {};
  
  if (header.type != obj::CST_SYMTAB || !data || 
      header.entsize == 0 || index * header.entsize >= header.size) {
    return symbol;
  }
  
  memcpy(&symbol, data + (index * header.entsize), sizeof(CoilSymbol));
  return symbol;
}

void SectionData::setSymbol(uint32_t index, const CoilSymbol& symbol) {
  if (header.type != obj::CST_SYMTAB || !data || 
      header.entsize == 0 || index * header.entsize >= header.size) {
    return;
  }
  
  memcpy(data + (index * header.entsize), &symbol, sizeof(CoilSymbol));
}

CoilRelocation SectionData::getRelocation(uint32_t index) const {
  CoilRelocation rel = {};
  
  if (header.type != obj::CST_REL || !data || 
      header.entsize == 0 || index * header.entsize >= header.size) {
    return rel;
  }
  
  memcpy(&rel, data + (index * header.entsize), sizeof(CoilRelocation));
  return rel;
}

void SectionData::setRelocation(uint32_t index, const CoilRelocation& rel) {
  if (header.type != obj::CST_REL || !data || 
      header.entsize == 0 || index * header.entsize >= header.size) {
    return;
  }
  
  memcpy(data + (index * header.entsize), &rel, sizeof(CoilRelocation));
}

uint32_t SectionData::getEntryCount() const {
  if (header.entsize == 0) {
    return 0;
  }
  return header.size / header.entsize;
}

// StringTable implementation
StringTable StringTable::create() {
  StringTable table = {};
  
  // Initialize with a null byte for index 0 (empty string)
  table.data[0] = 0;
  table.size = 1;
  
  return table;
}

StringTable StringTable::fromSection(const SectionData& section) {
  StringTable table = {};
  
  if (section.header.type != obj::CST_STRTAB || !section.data) {
    table.data[0] = 0;
    table.size = 1;
    return table;
  }
  
  // Copy string table data (up to MAX_SIZE)
  size_t copySize = std::min((size_t)section.header.size, MAX_SIZE);
  memcpy(table.data, section.data, copySize);
  table.size = copySize;
  
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

// CoilObject implementation
CoilObject CoilObject::create(uint16_t type, const Context* context) {
  CoilObject obj = {};
  obj.header = CoilHeader::initialize(type);
  obj.ctx = context;
  
  // Create a default section string table as section 0
  SectionData strTable = SectionData::create(
    ".shstrtab", obj::CST_STRTAB, obj::CSF_STRINGS, nullptr, 0, 0);
  
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
  
  // Save current position
  StreamPosition pos = stream->getReadPosition();
  
  // Reset to beginning of stream
  stream->resetReadPosition();
  
  // Read magic bytes
  uint8_t magic[5];
  size_t bytesRead = stream->read(magic, sizeof(magic));
  bool result = false;
  
  if (bytesRead == sizeof(magic)) {
    result = (magic[0] == obj::COILMAG0 && 
             magic[1] == obj::COILMAG1 && 
             magic[2] == obj::COILMAG2 && 
             magic[3] == obj::COILMAG3 && 
             magic[4] == obj::COILMAG4);
  }
  
  return result;
}

CoilObject CoilObject::load(Stream* stream, const Context* context) {
  CoilObject obj = {};
  
  if (!stream || !context) {
    return obj;
  }
  
  obj.ctx = context;
  
  // Reset to beginning of stream
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
      SectionData section = {};
      
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
          
          // Skip the string table itself
          if (i == obj.header.shstrndx) {
            strncpy(section->name, ".shstrtab", sizeof(section->name) - 1);
            section->name[sizeof(section->name) - 1] = '\0';
            continue;
          }
          
          // Skip empty name indices
          if (section->header.name == 0) {
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
    if (section->header.align > 1) {
      currentOffset = (currentOffset + section->header.align - 1) 
                    & ~((uint32_t)section->header.align - 1);
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
    
    if (currentPos < targetOffset) {
      // Write padding bytes until reaching the proper offset
      size_t paddingSize = targetOffset - currentPos;
      uint8_t padding[64] = {0};
      
      while (paddingSize > 0) {
        size_t writeSize = std::min(paddingSize, sizeof(padding));
        size_t bytesWritten = stream->write(padding, writeSize);
        
        if (bytesWritten != writeSize) {
          if (ctx && ctx->errorManager) {
            ctx->errorManager->addError(
              ErrorCode::IO,
              stream->getWritePosition(),
              "Failed to write section padding"
            );
          }
          return false;
        }
        
        paddingSize -= bytesWritten;
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
  
  return true;
}

std::pair<const SectionData*, CoilSymbol> CoilObject::findSymbol(const char* name) const {
  if (!name) {
    CoilSymbol empty = {};
    return {nullptr, empty};
  }
  
  // Find the symbol table and string table sections
  const SectionData* symTab = nullptr;
  const SectionData* strTab = nullptr;
  
  // Look for symbol table
  for (size_t i = 0; i < sectionCount; i++) {
    if (sections[i].header.type == obj::CST_SYMTAB) {
      symTab = &sections[i];
      
      // Get string table via link field
      if (symTab->header.link < sectionCount) {
        strTab = &sections[symTab->header.link];
        if (strTab->header.type != obj::CST_STRTAB) {
          strTab = nullptr;
        }
      }
      
      break;
    }
  }
  
  // If no symbol table was found, or no string table is linked
  if (!symTab || !strTab || !strTab->data) {
    CoilSymbol empty = {};
    return {nullptr, empty};
  }
  
  // Search the symbol table
  uint32_t symbolCount = symTab->getEntryCount();
  for (uint32_t i = 0; i < symbolCount; i++) {
    CoilSymbol symbol = symTab->getSymbol(i);
    
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
      return {symTab, symbol};
    }
  }
  
  CoilSymbol empty = {};
  return {nullptr, empty};
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
  
  for (size_t i = 0; i < sectionCount; i++) {
    if (sections[i].header.type == obj::CST_SYMTAB) {
      symTab = &sections[i];
      break;
    }
  }
  
  if (!symTab) {
    // Create a new symbol table section
    symTab = addSection(".symtab", obj::CST_SYMTAB, 0, nullptr, 0, sizeof(CoilSymbol));
    if (!symTab) {
      return false;
    }
  }
  
  // Find or create the string table for symbols
  SectionData* strTab = nullptr;
  
  // Check if there's already a linked string table
  if (symTab->header.link < sectionCount) {
    strTab = &sections[symTab->header.link];
    if (strTab->header.type != obj::CST_STRTAB) {
      strTab = nullptr;
    }
  }
  
  // If no valid string table exists, create one
  if (!strTab) {
    strTab = addSection(".strtab", obj::CST_STRTAB, obj::CSF_STRINGS, nullptr, 0, 0);
    if (!strTab) {
      return false;
    }
    
    // Initialize with a null byte for index 0
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
    
    // Link the symbol table to the string table
    symTab->header.link = sectionCount - 1;
  }
  
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
  
  // Create the symbol
  CoilSymbol symbol = {};
  symbol.name = nameOffset;
  symbol.value = value;
  symbol.size = size;
  symbol.setType(type);
  symbol.setBinding(binding);
  symbol.other = 0;
  symbol.shndx = sectionIndex;
  
  // Add the symbol to the symbol table
  uint32_t symIndex = symTab->getEntryCount();
  uint32_t newTableSize = (symIndex + 1) * sizeof(CoilSymbol);
  
  // Initialize or resize the symbol table
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
          "Failed to resize symbol table"
        );
      }
      return false;
    }
    symTab->data = newTabData;
  }
  
  // Update symbol table size
  symTab->header.size = newTableSize;
  
  // Add the symbol
  memcpy(symTab->data + (symIndex * sizeof(CoilSymbol)), &symbol, sizeof(CoilSymbol));
  
  return true;
}

void CoilObject::cleanup() {
  for (size_t i = 0; i < sectionCount; i++) {
    sections[i].freeData();
  }
  sectionCount = 0;
}

} // namespace coil