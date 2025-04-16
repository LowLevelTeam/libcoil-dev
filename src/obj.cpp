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

      if (section.header.type == SectionType::StrTab) {
        if (this->strtab) {
          reportError(
            ErrorLevel::Error,
            "The object file loaded contains two string tables\n"
          );
        } else {
          this->strtab = this->sections.data() + i;
        }
      } else if (section.header.type == SectionType::SymTab) {
        if (this->strtab) {
          reportError(
            ErrorLevel::Error,
            "The object file loaded contains two symbol tables\n"
          );
        } else {
          this->symbols = reinterpret_cast<Symbol*>(this->sections[i].data());
          this->symbol_count = this->sections[i].header.size / sizeof(Symbol);
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
    for (size_t i = 0; i < this->header.section_count; ++i) {
      // get string table offset for section name
      u64 sectnameoff = this->sections[i].header.name;

      // get section name from the string table
      const char *sectname = this->strtab->data.data() + sectnameoff;

      // compare section name and expected name
      if (strncmp(sectname, name, namelen) == 0) {
        return i + 1; // incrememnt as zero is a fail
      }
    }

    // Light fail
    return 0;
  }

  // -------------------------------- Symbol Table Functionality -------------------------------- //
  u16 Object::getSymbolIndex(const char *name, size_t namelen) {
    for (size_t i = 0; i < this->symbol_count; ++i) {
      // get string table offset for symbol name
      u64 symnameoff = this->symbols[i].name;

      // get section name from the string table
      const char *symname = this->strtab->data.data() + symnameoff;

      // compare section name and expected name
      if (strncmp(symname, name, namelen) == 0) {
        return i + 1; // incrememnt as zero is a fail
      }
    }

    // Light fail
    return 0;
  }

  // -------------------------------- String Table Functionality -------------------------------- //


} // namespace coil