#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

#include "coil/binary_format.h"
#include "coil/type_system.h"
#include "coil/instruction_set.h"
#include "coil/variable_system.h"
#include "coil/error_codes.h"
#include "coil/utils/validation.h"

// Helper function to print binary data for debugging
void printBinaryData(const std::vector<uint8_t>& data, size_t start = 0, size_t count = (size_t)-1) {
    if (count == (size_t)-1) count = data.size();
    count = std::min(count, data.size() - start);
    
    std::cout << "Binary data (" << count << " bytes):" << std::endl;
    for (size_t i = 0; i < count; i += 16) {
        std::cout << std::setw(4) << std::setfill('0') << std::hex << i + start << ": ";
        
        // Print hex values
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i + j + start]) << " ";
            if (j == 7) std::cout << " "; // Extra space in the middle
        }
        
        // Fill remaining space if less than 16 bytes
        size_t remaining = (i + 16 <= count) ? 0 : 16 - (count - i);
        for (size_t j = 0; j < remaining; j++) {
            std::cout << "   ";
            if (j == 7) std::cout << " "; // Extra space in the middle
        }
        
        std::cout << "  ";
        
        // Print ASCII representation
        for (size_t j = 0; j < 16 && i + j < count; j++) {
            char c = data[i + j + start];
            std::cout << (isprint(c) ? c : '.');
        }
        
        std::cout << std::endl;
    }
    std::cout << std::dec; // Reset to decimal
}

// Helper function to write binary data to a file
void writeBinaryFile(const std::string& filename, const std::vector<uint8_t>& data) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
      std::cerr << "Error opening file for writing: " << filename << std::endl;
      return;
  }
  
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
  
  if (!file) {
      std::cerr << "Error writing to file: " << filename << std::endl;
  }
}

// Helper function to read binary data from a file
std::vector<uint8_t> readBinaryFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
      std::cerr << "Error opening file for reading: " << filename << std::endl;
      return {};
  }
  
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  std::vector<uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
      std::cerr << "Error reading from file: " << filename << std::endl;
      return {};
  }
  
  return buffer;
}

