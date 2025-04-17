/**
 * @file test_obj.cpp
 * @brief Tests for the COIL object format
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include <string>
#include <vector>
#include <filesystem>

// Helper function to create a memory stream with object data
std::unique_ptr<coil::MemoryStream> createObjectStream(const coil::Object& obj) {
  // Create a memory stream to write to
  auto stream = std::make_unique<coil::MemoryStream>(1024);
  
  // Save the object to the stream
  obj.save(*stream);
  
  // Reset stream position for reading
  stream->seek(0);
  
  return stream;
}

TEST_CASE("COIL Object Creation", "[obj]") {
  SECTION("Create empty object") {
    coil::Object obj = coil::Object::create();
    
    // Verify header fields
    CHECK(obj.getHeader().magic[0] == 'C');
    CHECK(obj.getHeader().magic[1] == 'O');
    CHECK(obj.getHeader().magic[2] == 'I');
    CHECK(obj.getHeader().magic[3] == 'L');
    CHECK(obj.getHeader().version == coil::COIL_VERSION);
    CHECK(obj.getHeader().section_count == 0);
    
    // Should have no sections, string table, or symbol table yet
    CHECK(obj.getSectionCount() == 0);
    CHECK(obj.getStringTable() == nullptr);
    CHECK(obj.getSymbolTable() == nullptr);
  }
}

TEST_CASE("COIL Object String Table", "[obj]") {
  coil::Object obj = coil::Object::create();
  
  SECTION("Initialize string table") {
    obj.initStringTable();
    CHECK(obj.getStringTable() != nullptr);
    
    // String table should have at least one section
    CHECK(obj.getSectionCount() == 1);
    
    // Should have a null byte at beginning
    const auto& strtab = obj.getStringTable()->getData();
    REQUIRE(strtab.size() > 0);
    CHECK(strtab[0] == 0);
    
    // Should have the section name in the string table
    std::string_view name = obj.getString(1);  // First string after null byte
    REQUIRE(!name.empty());
    CHECK(name == ".strtab");
  }
  
  SECTION("Add and retrieve strings") {
    obj.initStringTable();
    
    // Add some strings
    coil::u64 offset1 = obj.addString("test1");
    coil::u64 offset2 = obj.addString("test2");
    coil::u64 offset3 = obj.addString("longer test string");
    
    // Offsets should be non-zero
    CHECK(offset1 > 0);
    CHECK(offset2 > offset1);
    CHECK(offset3 > offset2);
    
    // Retrieve and verify strings
    CHECK(obj.getString(offset1) == "test1");
    CHECK(obj.getString(offset2) == "test2");
    CHECK(obj.getString(offset3) == "longer test string");
    
    // Adding the same string twice should return the same offset
    coil::u64 offset1b = obj.addString("test1");
    CHECK(offset1b == offset1);
  }
  
  SECTION("Invalid string operations") {
    // Get non-existent string
    CHECK(obj.getString(100).empty());
    
    // Initialize string table
    obj.initStringTable();
    
    // Adding an empty string should return 0
    CHECK(obj.addString("") == 0);
    
    // Get non-existent string after initialization
    CHECK(obj.getString(100).empty());
  }
}

TEST_CASE("COIL Object Symbol Table", "[obj]") {
  coil::Object obj = coil::Object::create();
  
  SECTION("Initialize symbol table") {
    obj.initSymbolTable();
    CHECK(obj.getSymbolTable() != nullptr);
    
    // Should have created both a string table and symbol table
    CHECK(obj.getSectionCount() == 2);
    CHECK(obj.getStringTable() != nullptr);
  }
  
  SECTION("Add and retrieve symbols") {
    obj.initSymbolTable();
    
    // Add some strings for symbol names
    coil::u64 name1 = obj.addString("symbol1");
    coil::u64 name2 = obj.addString("symbol2");
    
    // Add symbols
    coil::Symbol sym1 = {name1, 0x1000, 1, static_cast<coil::u8>(coil::SymbolType::Func), static_cast<coil::u8>(coil::SymbolBinding::Global)};
    coil::Symbol sym2 = {name2, 0x2000, 1, static_cast<coil::u8>(coil::SymbolType::Object), static_cast<coil::u8>(coil::SymbolBinding::Local)};
    
    coil::u16 index1 = obj.addSymbol(sym1);
    coil::u16 index2 = obj.addSymbol(sym2);
    
    // Indices should be valid
    CHECK(index1 == 1);
    CHECK(index2 == 2);
    
    // Retrieve symbols by index
    const coil::Symbol* retrieved1 = obj.getSymbol(index1);
    const coil::Symbol* retrieved2 = obj.getSymbol(index2);
    
    REQUIRE(retrieved1 != nullptr);
    REQUIRE(retrieved2 != nullptr);
    
    // Verify symbol properties
    CHECK(retrieved1->name == name1);
    CHECK(retrieved1->value == 0x1000);
    CHECK(retrieved1->section_index == 1);
    CHECK(retrieved1->type == static_cast<coil::u8>(coil::SymbolType::Func));
    CHECK(retrieved1->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
    
    CHECK(retrieved2->name == name2);
    CHECK(retrieved2->value == 0x2000);
    CHECK(retrieved2->section_index == 1);
    CHECK(retrieved2->type == static_cast<coil::u8>(coil::SymbolType::Object));
    CHECK(retrieved2->binding == static_cast<coil::u8>(coil::SymbolBinding::Local));
    
    // Retrieve symbol by name
    coil::u16 index1b = obj.getSymbolIndex("symbol1");
    CHECK(index1b == index1);
  }
  
  SECTION("Invalid symbol operations") {
    // Get non-existent symbol
    CHECK(obj.getSymbol(1) == nullptr);
    
    // Initialize symbol table
    obj.initSymbolTable();
    
    // Get non-existent symbol after initialization
    CHECK(obj.getSymbol(100) == nullptr);
    
    // Get non-existent symbol by name
    CHECK(obj.getSymbolIndex("nonexistent") == 0);
  }
  
  SECTION("Add symbols using convenience method") {
    obj.initSymbolTable();
    
    coil::u64 name = obj.addString("func1");
    
    // Add using the convenience method
    coil::u16 index = obj.addSymbol(
      name,                                           // name
      0x1000,                                         // value
      1,                                              // section_index
      static_cast<coil::u8>(coil::SymbolType::Func),  // type
      static_cast<coil::u8>(coil::SymbolBinding::Global) // binding
    );
    
    CHECK(index == 1);
    
    const coil::Symbol* sym = obj.getSymbol(index);
    REQUIRE(sym != nullptr);
    CHECK(sym->name == name);
    CHECK(sym->value == 0x1000);
  }
}

TEST_CASE("COIL Object Sections", "[obj]") {
  coil::Object obj = coil::Object::create();
  
  SECTION("Add and retrieve sections") {
    // Initialize string table for section names
    obj.initStringTable();
    
    // Add section names
    coil::u64 name1 = obj.addString(".text");
    coil::u64 name2 = obj.addString(".data");
    
    // Create section headers
    coil::SectionHeader header1 = {
      name1, 
      0, 
      static_cast<coil::u16>(coil::SectionFlag::Code | coil::SectionFlag::Alloc), 
      static_cast<coil::u8>(coil::SectionType::ProgBits)
    };
    
    coil::SectionHeader header2 = {
      name2, 
      0, 
      static_cast<coil::u16>(coil::SectionFlag::Write | coil::SectionFlag::Alloc), 
      static_cast<coil::u8>(coil::SectionType::ProgBits)
    };
    
    // Add sections
    coil::u16 index1 = obj.addSection(header1);
    coil::u16 index2 = obj.addSection(header2);
    
    // Indices should be valid (string table is 1)
    CHECK(index1 == 2);
    CHECK(index2 == 3);
    
    // Retrieve sections by index
    coil::BaseSection* section1 = obj.getSection(index1);
    coil::BaseSection* section2 = obj.getSection(index2);
    
    REQUIRE(section1 != nullptr);
    REQUIRE(section2 != nullptr);
    
    // Verify section properties
    CHECK(section1->getHeader().name == name1);
    CHECK(section1->getHeader().flags == header1.flags);
    CHECK(section1->getHeader().type == header1.type);
    
    CHECK(section2->getHeader().name == name2);
    CHECK(section2->getHeader().flags == header2.flags);
    CHECK(section2->getHeader().type == header2.type);
    
    // Retrieve section by name
    coil::u16 index1b = obj.getSectionIndex(".text");
    CHECK(index1b == index1);
  }
  
  SECTION("Section with data") {
    // Initialize string table for section names
    obj.initStringTable();
    
    // Add section name
    coil::u64 name = obj.addString(".data");
    
    // Create test data
    std::vector<coil::u8> data = {1, 2, 3, 4, 5, 6, 7, 8};
    
    // Add section with data
    coil::u16 index = obj.addSection(
      name, 
      static_cast<coil::u16>(coil::SectionFlag::Write), 
      static_cast<coil::u8>(coil::SectionType::ProgBits), 
      data.size(),
      data
    );
    
    // Index should be valid (string table is 1)
    CHECK(index == 2);
    
    // Retrieve section
    coil::BaseSection* section = obj.getSection(index);
    REQUIRE(section != nullptr);
    
    // Get section data
    REQUIRE(section->getSectionType() == static_cast<coil::u8>(coil::SectionType::ProgBits));
    auto* dataSection = dynamic_cast<coil::DataSection*>(section);
    REQUIRE(dataSection != nullptr);
    
    const auto& sectionData = dataSection->getData();
    REQUIRE(sectionData.size() == data.size());
    
    for (size_t i = 0; i < data.size(); i++) {
      CHECK(sectionData[i] == data[i]);
    }
  }
  
  SECTION("Invalid section operations") {
    // Get non-existent section
    CHECK(obj.getSection(1) == nullptr);
    
    // Initialize string table
    obj.initStringTable();
    
    // Get non-existent section after initialization
    CHECK(obj.getSection(100) == nullptr);
    
    // Get non-existent section by name
    CHECK(obj.getSectionIndex("nonexistent") == 0);
  }
}

TEST_CASE("COIL Object Save/Load", "[obj]") {
  SECTION("Save and load empty object") {
    coil::Object obj1 = coil::Object::create();
    
    // Create a memory stream for the object
    auto stream = createObjectStream(obj1);
    
    // Load into a new object
    coil::Object obj2;
    obj2.load(*stream);
    
    // Verify header
    CHECK(obj2.getHeader().magic[0] == 'C');
    CHECK(obj2.getHeader().magic[1] == 'O');
    CHECK(obj2.getHeader().magic[2] == 'I');
    CHECK(obj2.getHeader().magic[3] == 'L');
    CHECK(obj2.getHeader().version == coil::COIL_VERSION);
    CHECK(obj2.getHeader().section_count == 0);
  }
  
  SECTION("Save and load object with sections and symbols") {
    coil::Object obj1 = coil::Object::create();
    
    // Initialize tables
    obj1.initStringTable();
    obj1.initSymbolTable();
    
    // Add some strings
    coil::u64 strOffset1 = obj1.addString("test_string");
    coil::u64 strOffset2 = obj1.addString(".custom_section");
    coil::u64 strOffset3 = obj1.addString("symbol_name");
    
    // Add a custom section
    std::vector<coil::u8> sectionData = {0x10, 0x20, 0x30, 0x40, 0x50};
    coil::u16 sectionIndex = obj1.addSection(
      strOffset2,
      static_cast<coil::u16>(coil::SectionFlag::Write),
      static_cast<coil::u8>(coil::SectionType::ProgBits),
      sectionData.size(),
      sectionData
    );
    
    // Add a symbol
    coil::u16 symbolIndex = obj1.addSymbol(
      strOffset3,
      0x1234,
      sectionIndex,
      static_cast<coil::u8>(coil::SymbolType::Func),
      static_cast<coil::u8>(coil::SymbolBinding::Global)
    );
    
    // Create a memory stream for the object
    auto stream = createObjectStream(obj1);
    
    // Load into a new object
    coil::Object obj2;
    obj2.load(*stream);
    
    // Verify section count (string table + symbol table + custom section)
    CHECK(obj2.getSectionCount() == 3);
    
    // Verify string table content
    CHECK(obj2.getStringTable() != nullptr);
    CHECK(obj2.getString(strOffset1) == "test_string");
    CHECK(obj2.getString(strOffset2) == ".custom_section");
    CHECK(obj2.getString(strOffset3) == "symbol_name");
    
    // Verify custom section
    coil::u16 sectionIndex2 = obj2.getSectionIndex(".custom_section");
    CHECK(sectionIndex2 > 0);
    
    coil::BaseSection* section = obj2.getSection(sectionIndex2);
    REQUIRE(section != nullptr);
    
    // Check if it's a data section
    REQUIRE(section->getSectionType() == static_cast<coil::u8>(coil::SectionType::ProgBits));
    auto* dataSection = dynamic_cast<coil::DataSection*>(section);
    REQUIRE(dataSection != nullptr);
    
    const auto& loadedData = dataSection->getData();
    REQUIRE(loadedData.size() == sectionData.size());
    
    for (size_t i = 0; i < sectionData.size(); i++) {
      CHECK(loadedData[i] == sectionData[i]);
    }
    
    // Verify symbol
    coil::u16 symbolIndex2 = obj2.getSymbolIndex("symbol_name");
    CHECK(symbolIndex2 > 0);
    
    const coil::Symbol* symbol = obj2.getSymbol(symbolIndex2);
    REQUIRE(symbol != nullptr);
    
    CHECK(symbol->name == strOffset3);
    CHECK(symbol->value == 0x1234);
    CHECK(symbol->section_index == sectionIndex2);  // Section indices might differ after load
    CHECK(symbol->type == static_cast<coil::u8>(coil::SymbolType::Func));
    CHECK(symbol->binding == static_cast<coil::u8>(coil::SymbolBinding::Global));
  }
}

TEST_CASE("COIL Object Error Cases", "[obj]") {
  SECTION("Invalid header") {
    // Create a memory stream with corrupted data
    coil::MemoryStream stream(1024);
    
    // Write invalid magic
    const char badMagic[] = "BAAD";
    stream.write(badMagic, 4);
    
    // Fill rest of header with zeros
    std::vector<coil::u8> zeros(sizeof(coil::ObjectHeader) - 4, 0);
    stream.write(zeros.data(), zeros.size());
    
    // Reset position
    stream.seek(0);
    
    // Attempt to load should throw
    coil::Object obj;
    CHECK_THROWS_AS(obj.load(stream), coil::FormatException);
  }
  
  SECTION("Multiple string tables") {
    coil::Object obj = coil::Object::create();
    
    // Initialize string table
    obj.initStringTable();
    
    // Add a second string table header
    coil::SectionHeader header = {
      0,
      0,
      0,
      static_cast<coil::u8>(coil::SectionType::StrTab)
    };
    
    // Adding a second string table should throw
    CHECK_THROWS_AS(obj.addSection(header), coil::AlreadyExistsException);
  }
  
  SECTION("Multiple symbol tables") {
    coil::Object obj = coil::Object::create();
    
    // Initialize symbol table (creates string table automatically)
    obj.initSymbolTable();
    
    // Add a second symbol table header
    coil::u64 nameOffset = obj.addString(".symtab2");
    coil::SectionHeader header = {
      nameOffset,
      0,
      0,
      static_cast<coil::u8>(coil::SectionType::SymTab)
    };
    
    // Adding a second symbol table should throw
    CHECK_THROWS_AS(obj.addSection(header), coil::AlreadyExistsException);
  }
}

TEST_CASE("COIL Object Section Flags", "[obj]") {
  SECTION("Section flag operations") {
    // Test bitwise OR operator
    coil::SectionFlag flag1 = coil::SectionFlag::Write;
    coil::SectionFlag flag2 = coil::SectionFlag::Code;
    coil::SectionFlag combined = flag1 | flag2;
    
    CHECK((combined & coil::SectionFlag::Write) == true);
    CHECK((combined & coil::SectionFlag::Code) == true);
    CHECK((combined & coil::SectionFlag::Merge) == false);
  }
}

TEST_CASE("COIL Object File I/O", "[obj]") {
  const std::string filename = "test_object.coil";
  
  // Clean up any existing file
  std::filesystem::remove(filename);
  
  SECTION("Write and read object file") {
    // Create an object with some content
    coil::Object obj1 = coil::Object::create();
    obj1.initStringTable();
    obj1.initSymbolTable();
    
    // Add a section with data
    std::vector<coil::u8> data = {0x01, 0x02, 0x03, 0x04};
    coil::u64 nameOffset = obj1.addString(".data");
    obj1.addSection(nameOffset, 0, static_cast<coil::u8>(coil::SectionType::ProgBits), data.size(), data);
    
    // Write to file
    {
      coil::FileStream stream(filename, coil::StreamMode::Write);
      obj1.save(stream);
    }
    
    // Read back
    coil::Object obj2;
    {
      coil::FileStream stream(filename, coil::StreamMode::Read);
      obj2.load(stream);
    }
    
    // Verify object
    CHECK(obj2.getSectionCount() == 3); // string table, symbol table, data section
    
    // Verify data section
    coil::u16 sectionIndex = obj2.getSectionIndex(".data");
    CHECK(sectionIndex > 0);
    
    auto* section = dynamic_cast<coil::DataSection*>(obj2.getSection(sectionIndex));
    REQUIRE(section != nullptr);
    
    const auto& loadedData = section->getData();
    CHECK(loadedData.size() == data.size());
    
    for (size_t i = 0; i < data.size(); i++) {
      CHECK(loadedData[i] == data[i]);
    }
    
    // Clean up
    std::filesystem::remove(filename);
  }
}