#include <catch2/catch_all.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"

// Helper class for capturing log output
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
        REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_DYN), "Shared Object") == 0);
        REQUIRE(strcmp(coil::obj::getFileTypeName(coil::obj::CT_LIB), "Library") == 0);
        REQUIRE(strcmp(coil::obj::getFileTypeName(999), "Unknown") == 0);
    }
    
    SECTION("Section type names") {
        REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_NULL), "Null") == 0);
        REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_CODE), "Code") == 0);
        REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_DATA), "Data") == 0);
        REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_SYMTAB), "Symbol Table") == 0);
        REQUIRE(strcmp(coil::obj::getSectionTypeName(coil::obj::CST_STRTAB), "String Table") == 0);
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
            coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC | coil::obj::CSF_EXEC,
            buffer, sizeof(buffer)
        );
        // Check that all expected flags are present (order doesn't matter)
        REQUIRE(strchr(buffer, 'W') != nullptr);
        REQUIRE(strchr(buffer, 'A') != nullptr);
        REQUIRE(strchr(buffer, 'X') != nullptr);
        
        // Check buffer safety with small buffer
        char smallBuffer[2] = {0};
        coil::obj::getSectionFlagsString(
            coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC | coil::obj::CSF_EXEC,
            smallBuffer, sizeof(smallBuffer)
        );
        // Should only contain one character + null terminator
        REQUIRE(strlen(smallBuffer) == 1);
    }
    
    SECTION("Symbol binding names") {
        REQUIRE(strcmp(coil::obj::getSymbolBindingName(coil::obj::CSB_LOCAL), "Local") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolBindingName(coil::obj::CSB_GLOBAL), "Global") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolBindingName(coil::obj::CSB_WEAK), "Weak") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolBindingName(coil::obj::CSB_EXTERN), "External") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolBindingName(99), "Unknown") == 0);
    }
    
    SECTION("Symbol type names") {
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(coil::obj::CST_NOTYPE), "None") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(coil::obj::CST_OBJECT), "Object") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(coil::obj::CST_FUNC), "Function") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(coil::obj::CST_SECTION), "Section") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(coil::obj::CST_FILE), "File") == 0);
        REQUIRE(strcmp(coil::obj::getSymbolTypeName(99), "Unknown") == 0);
    }
}

