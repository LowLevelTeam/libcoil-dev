/**
* @file test_obj.cpp
* @brief Tests for the COIL object format
*/

#include <catch2/catch_test_macros.hpp>
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include <cstring>
#include <vector>
#include <string>

TEST_CASE("Object file creation and manipulation", "[obj]") {
  SECTION("Creating an empty object") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Check initial state
      CHECK(obj.getType() == coil::ObjType::Relocatable);
      CHECK(obj.getSectionCount() == 0);
      CHECK(obj.getSection((coil::u16)0) == nullptr);
      CHECK(obj.getSection(".text") == nullptr);
  }
  
  SECTION("Adding sections") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Sample code section
      const uint8_t code[] = {0x01, 0x02, 0x03, 0x04};
      coil::Result result = obj.addSection(".text", coil::SectionType::Code, 
                                          coil::SectionFlag::Executable, 
                                          code, sizeof(code));
      
      // Check result
      CHECK(result == coil::Result::Success);
      CHECK(obj.getSectionCount() == 1);
      
      // Verify section
      const coil::Section* section = obj.getSection(".text");
      REQUIRE(section != nullptr);
      CHECK(section->header.type == static_cast<uint32_t>(coil::SectionType::Code));
      CHECK(section->header.flags == static_cast<uint32_t>(coil::SectionFlag::Executable));
      CHECK(section->header.size == sizeof(code));
      CHECK(section->data == code);
      CHECK(std::string(section->name) == ".text");
      
      // Add a data section
      const uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
      result = obj.addSection(".data", coil::SectionType::Data, 
                              coil::SectionFlag::Writable | coil::SectionFlag::Allocate, 
                              data, sizeof(data));
      
      // Check result
      CHECK(result == coil::Result::Success);
      CHECK(obj.getSectionCount() == 2);
      
      // Verify section
      section = obj.getSection(".data");
      REQUIRE(section != nullptr);
      CHECK(section->header.type == static_cast<uint32_t>(coil::SectionType::Data));
      CHECK(section->header.flags == static_cast<uint32_t>(
                  coil::SectionFlag::Writable | coil::SectionFlag::Allocate));
      CHECK(section->header.size == sizeof(data));
      CHECK(section->data == data);
      CHECK(std::string(section->name) == ".data");
      
      // Get section by index
      section = obj.getSection(1);
      REQUIRE(section != nullptr);
      CHECK(std::string(section->name) == ".data");
  }
  
  SECTION("Section limit") {
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add maximum number of sections
      const uint8_t dummy = 0;
      bool success = true;
      
      for (size_t i = 0; i < 16; i++) {
          char name[32];
          snprintf(name, sizeof(name), ".section%zu", i);
          
          coil::Result result = obj.addSection(name, coil::SectionType::Data, 
                                              coil::SectionFlag::None, &dummy, 1);
          
          if (result != coil::Result::Success) {
              success = false;
              break;
          }
      }
      
      // Should have added all 16 sections
      CHECK(success);
      CHECK(obj.getSectionCount() == 16);
      
      // Adding one more should fail
      coil::Result result = obj.addSection(".one_too_many", coil::SectionType::Data, 
                                          coil::SectionFlag::None, &dummy, 1);
      CHECK(result != coil::Result::Success);
  }
}

TEST_CASE("Object file saving and loading", "[obj]") {
  // Create a temporary memory stream
  coil::MemoryStream stream(nullptr, 4096, coil::StreamMode::ReadWrite);
  
  SECTION("Save and load an object file") {
      // Create an object with sections
      coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
      
      // Add a code section
      const uint8_t code[] = {0x01, 0x02, 0x03, 0x04};
      obj.addSection(".text", coil::SectionType::Code, 
                    coil::SectionFlag::Executable, code, sizeof(code));
      
      // Add a data section
      const uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
      obj.addSection(".data", coil::SectionType::Data, 
                    coil::SectionFlag::Writable | coil::SectionFlag::Allocate, 
                    data, sizeof(data));
      
      // Save the object to the stream
      CHECK(obj.save(stream) == coil::Result::Success);
      
      // Reset stream position
      CHECK(stream.seek(0) == coil::Result::Success);
      
      // Load into a new object
      coil::Object loaded_obj;
      CHECK(coil::Object::load(stream, loaded_obj) == coil::Result::Success);
      
      // Verify the loaded object
      CHECK(loaded_obj.getType() == coil::ObjType::Relocatable);
      CHECK(loaded_obj.getSectionCount() == 2);
      
      // Verify sections
      const coil::Section* section = loaded_obj.getSection(".text");
      REQUIRE(section != nullptr);
      CHECK(section->header.type == static_cast<uint32_t>(coil::SectionType::Code));
      CHECK(section->header.flags == static_cast<uint32_t>(coil::SectionFlag::Executable));
      CHECK(section->header.size == sizeof(code));
      
      section = loaded_obj.getSection(".data");
      REQUIRE(section != nullptr);
      CHECK(section->header.type == static_cast<uint32_t>(coil::SectionType::Data));
      CHECK(section->header.flags == static_cast<uint32_t>(
                coil::SectionFlag::Writable | coil::SectionFlag::Allocate));
      CHECK(section->header.size == sizeof(data));
  }
}

TEST_CASE("Object file flag operations", "[obj]") {
  // Test the flag operators
  coil::SectionFlag flag1 = coil::SectionFlag::Writable;
  coil::SectionFlag flag2 = coil::SectionFlag::Executable;
  
  // OR operator
  coil::SectionFlag combined = flag1 | flag2;
  CHECK((combined & flag1) == true);
  CHECK((combined & flag2) == true);
  CHECK((combined & coil::SectionFlag::Allocate) == false);
  
  // Complex combination
  coil::SectionFlag all = coil::SectionFlag::Writable | 
                          coil::SectionFlag::Allocate | 
                          coil::SectionFlag::Executable | 
                          coil::SectionFlag::Strings;
  
  CHECK((all & coil::SectionFlag::Writable) == true);
  CHECK((all & coil::SectionFlag::Allocate) == true);
  CHECK((all & coil::SectionFlag::Executable) == true);
  CHECK((all & coil::SectionFlag::Strings) == true);
  
  // None flag
  CHECK((coil::SectionFlag::None & coil::SectionFlag::Writable) == false);
}