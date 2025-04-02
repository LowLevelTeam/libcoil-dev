#include "test_helper.h"

bool testCoilHeaderEncoding() {
  // Create a default header
  coil::CoilHeader header = coil::CoilHeader::createDefault();
  
  // Modify some values
  header.major = 1;
  header.minor = 2;
  header.patch = 3;
  header.flags = coil::FormatFlags::OBJECT_FILE | coil::FormatFlags::DEBUG_INFO;
  header.symbol_offset = 100;
  header.section_offset = 200;
  header.reloc_offset = 300;
  header.debug_offset = 400;
  header.file_size = 1000;
  
  // Encode to binary
  std::vector<uint8_t> encoded = header.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::CoilHeader decoded = coil::CoilHeader::decode(encoded, offset);
  
  // Check values
  TEST_ASSERT(decoded.magic[0] == 'C' && decoded.magic[1] == 'O' && decoded.magic[2] == 'I' && decoded.magic[3] == 'L');
  TEST_ASSERT(decoded.major == 1);
  TEST_ASSERT(decoded.minor == 2);
  TEST_ASSERT(decoded.patch == 3);
  TEST_ASSERT(decoded.flags == (coil::FormatFlags::OBJECT_FILE | coil::FormatFlags::DEBUG_INFO));
  TEST_ASSERT(decoded.symbol_offset == 100);
  TEST_ASSERT(decoded.section_offset == 200);
  TEST_ASSERT(decoded.reloc_offset == 300);
  TEST_ASSERT(decoded.debug_offset == 400);
  TEST_ASSERT(decoded.file_size == 1000);
  
  return true;
}

bool testSymbolEncoding() {
  // Create a symbol
  coil::Symbol symbol;
  symbol.name = "test_symbol";
  symbol.name_length = static_cast<uint16_t>(symbol.name.length());
  symbol.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::FUNCTION;
  symbol.value = 0x1000;
  symbol.section_index = 1;
  symbol.processor_type = 2;
  
  // Encode to binary
  std::vector<uint8_t> encoded = symbol.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Symbol decoded = coil::Symbol::decode(encoded, offset);
  
  // Check values
  TEST_ASSERT(decoded.name == "test_symbol");
  TEST_ASSERT(decoded.name_length == symbol.name_length);
  TEST_ASSERT(decoded.attributes == (coil::SymbolFlags::GLOBAL | coil::SymbolFlags::FUNCTION));
  TEST_ASSERT(decoded.value == 0x1000);
  TEST_ASSERT(decoded.section_index == 1);
  TEST_ASSERT(decoded.processor_type == 2);
  
  return true;
}

bool testSectionEncoding() {
  // Create a section
  coil::Section section;
  section.name_index = 1;
  section.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
  section.offset = 100;
  section.size = 10;
  section.address = 0x10000;
  section.alignment = 16;
  section.processor_type = 1;
  section.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
  
  // Encode to binary
  std::vector<uint8_t> encoded = section.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Section decoded = coil::Section::decode(encoded, offset);
  
  // Check values
  TEST_ASSERT(decoded.name_index == 1);
  TEST_ASSERT(decoded.attributes == (coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE));
  TEST_ASSERT(decoded.offset == 100);
  TEST_ASSERT(decoded.size == 10);
  TEST_ASSERT(decoded.address == 0x10000);
  TEST_ASSERT(decoded.alignment == 16);
  TEST_ASSERT(decoded.processor_type == 1);
  TEST_ASSERT(decoded.data.size() == 10);
  TEST_ASSERT(std::equal(decoded.data.begin(), decoded.data.end(), section.data.begin()));
  
  return true;
}

bool testRelocationEncoding() {
  // Create a relocation
  coil::Relocation reloc;
  reloc.offset = 0x100;
  reloc.symbol_index = 1;
  reloc.section_index = 2;
  reloc.type = coil::RelocationType::ABSOLUTE;
  reloc.size = 4;
  
  // Encode to binary
  std::vector<uint8_t> encoded = reloc.encode();
  
  // Decode from binary
  size_t offset = 0;
  coil::Relocation decoded = coil::Relocation::decode(encoded, offset);
  
  // Check values
  TEST_ASSERT(decoded.offset == 0x100);
  TEST_ASSERT(decoded.symbol_index == 1);
  TEST_ASSERT(decoded.section_index == 2);
  TEST_ASSERT(decoded.type == coil::RelocationType::ABSOLUTE);
  TEST_ASSERT(decoded.size == 4);
  
  return true;
}