TEST_CASE("Symbol table entry", "[object]") {
    SECTION("Symbol type and binding") {
        coil::CoilSymbolEntry symbol;
        memset(&symbol, 0, sizeof(symbol));
        
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

TEST_CASE("Relocation entries", "[object]") {
    SECTION("RelEntry type and symbol") {
        coil::CoilRelEntry rel;
        memset(&rel, 0, sizeof(rel));
        
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
        rel.setType(coil::obj::CR_PC32);
        
        // Verify changes
        REQUIRE(rel.getSymbol() == 99);
        REQUIRE(rel.getType() == coil::obj::CR_PC32);
    }
    
    SECTION("RelaEntry type and symbol") {
        coil::CoilRelaEntry rela;
        memset(&rela, 0, sizeof(rela));
        
        // Set values
        rela.offset = 0x2000;
        rela.setSymbol(42);
        rela.setType(coil::obj::CR_DIRECT64);
        rela.addend = -123;
        
        // Verify values
        REQUIRE(rela.offset == 0x2000);
        REQUIRE(rela.getSymbol() == 42);
        REQUIRE(rela.getType() == coil::obj::CR_DIRECT64);
        REQUIRE(rela.addend == -123);
        
        // Change values
        rela.setSymbol(99);
        rela.setType(coil::obj::CR_PC64);
        rela.addend = 456;
        
        // Verify changes
        REQUIRE(rela.getSymbol() == 99);
        REQUIRE(rela.getType() == coil::obj::CR_PC64);
        REQUIRE(rela.addend == 456);
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
        size_t strTableSize = sizeof(strTableData);
        
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
        coil::CoilSymbolEntry symbols[2];
        
        // First symbol: local variable
        symbols[0].name = 1;  // Offset into string table
        symbols[0].value = 0x100;
        symbols[0].size = 4;
        symbols[0].setType(coil::obj::CST_OBJECT);
        symbols[0].setBinding(coil::obj::CSB_LOCAL);
        symbols[0].other = 0;
        symbols[0].shndx = 1;  // Section index
        
        // Second symbol: global function
        symbols[1].name = 7;  // Offset into string table
        symbols[1].value = 0x200;
        symbols[1].size = 32;
        symbols[1].setType(coil::obj::CST_FUNC);
        symbols[1].setBinding(coil::obj::CSB_GLOBAL);
        symbols[1].other = 0;
        symbols[1].shndx = 2;  // Section index
        
        coil::SectionData section = coil::SectionData::create(
            ".symtab", coil::obj::CST_SYMTAB, 0,
            reinterpret_cast<const uint8_t*>(symbols), sizeof(symbols), sizeof(coil::CoilSymbolEntry)
        );
        
        // Test getting symbols
        REQUIRE(section.getEntryCount() == 2);
        
        coil::CoilSymbolEntry sym0 = section.getSymbol(0);
        REQUIRE(sym0.name == 1);
        REQUIRE(sym0.value == 0x100);
        REQUIRE(sym0.size == 4);
        REQUIRE(sym0.getType() == coil::obj::CST_OBJECT);
        REQUIRE(sym0.getBinding() == coil::obj::CSB_LOCAL);
        REQUIRE(sym0.shndx == 1);
        
        coil::CoilSymbolEntry sym1 = section.getSymbol(1);
        REQUIRE(sym1.name == 7);
        REQUIRE(sym1.value == 0x200);
        REQUIRE(sym1.size == 32);
        REQUIRE(sym1.getType() == coil::obj::CST_FUNC);
        REQUIRE(sym1.getBinding() == coil::obj::CSB_GLOBAL);
        REQUIRE(sym1.shndx == 2);
        
        // Test setting symbols
        coil::CoilSymbolEntry newSym;
        newSym.name = 14;  // Offset into string table
        newSym.value = 0x300;
        newSym.size = 8;
        newSym.setType(coil::obj::CST_OBJECT);
        newSym.setBinding(coil::obj::CSB_EXTERN);
        newSym.other = 0;
        newSym.shndx = 3;  // Section index
        
        section.setSymbol(1, newSym);
        
        // Verify the symbol was updated
        sym1 = section.getSymbol(1);
        REQUIRE(sym1.name == 14);
        REQUIRE(sym1.value == 0x300);
        REQUIRE(sym1.size == 8);
        REQUIRE(sym1.getType() == coil::obj::CST_OBJECT);
        REQUIRE(sym1.getBinding() == coil::obj::CSB_EXTERN);
        REQUIRE(sym1.shndx == 3);
        
        section.freeData();
    }
    
    SECTION("Relocation table operations") {
        // Create a relocation table with 2 entries
        coil::CoilRelEntry rels[2];
        
        // First relocation
        rels[0].offset = 0x1000;
        rels[0].setSymbol(1);
        rels[0].setType(coil::obj::CR_DIRECT32);
        
        // Second relocation
        rels[1].offset = 0x2000;
        rels[1].setSymbol(2);
        rels[1].setType(coil::obj::CR_PC32);
        
        coil::SectionData section = coil::SectionData::create(
            ".rel.text", coil::obj::CST_REL, 0,
            reinterpret_cast<const uint8_t*>(rels), sizeof(rels), sizeof(coil::CoilRelEntry)
        );
        
        // Test getting relocations
        REQUIRE(section.getEntryCount() == 2);
        
        coil::CoilRelEntry rel0 = section.getRel(0);
        REQUIRE(rel0.offset == 0x1000);
        REQUIRE(rel0.getSymbol() == 1);
        REQUIRE(rel0.getType() == coil::obj::CR_DIRECT32);
        
        coil::CoilRelEntry rel1 = section.getRel(1);
        REQUIRE(rel1.offset == 0x2000);
        REQUIRE(rel1.getSymbol() == 2);
        REQUIRE(rel1.getType() == coil::obj::CR_PC32);
        
        // Test setting relocations
        coil::CoilRelEntry newRel;
        newRel.offset = 0x3000;
        newRel.setSymbol(3);
        newRel.setType(coil::obj::CR_GOT32);
        
        section.setRel(1, newRel);
        
        // Verify the relocation was updated
        rel1 = section.getRel(1);
        REQUIRE(rel1.offset == 0x3000);
        REQUIRE(rel1.getSymbol() == 3);
        REQUIRE(rel1.getType() == coil::obj::CR_GOT32);
        
        section.freeData();
    }
    
    SECTION("Relocation with addend table operations") {
        // Create a relocation table with 2 entries
        coil::CoilRelaEntry relas[2];
        
        // First relocation
        relas[0].offset = 0x1000;
        relas[0].setSymbol(1);
        relas[0].setType(coil::obj::CR_DIRECT64);
        relas[0].addend = -100;
        
        // Second relocation
        relas[1].offset = 0x2000;
        relas[1].setSymbol(2);
        relas[1].setType(coil::obj::CR_PC64);
        relas[1].addend = 200;
        
        coil::SectionData section = coil::SectionData::create(
            ".rela.text", coil::obj::CST_RELA, 0,
            reinterpret_cast<const uint8_t*>(relas), sizeof(relas), sizeof(coil::CoilRelaEntry)
        );
        
        // Test getting relocations
        REQUIRE(section.getEntryCount() == 2);
        
        coil::CoilRelaEntry rela0 = section.getRela(0);
        REQUIRE(rela0.offset == 0x1000);
        REQUIRE(rela0.getSymbol() == 1);
        REQUIRE(rela0.getType() == coil::obj::CR_DIRECT64);
        REQUIRE(rela0.addend == -100);
        
        coil::CoilRelaEntry rela1 = section.getRela(1);
        REQUIRE(rela1.offset == 0x2000);
        REQUIRE(rela1.getSymbol() == 2);
        REQUIRE(rela1.getType() == coil::obj::CR_PC64);
        REQUIRE(rela1.addend == 200);
        
        // Test setting relocations
        coil::CoilRelaEntry newRela;
        newRela.offset = 0x3000;
        newRela.setSymbol(3);
        newRela.setType(coil::obj::CR_COPY);
        newRela.addend = 300;
        
        section.setRela(1, newRela);
        
        // Verify the relocation was updated
        rela1 = section.getRela(1);
        REQUIRE(rela1.offset == 0x3000);
        REQUIRE(rela1.getSymbol() == 3);
        REQUIRE(rela1.getType() == coil::obj::CR_COPY);
        REQUIRE(rela1.addend == 300);
        
        section.freeData();
    }
}

TEST_CASE("COIL header operations", "[object]") {
    SECTION("Header initialization") {
        // Create a header for a relocatable file for x86-64 architecture
        coil::CoilHeader header = coil::CoilHeader::initialize(coil::obj::CT_REL, 2);
        
        // Verify magic number
        REQUIRE(header.ident[0] == coil::obj::COILMAG0);
        REQUIRE(header.ident[1] == coil::obj::COILMAG1);
        REQUIRE(header.ident[2] == coil::obj::COILMAG2);
        REQUIRE(header.ident[3] == coil::obj::COILMAG3);
        REQUIRE(header.ident[4] == coil::obj::COILMAG4);
        
        // Verify version
        REQUIRE(header.ident[6] == coil::obj::COIL_VERSION);
        REQUIRE(header.version == coil::obj::COIL_VERSION);
        
        // Verify type and machine
        REQUIRE(header.type == coil::obj::CT_REL);
        REQUIRE(header.flags == 2); // x86-64 machine
        
        // Verify sizes
        REQUIRE(header.ehsize == sizeof(coil::CoilHeader));
        REQUIRE(header.shentsize == sizeof(coil::CoilSectionHeader));
    }
    
    SECTION("Endian detection") {
        coil::CoilHeader header = coil::CoilHeader::initialize(coil::obj::CT_REL, 0);
        
        // We can only check that the endianness byte is set to a valid value
        REQUIRE((header.ident[5] == coil::obj::COILDATA2LSB || 
                 header.ident[5] == coil::obj::COILDATA2MSB));
        
        // isLittleEndian should match the detected endianness
        REQUIRE(header.isLittleEndian() == (header.ident[5] == coil::obj::COILDATA2LSB));
    }
}

TEST_CASE("String table operations", "[object]") {
    SECTION("Creating a string table") {
        coil::StringTable table = coil::StringTable::create();
        
        // Initial state: empty string at index 0
        REQUIRE(table.size == 1);
        REQUIRE(table.data[0] == 0);
        REQUIRE(table.getString(0) != nullptr);
        REQUIRE(strcmp(table.getString(0), "") == 0);
        
        // Invalid offset
        REQUIRE(table.getString(table.size) == nullptr);
        
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
        
        // Adding the same string again should return the existing offset
        REQUIRE(table.addString("first") == offset1);
        REQUIRE(table.addString("second") == offset2);
        
        // Handle null string
        REQUIRE(table.addString(nullptr) == 0);
    }
    
    SECTION("Creating from section") {
        // Create a section with a properly constructed string table
        uint8_t strTableData[] = {
            0,                                  // Empty string
            'f', 'i', 'r', 's', 't', 0,         // "first"
            's', 'e', 'c', 'o', 'n', 'd', 0,    // "second"
            't', 'h', 'i', 'r', 'd', 0          // "third"
        };
        size_t strTableSize = sizeof(strTableData);
        
        coil::SectionData section = coil::SectionData::create(
            ".strtab", coil::obj::CST_STRTAB, coil::obj::CSF_STRINGS,
            strTableData, strTableSize, 0
        );
        
        // Create string table from section
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
        coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, 2, &ctx);
        
        // Verify object was created
        REQUIRE(obj.header.type == coil::obj::CT_REL);
        REQUIRE(obj.header.flags == 2); // x86-64
        REQUIRE(obj.sectionCount > 0); // Should have at least a string table section
        
        // Verify string table section was created
        REQUIRE(obj.header.shstrndx < obj.sectionCount);
        const coil::SectionData* strTab = obj.getSection(obj.header.shstrndx);
        REQUIRE(strTab != nullptr);
        REQUIRE(strTab->header.type == coil::obj::CST_STRTAB);
        
        obj.cleanup();
    }
    
    SECTION("Adding sections to an object") {
        coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, 2, &ctx);
        
        // Add a code section
        const char* codeData = "Example code data";
        coil::SectionData* codeSection = obj.addSection(
            ".text", coil::obj::CST_CODE, coil::obj::CSF_EXEC,
            reinterpret_cast<const uint8_t*>(codeData), strlen(codeData), 0
        );
        
        REQUIRE(codeSection != nullptr);
        REQUIRE(strcmp(codeSection->name, ".text") == 0);
        REQUIRE(codeSection->header.type == coil::obj::CST_CODE);
        REQUIRE(codeSection->header.flags == coil::obj::CSF_EXEC);
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
        
        // Verify section names were added to string table
        const coil::SectionData* strTab = obj.getSection(obj.header.shstrndx);
        REQUIRE(strTab != nullptr);
        REQUIRE(strTab->data != nullptr);
        REQUIRE(strTab->header.size > 1);
        
        // Clean up
        obj.cleanup();
    }
    
    SECTION("Adding symbols to an object") {
        coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, 2, &ctx);
        
        // Add a code section
        const char* codeData = "Example code data";
        coil::SectionData* codeSection = obj.addSection(
            ".text", coil::obj::CST_CODE, coil::obj::CSF_EXEC,
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
        
        const coil::SectionData* strTab = obj.getSectionByName(".strtab");
        REQUIRE(strTab != nullptr);
        REQUIRE(strTab->header.type == coil::obj::CST_STRTAB);
        
        // Find symbols by name
        auto mainSym = obj.findSymbol("main");
        REQUIRE(mainSym.first != nullptr);
        REQUIRE(mainSym.second != nullptr);
        REQUIRE(mainSym.second->getType() == coil::obj::CST_FUNC);
        REQUIRE(mainSym.second->getBinding() == coil::obj::CSB_GLOBAL);
        
        auto printfSym = obj.findSymbol("printf");
        REQUIRE(printfSym.first != nullptr);
        REQUIRE(printfSym.second != nullptr);
        REQUIRE(printfSym.second->getType() == coil::obj::CST_FUNC);
        REQUIRE(printfSym.second->getBinding() == coil::obj::CSB_EXTERN);
        
        auto dataSym = obj.findSymbol("data");
        REQUIRE(dataSym.first != nullptr);
        REQUIRE(dataSym.second != nullptr);
        REQUIRE(dataSym.second->getType() == coil::obj::CST_OBJECT);
        REQUIRE(dataSym.second->getBinding() == coil::obj::CSB_LOCAL);
        
        // Non-existent symbol
        auto nonExistentSym = obj.findSymbol("nonexistent");
        REQUIRE(nonExistentSym.first == nullptr);
        REQUIRE(nonExistentSym.second == nullptr);
        
        // Clean up
        obj.cleanup();
    }
    
    SECTION("Saving and loading a COIL object") {
        TempFile tempFile;
        
        // Create an object with sections
        {
            coil::CoilObject obj = coil::CoilObject::create(coil::obj::CT_REL, 2, &ctx);
            
            // Add a code section
            const char* codeData = "Example code";
            obj.addSection(
                ".text", coil::obj::CST_CODE, coil::obj::CSF_EXEC,
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
            REQUIRE(obj.header.flags == 2); // x86-64
            
            // Count sections (shstrtab, text, data, symtab, strtab)
            REQUIRE(obj.sectionCount == 5);
            
            // Verify text section
            const coil::SectionData* textSection = obj.getSectionByName(".text");
            REQUIRE(textSection != nullptr);
            REQUIRE(textSection->header.type == coil::obj::CST_CODE);
            REQUIRE(textSection->header.flags == coil::obj::CSF_EXEC);
            REQUIRE(textSection->data != nullptr);
            REQUIRE(memcmp(textSection->data, "Example code", 12) == 0);
            
            // Verify data section
            const coil::SectionData* dataSection = obj.getSectionByName(".data");
            REQUIRE(dataSection != nullptr);
            REQUIRE(dataSection->header.type == coil::obj::CST_DATA);
            REQUIRE(dataSection->header.flags == (coil::obj::CSF_WRITE | coil::obj::CSF_ALLOC));
            REQUIRE(dataSection->data != nullptr);
            REQUIRE(memcmp(dataSection->data, "Example data", 12) == 0);
            
            // Verify symbol
            auto mainSym = obj.findSymbol("main");
            REQUIRE(mainSym.first != nullptr);
            REQUIRE(mainSym.second != nullptr);
            REQUIRE(mainSym.second->getType() == coil::obj::CST_FUNC);
            REQUIRE(mainSym.second->getBinding() == coil::obj::CSB_GLOBAL);
            
            obj.cleanup();
        }
    }
}