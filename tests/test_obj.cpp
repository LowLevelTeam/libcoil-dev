/**
* @file test_obj.cpp
* @brief Tests for the COIL optimized object format
*/

#include <catch2/catch_test_macros.hpp>
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include "coil/err.hpp"
#include <vector>
#include <string>

void streamTestErrorCallback(coil::ErrorLevel level, const char* message, 
  const coil::ErrorPosition* position, void* user_data) {
  printf("Error '%s'\n", message);
}

TEST_CASE("COIL Object Creation and Basic Operations", "[obj]") {
  SECTION("Creating an empty object") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Check initial state
      CHECK(obj.getType() == coil::ObjType::Relocatable);
      CHECK(obj.getSectionCount() == 0);
      CHECK(obj.getSection(".text") == nullptr);
  }
  
  SECTION("Adding sections") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add a code section
      CHECK(obj.addSection(".text", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Exec | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Verify section exists
      coil::Section* section = obj.getSection(".text");
      REQUIRE(section != nullptr);
      CHECK(section->getHeader().type == static_cast<coil::u8>(coil::SectionType::ProgBits));
      CHECK((section->getHeader().flags & static_cast<coil::u16>(coil::SectionFlag::Exec)) != 0);
      CHECK((section->getHeader().flags & static_cast<coil::u16>(coil::SectionFlag::Alloc)) != 0);
      
      // Add a data section
      CHECK(obj.addSection(".data", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Write | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Verify section count
      CHECK(obj.getSectionCount() == 2);
      
      // Get section by index
      coil::Section* section_by_index = obj.getSection(1);
      REQUIRE(section_by_index != nullptr);
      CHECK(std::string(section_by_index->getName()) == ".data");
  }
  
  SECTION("Setting section data") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add a code section
      CHECK(obj.addSection(".text", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Exec | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Set section data
      const coil::u8 code[] = {0x01, 0x02, 0x03, 0x04};
      CHECK(obj.setSectionData(".text", code, sizeof(code)) == coil::Result::Success);
      
      // Verify data
      coil::Section* section = obj.getSection(".text");
      REQUIRE(section != nullptr);
      CHECK(section->getData().size() == sizeof(code));
      CHECK(section->getHeader().size == sizeof(code));
      
      // Verify data contents
      const std::vector<coil::u8>& data = section->getData();
      for (size_t i = 0; i < sizeof(code); i++) {
          CHECK(data[i] == code[i]);
      }
  }
  
  SECTION("Adding symbols") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add a code section
      CHECK(obj.addSection(".text", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Exec | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Add a function symbol
      CHECK(obj.addSymbol("main", 0, 10, coil::SymbolType::Func, 
                        coil::SymbolBinding::Global, 0) == coil::Result::Success);
      
      // Find the symbol
      const coil::Symbol* sym = obj.findSymbol("main");
      REQUIRE(sym != nullptr);
      CHECK(sym->value == 0);
      CHECK(sym->size == 10);
      CHECK(sym->getType() == coil::SymbolType::Func);
      CHECK(sym->getBinding() == coil::SymbolBinding::Global);
  }
  
  SECTION("String table functionality") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add some strings
      coil::u32 offset1 = obj.addString("first_string");
      coil::u32 offset2 = obj.addString("second_string");
      coil::u32 offset3 = obj.addString("first_string"); // Duplicate should return same offset
      
      // Verify offsets
      CHECK(offset1 > 0); // 0 is reserved for empty string
      CHECK(offset2 > offset1);
      CHECK(offset3 == offset1); // Should reuse existing string
      
      // Verify string retrieval
      CHECK(std::string(obj.getString(offset1)) == "first_string");
      CHECK(std::string(obj.getString(offset2)) == "second_string");
      CHECK(std::string(obj.getString(0)) == ""); // Empty string at offset 0
  }
  
  SECTION("Adding relocations") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add sections
      CHECK(obj.addSection(".text", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Exec | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      CHECK(obj.addSection(".data", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Write | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Add symbols
      CHECK(obj.addSymbol("func1", 0, 20, coil::SymbolType::Func, 
                        coil::SymbolBinding::Global, 0) == coil::Result::Success);
      
      CHECK(obj.addSymbol("data1", 0, 8, coil::SymbolType::Object, 
                        coil::SymbolBinding::Global, 1) == coil::Result::Success);
      
      // Add relocations
      CHECK(obj.addRelocation(".text", 4, "data1", 
                            coil::RelocationType::Abs32) == coil::Result::Success);
      
      // Verify a relocation section was created
      coil::Section* reltab = obj.getSection(".reltext");
      REQUIRE(reltab != nullptr);
      CHECK(reltab->getHeader().type == static_cast<coil::u8>(coil::SectionType::RelTab));
      CHECK(reltab->getHeader().entry_size == sizeof(coil::Relocation));
      CHECK(reltab->getData().size() == sizeof(coil::Relocation));
  }
}

TEST_CASE("COIL Object Saving and Loading", "[obj]") {
  coil::setErrorCallback(streamTestErrorCallback, nullptr);
  coil::MemoryStream stream(nullptr, 4096, coil::StreamMode::ReadWrite);

  
  SECTION("Save and load basic object") {
      // Create object with sections, symbols, and relocations
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add sections
      CHECK(obj.addSection(".text", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Exec | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      CHECK(obj.addSection(".data", coil::SectionType::ProgBits, 
                          coil::SectionFlag::Write | coil::SectionFlag::Alloc) == coil::Result::Success);
      
      // Set section data
      const coil::u8 code[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
      CHECK(obj.setSectionData(".text", code, sizeof(code)) == coil::Result::Success);
      
      const coil::u8 data[] = {0x10, 0x20, 0x30, 0x40};
      CHECK(obj.setSectionData(".data", data, sizeof(data)) == coil::Result::Success);
      
      // Add symbols
      CHECK(obj.addSymbol("main", 0, 8, coil::SymbolType::Func, 
                        coil::SymbolBinding::Global, 0) == coil::Result::Success);
      
      CHECK(obj.addSymbol("global_var", 0, 4, coil::SymbolType::Object, 
                        coil::SymbolBinding::Global, 1) == coil::Result::Success);
      
      // Add a relocation
      CHECK(obj.addRelocation(".text", 4, "global_var", 
                            coil::RelocationType::Abs32) == coil::Result::Success);
      
      // Save the object
      CHECK(obj.save(stream) == coil::Result::Success);
      
      // Prepare for loading
      CHECK(stream.seek(0) == coil::Result::Success);
      
      // Load the object
      coil::Object loaded_obj;
      CHECK(coil::Object::load(stream, loaded_obj) == coil::Result::Success);
      
      // Verify type and section count
      CHECK(loaded_obj.getType() == coil::ObjType::Relocatable);
      CHECK(loaded_obj.getSectionCount() == 4); // .text, .data, .symtab, .reltext
      
      // Verify sections
      coil::Section* text_section = loaded_obj.getSection(".text");
      REQUIRE(text_section != nullptr);
      CHECK(text_section->getData().size() == sizeof(code));
      
      coil::Section* data_section = loaded_obj.getSection(".data");
      REQUIRE(data_section != nullptr);
      CHECK(data_section->getData().size() == sizeof(data));
      
      // Verify symbols
      const coil::Symbol* main_sym = loaded_obj.findSymbol("main");
      REQUIRE(main_sym != nullptr);
      CHECK(main_sym->value == 0);
      CHECK(main_sym->size == 8);
      CHECK(main_sym->getType() == coil::SymbolType::Func);
      
      const coil::Symbol* var_sym = loaded_obj.findSymbol("global_var");
      REQUIRE(var_sym != nullptr);
      CHECK(var_sym->value == 0);
      CHECK(var_sym->size == 4);
      CHECK(var_sym->getType() == coil::SymbolType::Object);
  }
  /*
  SECTION("Save and load with large string table") {
      coil::Object obj = coil::Object::create(coil::ObjType::Shared);
      
      // Add many strings to stress string table
      std::vector<std::string> strings;
      for (int i = 0; i < 100; i++) {
          char buffer[64];
          std::snprintf(buffer, sizeof(buffer), "symbol%d_with_very_long_name", i);
          strings.push_back(buffer);
          
          // Add a symbol with this name
          CHECK(obj.addSymbol(buffer, i * 8, 8, coil::SymbolType::Object, 
                            coil::SymbolBinding::Local, 0) == coil::Result::Success);
      }
      
      // Save the object
      CHECK(obj.save(stream) == coil::Result::Success);
      
      // Prepare for loading
      CHECK(stream.seek(0) == coil::Result::Success);
      
      // Load the object
      coil::Object loaded_obj;
      CHECK(coil::Object::load(stream, loaded_obj) == coil::Result::Success);
      
      // Verify all symbols can be found
      for (const std::string& name : strings) {
          const coil::Symbol* sym = loaded_obj.findSymbol(name.c_str());
          REQUIRE(sym != nullptr);
          CHECK(sym->getBinding() == coil::SymbolBinding::Local);
      }
  }
  */
}

TEST_CASE("COIL Object Flag Operations", "[obj]") {
  // Test flag bitwise operations
  coil::SectionFlag flag1 = coil::SectionFlag::Write;
  coil::SectionFlag flag2 = coil::SectionFlag::Exec;
  
  coil::SectionFlag combined = flag1 | flag2;
  CHECK((combined & coil::SectionFlag::Write) == true);
  CHECK((combined & coil::SectionFlag::Exec) == true);
  CHECK((combined & coil::SectionFlag::Alloc) == false);
  
  // Test multiple flags
  coil::SectionFlag all = coil::SectionFlag::Write | 
                        coil::SectionFlag::Alloc | 
                        coil::SectionFlag::Exec | 
                        coil::SectionFlag::Merge;
  
  CHECK((all & coil::SectionFlag::Write) == true);
  CHECK((all & coil::SectionFlag::Alloc) == true);
  CHECK((all & coil::SectionFlag::Exec) == true);
  CHECK((all & coil::SectionFlag::Merge) == true);
  CHECK((all & coil::SectionFlag::Strings) == false);
}