#include <catch2/catch_all.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"

// Helper for capturing log output
struct CaptureBuffer {
  static constexpr size_t BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];
  FILE* fp;
  
  CaptureBuffer() {
    memset(buffer, 0, BUFFER_SIZE);
    fp = fmemopen(buffer, BUFFER_SIZE, "w+");
  }
  
  ~CaptureBuffer() {
    if (fp) fclose(fp);
  }
  
  FILE* getFile() const {
    return fp;
  }
  
  const char* getBuffer() const {
    return buffer;
  }
  
  void clear() {
    if (fp) fclose(fp);
    memset(buffer, 0, BUFFER_SIZE);
    fp = fmemopen(buffer, BUFFER_SIZE, "w+");
  }
  
  bool contains(const char* str) const {
    return strstr(buffer, str) != nullptr;
  }
};

// Helper for temporary files
struct TempFile {
  char filename[256];
  
  TempFile() {
    snprintf(filename, sizeof(filename), "coil_test_obj_%p.tmp", (void*)this);
  }
  
  ~TempFile() {
    std::remove(filename);
  }
  
  const char* getFilename() const {
    return filename;
  }
};

// Create context for testing
coil::Context createTestContext() {
  static CaptureBuffer capture;
  static coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
  static coil::ErrorManager errorMgr(&logger);
  return {&logger, &errorMgr};
}

TEST_CASE("Object format utilities", "[object]") {
  SECTION("File type names") {
    REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_NONE), "None") == 0);
    REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_REL), "Relocatable") == 0);
    REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_EXEC), "Executable") == 0);
    REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_SHARED), "Shared") == 0);
    REQUIRE(strcmp(coil::obj::getFileTypeName(999), "Unknown") == 0);
  }
  
  SECTION("Section type names") {
    REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_NULL), "Null") == 0);
    REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_CODE), "Code") == 0);
    REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_DATA), "Data") == 0);
    REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_SYMTAB), "SymTab") == 0);
    REQUIRE(strcmp(coil::obj::getSectionTypeName(999), "Unknown") == 0);
  }
  
  SECTION("Section flags string") {
    char buffer[32] = {0};
    
    // Empty flags
    coil::obj::getSectionFlagsString(0, buffer, sizeof(buffer));
    REQUIRE(buffer[0] == '\0');
    
    // Write flag
    coil::obj::getSectionFlagsString(coil::obj::CSF_WRITE, buffer, sizeof(buffer));
    REQUIRE(strcmp(buffer, "W") == 0);
    
    // Multiple flags
    coil::obj::getSectionFlagsString(
      coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC | coil::obj::CSF_PROC,
      buffer, sizeof(buffer)
    );
    // Check that all expected flags are present
    REQUIRE(strchr(buffer, 'W') != nullptr);
    REQUIRE(strchr(buffer, 'A') != nullptr);
    REQUIRE(strchr(buffer, 'P') != nullptr);
  }
}

TEST_CASE("Symbol operations", "[object]") {
  SECTION("Symbol type and binding") {
    coil::CoilSymbol symbol = {};
    
    // Set type and binding
    symbol.setType(coil::obj::CST_FUNC);
    symbol.setBinding(coil::obj::CSB_GLOBAL);
    
    // Verify values
    REQUIRE(symbol.getType() == coil::obj::CST_FUNC);
    REQUIRE(symbol.getBinding() == coil::obj::CSB_GLOBAL);
    
    // Change type
    symbol.setType(coil::obj::CST_OBJECT);
    REQUIRE(symbol.getType() == coil::obj::CST_OBJECT);
    REQUIRE(symbol.getBinding() == coil::obj::CSB_GLOBAL); // Binding should remain unchanged
    
    // Change binding
    symbol.setBinding(coil::obj::CSB_WEAK);
    REQUIRE(symbol.getType() == coil::obj::CST_OBJECT);
    REQUIRE(symbol.getBinding() == coil::obj::CSB_WEAK);
  }
}

TEST_CASE("Relocation operations", "[object]") {
  SECTION("Relocation type and symbol") {
    coil::CoilRelocation rel = {};
    
    // Set values
    rel.offset = 0x1000;
    rel.setSymbol(42);
    rel.setType(coil::obj::CR_DIRECT32);
    
    // Verify values
    REQUIRE(rel.offset == 0x1000);
    REQUIRE(rel.getSymbol() == 42);
    REQUIRE(rel.getType() == coil::obj::CR_DIRECT32);
    
    // Change values
    rel.setSymbol(99);
    rel.setType(coil::obj::CR_REL32);
    
    // Verify changes
    REQUIRE(rel.getSymbol() == 99);
    REQUIRE(rel.getType() == coil::obj::CR_REL32);
  }
}