int main() {
  // Create a COIL object
  coil::CoilObject obj;
  
  // Create some symbols
  coil::Symbol textSection;
  textSection.name = ".text";
  textSection.name_length = static_cast<uint16_t>(textSection.name.length());
  textSection.attributes = coil::SymbolFlags::LOCAL;
  textSection.value = 0;
  textSection.section_index = 0; // Will be updated
  textSection.processor_type = 0;
  
  coil::Symbol dataSection;
  dataSection.name = ".data";
  dataSection.name_length = static_cast<uint16_t>(dataSection.name.length());
  dataSection.attributes = coil::SymbolFlags::LOCAL;
  dataSection.value = 0;
  dataSection.section_index = 0; // Will be updated
  dataSection.processor_type = 0;
  
  coil::Symbol mainSymbol;
  mainSymbol.name = "main";
  mainSymbol.name_length = static_cast<uint16_t>(mainSymbol.name.length());
  mainSymbol.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::FUNCTION;
  mainSymbol.value = 0;
  mainSymbol.section_index = 0; // Will be updated
  mainSymbol.processor_type = 0;
  
  // Add symbols to the object
  uint16_t textSectionSymIndex = obj.addSymbol(textSection);
  uint16_t dataSectionSymIndex = obj.addSymbol(dataSection);
  uint16_t mainSymIndex = obj.addSymbol(mainSymbol);
  
  // Create sections
  coil::Section textSect;
  textSect.name_index = textSectionSymIndex;
  textSect.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
  textSect.offset = 0;
  textSect.size = 0;
  textSect.address = 0;
  textSect.alignment = 16;
  textSect.processor_type = 0;
  
  coil::Section dataSect;
  dataSect.name_index = dataSectionSymIndex;
  dataSect.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::INITIALIZED;
  dataSect.offset = 0;
  dataSect.size = 0;
  dataSect.address = 0;
  dataSect.alignment = 8;
  dataSect.processor_type = 0;
  
  // Add sections to the object
  uint16_t textSectIndex = obj.addSection(textSect);
  uint16_t dataSectIndex = obj.addSection(dataSect);
  
  // Update symbols with correct section indices
  obj.setSymbolSectionIndex(textSectionSymIndex, textSectIndex);
  obj.setSymbolSectionIndex(dataSectionSymIndex, dataSectIndex);
  obj.setSymbolSectionIndex(mainSymIndex, textSectIndex);
  
  // Create some instructions for the text section
  // Main function with a simple variable declaration and arithmetic
  
  // First instruction: PROC 0x01 (CPU)
  std::vector<coil::Operand> procOperands = { coil::Operand::template createImmediate<int8_t>(1) };
  coil::Instruction procInstr(coil::Opcode::PROC, procOperands);
  
  // Enter scope
  coil::Instruction scopeEnterInstr(coil::Opcode::SCOPEE, {});
  
  // Create a variable: VAR #1, TYPE_INT32, 10
  std::vector<coil::Operand> varOperands = {
      coil::Operand::createVariable(1),
      coil::Operand::createImmediate<uint16_t>(coil::Type::INT32),
      coil::Operand::createImmediate<int32_t>(10)
  };
  coil::Instruction varInstr(coil::Opcode::VAR, varOperands);
  
  // Increment the variable: INC #1
  std::vector<coil::Operand> incOperands = { coil::Operand::createVariable(1) };
  coil::Instruction incInstr(coil::Opcode::INC, incOperands);
  
  // Leave scope
  coil::Instruction scopeLeaveInstr(coil::Opcode::SCOPEL, {});
  
  // Return: RET
  coil::Instruction retInstr(coil::Opcode::RET, {});
  
  // Clear the text section data
  obj.clearSectionData(textSectIndex);
  
  // Add instructions to the text section
  obj.addInstruction(textSectIndex, procInstr);
  obj.addInstruction(textSectIndex, scopeEnterInstr);
  obj.addInstruction(textSectIndex, varInstr);
  obj.addInstruction(textSectIndex, incInstr);
  obj.addInstruction(textSectIndex, scopeLeaveInstr);
  obj.addInstruction(textSectIndex, retInstr);
  
  // Print entire text section data for debugging
  std::cout << "Text section binary data:" << std::endl;
  printBinaryData(obj.getSection(textSectIndex).data);
  
  // Add some data to the data section
  // Simple null-terminated string "Hello, COIL!"
  std::string helloString = "Hello, COIL!";
  dataSect.data.insert(dataSect.data.end(), helloString.begin(), helloString.end());
  dataSect.data.push_back(0); // Null terminator
  dataSect.size = static_cast<uint32_t>(dataSect.data.size());
  
  // Update the data section
  obj.updateSection(dataSectIndex, dataSect);

  // Print entire text section data for debugging
  std::cout << "Data section binary data:" << std::endl;
  printBinaryData(obj.getSection(dataSectIndex).data);
  
  // Create an error manager for validation
  coil::ErrorManager errorManager;
  
  // Validate the COIL object
  bool isValid = coil::utils::Validation::validateCoilObject(obj, errorManager);
  
  if (!isValid) {
      std::cerr << "COIL object validation failed:" << std::endl;
      for (const auto& error : errorManager.getErrors()) {
          std::cerr << error.toString() << std::endl;
      }
      return 1;
  }
  
  // Encode the object to binary
  std::vector<uint8_t> binary = obj.encode();
  
  // Write the binary to a file
  std::string outputFile = "test.coil";
  writeBinaryFile(outputFile, binary);
  
  std::cout << "COIL object successfully created and written to " << outputFile << std::endl;
  std::cout << "Binary size: " << binary.size() << " bytes" << std::endl;
  std::cout << "Text section size: " << obj.getSection(textSectIndex).size << " bytes" << std::endl;
  std::cout << "Data section size: " << obj.getSection(dataSectIndex).size << " bytes" << std::endl;
  
  // Read the binary back and decode it
  std::vector<uint8_t> readBinary = readBinaryFile(outputFile);
  
  if (readBinary.empty()) {
      std::cerr << "Failed to read binary file" << std::endl;
      return 1;
  }
  
  try {
      coil::CoilObject decodedObj = coil::CoilObject::decode(readBinary);
      
      std::cout << "\nSuccessfully decoded COIL object:" << std::endl;
      std::cout << "Number of symbols: " << decodedObj.getSymbolCount() << std::endl;
      std::cout << "Number of sections: " << decodedObj.getSectionCount() << std::endl;
      
      // Print symbol information
      std::cout << "\nSymbols:" << std::endl;
      for (uint16_t i = 0; i < decodedObj.getSymbolCount(); ++i) {
          const auto& sym = decodedObj.getSymbol(i);
          std::cout << "  " << i << ": " << sym.name << " (section: " << sym.section_index << ")" << std::endl;
      }
      
      // Print section information
      std::cout << "\nSections:" << std::endl;
      for (uint16_t i = 0; i < decodedObj.getSectionCount(); ++i) {
          const auto& sect = decodedObj.getSection(i);
          std::cout << "  " << i << ": " << decodedObj.getSymbol(sect.name_index).name 
                    << " (size: " << sect.size << " bytes)" << std::endl;
      }
      
      // Validate the decoded object
      coil::ErrorManager decodeErrorManager;
      bool decodedValid = coil::utils::Validation::validateCoilObject(decodedObj, decodeErrorManager);
      
      if (!decodedValid) {
          std::cerr << "\nDecoded COIL object validation failed:" << std::endl;
          for (const auto& error : decodeErrorManager.getErrors()) {
              std::cerr << error.toString() << std::endl;
          }
          return 1;
      }
      
      std::cout << "\nDecoded COIL object validation passed!" << std::endl;
      
  } catch (const std::exception& e) {
      std::cerr << "Error decoding COIL object: " << e.what() << std::endl;
      return 1;
  }
  
  return 0;
}