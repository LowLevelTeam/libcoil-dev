/**
 * @file obj.cpp
 * @brief Implementation of COIL optimized object format
 */

#include "coil/obj.hpp"
#include "coil/err.hpp"
#include <cstdio>
#include <string>

namespace coil {
  // -------------------------------- Stream Functionality -------------------------------- //
  // Internal Functionality
  Result __load_header(Stream &stream, ObjectHeader &header) {
    Result result = Result::Success;
    
    // Load Header
    result = stream.readValue(header);
    if (result != Result::Success) return result;

    // Validate header
    if (
      header.magic[0] != COIL_MAGIC[0] ||
      header.magic[1] != COIL_MAGIC[1] ||
      header.magic[2] != COIL_MAGIC[2] ||
      header.magic[3] != COIL_MAGIC[3]
    ) {
      return Result::InvalidFormat;
    }

    if (header.version != COIL_VERSION) {
      reportError(
        ErrorLevel::Warning, 
        "Object format version is incompatible. file - %hu, library - %hu\n", 
        header.version, COIL_VERSION
      );
    }

    return result;
  }
  Result __load_section(Stream &stream, Section &section) {
    Result result = Result::Success;

    // Load Section Header
    result = stream.readValue(section.header);
    if (result != Result::Success) return result;
  
    // Setup Section Object
    section.data.resize(section.header.size);

    // Load Section Data
    size_t bytesread = stream.read(section.data.data(), section.header.size);
    if (bytesread != section.header.size) {
      return Result::IoError;
    }

    return result;
  }
  Result __save_section(Stream &stream, Section &section) {
    Result result = Result::Success;

    // Write Section Header
    result = stream.writeValue(section.header);
    if (result != Result::Success) return result;
  
    // Write Section Data
    size_t byteswritten = stream.read(section.data.data(), section.header.size);
    if (byteswritten != section.header.size) {
      return Result::IoError;
    }

    return result;
  }

  // Object Functionality
  Result Object::load(Stream& stream) {
    Result result;
    
    // Load Header
    result = __load_header(stream, this->header);
    if (result != Result::Success) return result;

    // Utilize header information to construct in-memory object
    this->sections.resize(this->header.section_count);

    // Load Sections
    for (size_t i = 0; i < this->header.section_count; ++i) {
      result = __load_section(stream, this->sections[i]);
      if (result != Result::Success) return result;

      if (this->sections[i].header.type == (u8)SectionType::StrTab) {
        if (this->strtab) {
          reportError(
            ErrorLevel::Error,
            "The object file loaded contains two string tables\n"
          );
          return Result::InvalidFormat;
        } else {
          this->strtab = this->sections.data() + i;
          this->strings = (const char*)this->strtab->data.data();
        }
      } else if (this->sections[i].header.type == (u8)SectionType::SymTab) {
        if (this->symtab) {
          reportError(
            ErrorLevel::Error,
            "The object file loaded contains two symbol tables\n"
          );
          return Result::InvalidFormat;
        } else {
          this->symtab = this->sections.data() + i;
        }
      }
    }

    return Result::Success;
  }

  Result Object::save(Stream& stream) {
    Result result;
    
    // Write Header
    result = stream.writeValue(this->header);
    if (result != Result::Success) return result;

    // Write Sections
    for (size_t i = 0; i < this->header.section_count; ++i) {
      result = __save_section(stream, this->sections[i]);
      if (result != Result::Success) return result;
    }

    return Result::Success;
  }

  // -------------------------------- Section Functionality -------------------------------- //
  u16 Object::getSectionIndex(const char *name, size_t namelen) {
    // Search sections and compare each name
    for (size_t i = 0; i < sections.size(); ++i) {
      const SectionHeader &sec = sections[i].header;

      // get string at offset
      const char *secname = this->getString(sec.name); 

      // compare section name and expected name
      if (strncmp(secname, name, namelen) == 0) {
        return i + 1; // incrememnt as zero is a fail
      }
    }

    return 0;
  }

  u16 Object::putSection(u64 name, u16 flags, u8 type, u64 size, const u8 *data, u64 datasize) {
    SectionHeader header;
    header.name = name;
    header.size = size;
    header.flags = flags;
    header.type = type;
    return putSection(header, data, datasize);
  }
  u16 Object::putSection(const SectionHeader& info, const u8 *data, u64 datasize) {
    Section section(info);

    if (info.size != 0) {
      section.data.reserve(info.size);
    }
    if (data && datasize != 0) {
      section.data.resize(datasize);
      memcpy(section.data.data(), data, datasize);
    }

    // Push section
    sections.push_back(std::move(section));

    if (info.type == (u8)SectionType::StrTab) {
      if (this->strtab) {
        reportError(
          ErrorLevel::Error,
          "Creating object with two string tables\n"
        );
        return 0;
      } else {
        this->strtab = getSection(sections.size());
        getString(0); // initalizes strings member
      }
    } else if (info.type == (u8)SectionType::SymTab) {
      if (this->symtab) {
        reportError(
          ErrorLevel::Error,
          "Creating object with two string tables\n"
        );
        return 0;
      } else {
        this->symtab = getSection(sections.size());
      }
    }

    // return index of section
    return sections.size(); // indicies are incremented by one as zero is taken for errors
  }

  // -------------------------------- Symbol Table Functionality -------------------------------- //
  u16 Object::getSymbolIndex(const char *name, size_t namelen) {
    for (size_t i = 0; i < symtab->symbols.size(); ++i) {
      Symbol &sym = symtab->symbols[i];
      
      // get string at offset
      const char *symname = this->getString(sym.name); 

      // compare section name and expected name
      if (strncmp(symname, name, namelen) == 0) {
        return i + 1; // incrememnt as zero is a fail
      }
    }

    return 0;
  }

  u16 Object::putSymbol(u64 name, u32 value, u16 section_index, u8 type, u8 binding) {
    Symbol symbol;
    symbol.name=name; 
    symbol.value=value; 
    symbol.section_index=section_index; 
    symbol.type=type; 
    symbol.binding=binding;
    return putSymbol(symbol);
  }
  u16 Object::putSymbol(const Symbol& symbol) {
    symtab->symbols.push_back(symbol);
    return symtab->symbols.size();
  }

  // -------------------------------- String Table Functionality -------------------------------- //
  Result Object::putString(const char *str) {
    if (!strings || !strtab) return Result::BadState; // string table not initalized
    const u8 *stru8 = (const u8*)str;
    strtab->data.insert(strtab->data.end(), stru8, stru8 + std::strlen(str) + 1);
    strings = (const char*)strtab->data.data();
    return Result::Success;
  }
}; // namespace coil