TEST_CASE("Section data operations", "[object]") {
  SECTION("Creating section data") {
    const char* testData = "Test section data";
    coil::SectionData section = coil::SectionData::create(
      ".test", coil::obj::CST_DATA, coil::obj::CSF_WRITE, 
      reinterpret_cast<const uint8_t*>(testData), strlen(testData), 0
    );
    
    // Verify section properties
    REQUIRE(strcmp(section.name, ".test") == 0);
    REQUIRE(section.header.type == coil::obj::CST_DATA);
    REQUIRE(section.header.flags == coil::obj::CSF_WRITE);
    REQUIRE(section.header.size == strlen(testData));
    REQUIRE(section.data != nullptr);
    REQUIRE(section.ownsData == true);
    REQUIRE(memcmp(section.data, testData, strlen(testData)) == 0);
    
    // Free section data
    section.freeData();
    REQUIRE(section.data == nullptr);
    REQUIRE(section.ownsData == false);
  }
  
  SECTION("String table operations") {
    uint8_t strTableData[] = "\0first\0second\0third";
    size_t strTableSize = sizeof(strTableData) - 1; // exclude terminal null from literal
    
    coil::SectionData section = coil::SectionData::create(
      ".strtab", coil::obj::CST_STRTAB, coil::obj::CSF_STRINGS,
      strTableData, strTableSize, 0
    );
    
    // Test getting strings
    REQUIRE(section.getString(0) != nullptr);
    REQUIRE(strcmp(section.getString(0), "") == 0);
    REQUIRE(strcmp(section.getString(1), "first") == 0);
    REQUIRE(strcmp(section.getString(7), "second") == 0);
    REQUIRE(strcmp(section.getString(14), "third") == 0);
    
    // Test invalid offset
    REQUIRE(section.getString(strTableSize) == nullptr);
    
    section.freeData();
  }
  
  SECTION("Symbol table operations") {
    // Create a symbol table with 2 entries
    coil::CoilSymbol symbols[2];
    memset(symbols, 0, sizeof(symbols));
    
    // First symbol: local variable
    symbols[0].name = 1;  // Offset into string table
    symbols[0].value = 0x100;
    symbols[0].size = 4;
    symbols[0].setType(coil::obj::CST_OBJECT);
    symbols[0].setBinding(coil::obj::CSB_LOCAL);
    symbols[0].shndx = 1;  // Section index
    
    // Second symbol: global function
    symbols[1].name = 7;  // Offset into string table
    symbols[1].value = 0x200;
    symbols[1].size = 32;
    symbols[1].setType(coil::obj::CST_FUNC);
    symbols[1].setBinding(coil::obj::CSB_GLOBAL);
    symbols[1].shndx = 2;  // Section index
    
    coil::SectionData section = coil::SectionData::create(
      ".symtab", coil::obj::CST_SYMTAB, 0,
      reinterpret_cast<const uint8_t*>(symbols), sizeof(symbols), sizeof(coil::CoilSymbol)
    );
    
    // Test getting symbols
    REQUIRE(section.getEntryCount() == 2);
    
    coil::CoilSymbol sym0 = section.getSymbol(0);
    REQUIRE(sym0.name == 1);
    REQUIRE(sym0.value == 0x100);
    REQUIRE(sym0.size == 4);
    REQUIRE(sym0.getType() == coil::obj::CST_OBJECT);
    REQUIRE(sym0.getBinding() == coil::obj::CSB_LOCAL);
    REQUIRE(sym0.shndx == 1);
    
    coil::CoilSymbol sym1 = section.getSymbol(1);
    REQUIRE(sym1.name == 7);
    REQUIRE(sym1.value == 0x200);
    REQUIRE(sym1.size == 32);
    REQUIRE(sym1.getType() == coil::obj::CST_FUNC);
    REQUIRE(sym1.getBinding() == coil::obj::CSB_GLOBAL);
    REQUIRE(sym1.shndx == 2);
    
    section.freeData();
  }
}