bool testCoilObjectEncodeAndDecode() {
  // Create a COIL object
  coil::CoilObject obj = createTestCoilObject();
  
  // Add some content to the sections
  std::vector<uint8_t> textData = {0x01, 0x02, 0x03, 0x04, 0x05};
  std::vector<uint8_t> dataData = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
  
  obj.updateSectionData(0, textData);
  obj.updateSectionData(1, dataData);
  
  // Add a relocation
  coil::Relocation reloc;
  reloc.offset = 2;
  reloc.symbol_index = 0;
  reloc.section_index = 0;
  reloc.type = coil::RelocationType::ABSOLUTE;
  reloc.size = 4;
  
  obj.addRelocation(reloc);
  
  // Encode to binary
  std::vector<uint8_t> encoded = obj.encode();
  
  // Decode from binary
  coil::CoilObject decoded = coil::CoilObject::decode(encoded);
  
  // Check object properties
  TEST_ASSERT(decoded.getSymbolCount() == obj.getSymbolCount());
  TEST_ASSERT(decoded.getSectionCount() == obj.getSectionCount());
  TEST_ASSERT(decoded.getRelocationCount() == obj.getRelocationCount());
  
  // Check section data
  TEST_ASSERT(decoded.getSection(0).data.size() == 5);
  TEST_ASSERT(std::equal(decoded.getSection(0).data.begin(), decoded.getSection(0).data.end(), textData.begin()));
  
  TEST_ASSERT(decoded.getSection(1).data.size() == 5);
  TEST_ASSERT(std::equal(decoded.getSection(1).data.begin(), decoded.getSection(1).data.end(), dataData.begin()));
  
  // Check symbol properties
  TEST_ASSERT(decoded.getSymbol(0).name == ".text");
  TEST_ASSERT(decoded.getSymbol(1).name == ".data");
  
  // Check relocation
  TEST_ASSERT(decoded.getRelocation(0).offset == 2);
  TEST_ASSERT(decoded.getRelocation(0).symbol_index == 0);
  TEST_ASSERT(decoded.getRelocation(0).section_index == 0);
  TEST_ASSERT(decoded.getRelocation(0).type == coil::RelocationType::ABSOLUTE);
  TEST_ASSERT(decoded.getRelocation(0).size == 4);
  
  return true;
}

bool testObjectModification() {
  // Create a COIL object
  coil::CoilObject obj = createTestCoilObject();
  
  // Initial object properties
  size_t initialSymbolCount = obj.getSymbolCount();
  size_t initialSectionCount = obj.getSectionCount();
  
  // Add a new symbol
  coil::Symbol newSymbol;
  newSymbol.name = "new_symbol";
  newSymbol.name_length = static_cast<uint16_t>(newSymbol.name.length());
  newSymbol.attributes = coil::SymbolFlags::GLOBAL;
  newSymbol.value = 0x2000;
  newSymbol.section_index = 0;
  newSymbol.processor_type = 0;
  
  uint16_t newSymbolIndex = obj.addSymbol(newSymbol);
  
  // Add a new section
  coil::Symbol bssSection;
  bssSection.name = ".bss";
  bssSection.name_length = static_cast<uint16_t>(bssSection.name.length());
  bssSection.attributes = coil::SymbolFlags::LOCAL;
  bssSection.value = 0;
  bssSection.section_index = 0;
  bssSection.processor_type = 0;
  
  uint16_t bssSectionSymIndex = obj.addSymbol(bssSection);
  
  coil::Section bssSect;
  bssSect.name_index = bssSectionSymIndex;
  bssSect.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::UNINITIALIZED;
  bssSect.offset = 0;
  bssSect.size = 0;
  bssSect.address = 0;
  bssSect.alignment = 8;
  bssSect.processor_type = 0;
  
  uint16_t bssSectIndex = obj.addSection(bssSect);
  obj.setSymbolSectionIndex(bssSectionSymIndex, bssSectIndex);
  
  // Check updated counts
  TEST_ASSERT(obj.getSymbolCount() == initialSymbolCount + 2);
  TEST_ASSERT(obj.getSectionCount() == initialSectionCount + 1);
  
  // Check finding symbols by name
  TEST_ASSERT(obj.findSymbol("new_symbol") == newSymbolIndex);
  TEST_ASSERT(obj.findSymbol(".bss") == bssSectionSymIndex);
  TEST_ASSERT(obj.findSymbol("nonexistent") == UINT16_MAX);
  
  // Update a symbol
  coil::Symbol updatedSymbol = obj.getSymbol(newSymbolIndex);
  updatedSymbol.value = 0x3000;
  obj.updateSymbol(newSymbolIndex, updatedSymbol);
  
  TEST_ASSERT(obj.getSymbol(newSymbolIndex).value == 0x3000);
  
  return true;
}

bool testInstructionAddition() {
  // Create a COIL object
  coil::CoilObject obj = createTestCoilObject();
  
  // Create an instruction: MOV #1, #2
  std::vector<coil::Operand> operands = {
      coil::Operand::createVariable(1),
      coil::Operand::createVariable(2)
  };
  coil::Instruction instruction(coil::Opcode::MOV, operands);
  
  // Add instruction to the section
  obj.addInstruction(0, instruction);
  
  // Check if the section data is updated correctly
  const auto& section = obj.getSection(0);
  
  // The instruction encoding should be at least the opcode + operand count + operand data
  TEST_ASSERT(section.data.size() > 0);
  TEST_ASSERT(section.data[0] == coil::Opcode::MOV);
  TEST_ASSERT(section.data[1] == 2);  // 2 operands
  
  return true;
}

int main() {
  std::vector<std::pair<std::string, std::function<bool()>>> tests = {
      {"CoilHeader Encoding/Decoding", testCoilHeaderEncoding},
      {"Symbol Encoding/Decoding", testSymbolEncoding},
      {"Section Encoding/Decoding", testSectionEncoding},
      {"Relocation Encoding/Decoding", testRelocationEncoding},
      {"CoilObject Encode/Decode", testCoilObjectEncodeAndDecode},
      {"Object Modification", testObjectModification},
      {"Instruction Addition", testInstructionAddition}
  };
  
  return runTests(tests);
}