TEST_CASE("StringTable operations", "[object]") {
  SECTION("Creating a string table") {
    coil::StringTable table = coil::StringTable::create();
    
    // Initial state: empty string at index 0
    REQUIRE(table.size == 1);
    REQUIRE(table.data[0] == 0);
    REQUIRE(table.getString(0) != nullptr);
    REQUIRE(strcmp(table.getString(0), "") == 0);
    
    // Add strings
    uint32_t offset1 = table.addString("first");
    uint32_t offset2 = table.addString("second");
    uint32_t offset3 = table.addString("third");
    
    // Verify offsets
    REQUIRE(offset1 == 1);
    REQUIRE(offset2 == 7);
    REQUIRE(offset3 == 14);
    
    // Verify strings
    REQUIRE(strcmp(table.getString(offset1), "first") == 0);
    REQUIRE(strcmp(table.getString(offset2), "second") == 0);
    REQUIRE(strcmp(table.getString(offset3), "third") == 0);
    
    // Adding same string should return existing offset
    REQUIRE(table.addString("first") == offset1);
  }
  
  SECTION("Creating from section") {
    uint8_t strTableData[] = "\0first\0second\0third";
    size_t strTableSize = sizeof(strTableData) - 1;
    
    coil::SectionData section = coil::SectionData::create(
      ".strtab", coil::obj::CST_STRTAB, coil::obj::CSF_STRINGS,
      strTableData, strTableSize, 0
    );
    
    coil::StringTable table = coil::StringTable::fromSection(section);
    
    // Verify size
    REQUIRE(table.size == strTableSize);
    
    // Verify strings
    REQUIRE(strcmp(table.getString(0), "") == 0);
    REQUIRE(strcmp(table.getString(1), "first") == 0);
    REQUIRE(strcmp(table.getString(7), "second") == 0);
    REQUIRE(strcmp(table.getString(14), "third") == 0);
    
    section.freeData();
  }
}

TEST_CASE("CoilObject operations", "[object]") {
  auto ctx = createTestContext();
  
  SECTION("Creating a COIL object") {
    coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, &ctx);
    
    // Verify object was created
    REQUIRE(obj.header.type == coil::obj::CT_REL);
    REQUIRE(obj.sectionCount > 0); // Should have at least a string table section
    
    // Verify string table section was created
    REQUIRE(obj.header.shstrndx < obj.sectionCount);
    const coil::SectionData* strTab = obj.getSection(obj.header.shstrndx);
    REQUIRE(strTab != nullptr);
    REQUIRE(strTab->header.type == coil::obj::CST_STRTAB);
    
    obj.cleanup();
  }
  
  SECTION("Adding sections to an object") {
    coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_EXEC, &ctx);
    
    // Add a code section
    const char* codeData = "Example code data";
    coil::SectionData* codeSection = obj.addSection(
      ".text", coil::obj::CST_CODE, coil::obj::CSF_PROC,
      reinterpret_cast<const uint8_t*>(codeData), strlen(codeData), 0
    );
    
    REQUIRE(codeSection != nullptr);
    REQUIRE(strcmp(codeSection->name, ".text") == 0);
    REQUIRE(codeSection->header.type == coil::obj::CST_CODE);
    REQUIRE(codeSection->header.flags == coil::obj::CSF_PROC);
    REQUIRE(codeSection->header.size == strlen(codeData));
    
    // Add a data section
    const char* dataData = "Example data";
    coil::SectionData* dataSection = obj.addSection(
      ".data", coil::obj::CST_DATA, coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC,
      reinterpret_cast<const uint8_t*>(dataData), strlen(dataData), 0
    );
    
    REQUIRE(dataSection != nullptr);
    REQUIRE(strcmp(dataSection->name, ".data") == 0);
    REQUIRE(dataSection->header.type == coil::obj::CST_DATA);
    REQUIRE(dataSection->header.flags == (coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC));
    REQUIRE(dataSection->header.size == strlen(dataData));
    
    // Verify sections can be found by name
    REQUIRE(obj.getSectionByName(".text") == codeSection);
    REQUIRE(obj.getSectionByName(".data") == dataSection);
    
    obj.cleanup();
  }
  
  SECTION("Adding symbols to an object") {
    coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, &ctx);
    
    // Add a code section
    const char* codeData = "Example code data";
    coil::SectionData* codeSection = obj.addSection(
      ".text", coil::obj::CST_CODE, coil::obj::CSF_PROC,
      reinterpret_cast<const uint8_t*>(codeData), strlen(codeData), 0
    );
    
    REQUIRE(codeSection != nullptr);
    
    // Add symbols
    REQUIRE(obj.addSymbol("main", 0, 16, coil::obj::CST_FUNC, coil::obj::CSB_GLOBAL, 1));
    REQUIRE(obj.addSymbol("printf", 0, 0, coil::obj::CST_FUNC, coil::obj::CSB_EXTERN, 0));
    REQUIRE(obj.addSymbol("data", 0, 4, coil::obj::CST_OBJECT, coil::obj::CSB_LOCAL, 1));
    
    // Verify symbol table and string table were created
    const coil::SectionData* symTab = obj.getSectionByName(".symtab");
    REQUIRE(symTab != nullptr);
    REQUIRE(symTab->header.type == coil::obj::CST_SYMTAB);
    REQUIRE(symTab->getEntryCount() == 3);
    
    // Find symbols by name
    auto mainSym = obj.findSymbol("main");
    REQUIRE(mainSym.first != nullptr);
    REQUIRE(mainSym.second.getType() == coil::obj::CST_FUNC);
    REQUIRE(mainSym.second.getBinding() == coil::obj::CSB_GLOBAL);
    
    auto printfSym = obj.findSymbol("printf");
    REQUIRE(printfSym.first != nullptr);
    REQUIRE(printfSym.second.getType() == coil::obj::CST_FUNC);
    REQUIRE(printfSym.second.getBinding() == coil::obj::CSB_EXTERN);
    
    auto dataSym = obj.findSymbol("data");
    REQUIRE(dataSym.first != nullptr);
    REQUIRE(dataSym.second.getType() == coil::obj::CST_OBJECT);
    REQUIRE(dataSym.second.getBinding() == coil::obj::CSB_LOCAL);
    
    obj.cleanup();
  }
  
  SECTION("Saving and loading a COIL object") {
    TempFile tempFile;
    
    // Create an object with sections and symbols
    {
      coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, &ctx);
      
      // Add a code section
      const char* codeData = "Example code";
      obj.addSection(
        ".text", coil::obj::CST_CODE, coil::obj::CSF_PROC,
        reinterpret_cast<const uint8_t*>(codeData), strlen(codeData), 0
      );
      
      // Add a data section
      const char* dataData = "Example data";
      obj.addSection(
        ".data", coil::obj::CST_DATA, coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC,
        reinterpret_cast<const uint8_t*>(dataData), strlen(dataData), 0
      );
      
      // Add a symbol
      obj.addSymbol("main", 0, 16, coil::obj::CST_FUNC, coil::obj::CSB_GLOBAL, 1);
      
      // Save the object to a file
      coil::FileStream stream = coil::FileStream::open(tempFile.getFilename(), "wb", &ctx);
      REQUIRE(obj.save(&stream));
      stream.close();
      
      obj.cleanup();
    }
    
    // Load the object from the file
    {
      coil::FileStream stream = coil::FileStream::open(tempFile.getFilename(), "rb", &ctx);
      
      // Verify file is a COIL file
      REQUIRE(coil::CoilObject::isCoilFile(&stream));
      
      // Load the object
      coil::CoilObject obj = coil::CoilObject::load(&stream, &ctx);
      stream.close();
      
      // Verify object properties
      REQUIRE(obj.header.type == coil::obj::CT_REL);
      
      printf("sectionCount: %zu\n", obj.sectionCount);
      for (size_t i = 0; i < obj.sectionCount; ++i) {
        printf("Section %zu, SectionName '%s'", i, obj.sections[i].name);
      }

      

      // Verify sections
      const coil::SectionData* textSection = obj.getSectionByName(".text");
      REQUIRE(textSection != nullptr);
      REQUIRE(textSection->header.type == coil::obj::CST_CODE);
      REQUIRE(textSection->header.flags == coil::obj::CSF_PROC);
      REQUIRE(textSection->data != nullptr);
      REQUIRE(memcmp(textSection->data, "Example code", 12) == 0);
      
      const coil::SectionData* dataSection = obj.getSectionByName(".data");
      REQUIRE(dataSection != nullptr);
      REQUIRE(dataSection->header.type == coil::obj::CST_DATA);
      REQUIRE(dataSection->header.flags == (coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC));
      REQUIRE(dataSection->data != nullptr);
      REQUIRE(memcmp(dataSection->data, "Example data", 12) == 0);
      
      // Verify symbol
      auto mainSym = obj.findSymbol("main");
      REQUIRE(mainSym.first != nullptr);
      REQUIRE(mainSym.second.getType() == coil::obj::CST_FUNC);
      REQUIRE(mainSym.second.getBinding() == coil::obj::CSB_GLOBAL);
      
      obj.cleanup();
    }
  }
}