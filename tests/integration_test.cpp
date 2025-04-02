#include "test_helper.h"

// Integration test: Create a complex COIL object, add sections, symbols, 
// instructions, encode it, validate it, and decode it
bool testCompleteWorkflow() {
    // Create a COIL object
    coil::CoilObject obj;
    
    // Create standard section symbols
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
    
    coil::Symbol bssSection;
    bssSection.name = ".bss";
    bssSection.name_length = static_cast<uint16_t>(bssSection.name.length());
    bssSection.attributes = coil::SymbolFlags::LOCAL;
    bssSection.value = 0;
    bssSection.section_index = 0; // Will be updated
    bssSection.processor_type = 0;
    
    // Create function symbols
    coil::Symbol mainSymbol;
    mainSymbol.name = "main";
    mainSymbol.name_length = static_cast<uint16_t>(mainSymbol.name.length());
    mainSymbol.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::FUNCTION;
    mainSymbol.value = 0; // Will be updated
    mainSymbol.section_index = 0; // Will be updated
    mainSymbol.processor_type = 0;
    
    coil::Symbol helperSymbol;
    helperSymbol.name = "helper";
    helperSymbol.name_length = static_cast<uint16_t>(helperSymbol.name.length());
    helperSymbol.attributes = coil::SymbolFlags::LOCAL | coil::SymbolFlags::FUNCTION;
    helperSymbol.value = 0; // Will be updated
    helperSymbol.section_index = 0; // Will be updated
    helperSymbol.processor_type = 0;
    
    // Create data symbols
    coil::Symbol messageSymbol;
    messageSymbol.name = "message";
    messageSymbol.name_length = static_cast<uint16_t>(messageSymbol.name.length());
    messageSymbol.attributes = coil::SymbolFlags::GLOBAL | coil::SymbolFlags::DATA;
    messageSymbol.value = 0; // Will be updated
    messageSymbol.section_index = 0; // Will be updated
    messageSymbol.processor_type = 0;
    
    // Add symbols to the object
    uint16_t textSectionSymIndex = obj.addSymbol(textSection);
    uint16_t dataSectionSymIndex = obj.addSymbol(dataSection);
    uint16_t bssSectionSymIndex = obj.addSymbol(bssSection);
    uint16_t mainSymIndex = obj.addSymbol(mainSymbol);
    uint16_t helperSymIndex = obj.addSymbol(helperSymbol);
    uint16_t messageSymIndex = obj.addSymbol(messageSymbol);
    
    // Create sections
    coil::Section textSect;
    textSect.name_index = textSectionSymIndex;
    textSect.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
    textSect.offset = 0;
    textSect.size = 0;
    textSect.address = 0x1000;
    textSect.alignment = 16;
    textSect.processor_type = 0;
    
    coil::Section dataSect;
    dataSect.name_index = dataSectionSymIndex;
    dataSect.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::INITIALIZED;
    dataSect.offset = 0;
    dataSect.size = 0;
    dataSect.address = 0x2000;
    dataSect.alignment = 8;
    dataSect.processor_type = 0;
    
    coil::Section bssSect;
    bssSect.name_index = bssSectionSymIndex;
    bssSect.attributes = coil::SectionFlags::READABLE | coil::SectionFlags::WRITABLE | coil::SectionFlags::UNINITIALIZED;
    bssSect.offset = 0;
    bssSect.size = 0;
    bssSect.address = 0x3000;
    bssSect.alignment = 8;
    bssSect.processor_type = 0;
    
    // Add sections to the object
    uint16_t textSectIndex = obj.addSection(textSect);
    uint16_t dataSectIndex = obj.addSection(dataSect);
    uint16_t bssSectIndex = obj.addSection(bssSect);
    
    // Update symbols with correct section indices
    obj.setSymbolSectionIndex(textSectionSymIndex, textSectIndex);
    obj.setSymbolSectionIndex(dataSectionSymIndex, dataSectIndex);
    obj.setSymbolSectionIndex(bssSectionSymIndex, bssSectIndex);
    obj.setSymbolSectionIndex(mainSymIndex, textSectIndex);
    obj.setSymbolSectionIndex(helperSymIndex, textSectIndex);
    obj.setSymbolSectionIndex(messageSymIndex, dataSectIndex);
    
    // Add data to the data section
    std::string messageString = "Hello, COIL!\0";
    std::vector<uint8_t> messageData(messageString.begin(), messageString.end());
    messageData.push_back(0);  // Null terminator
    obj.updateSectionData(dataSectIndex, messageData);
    
    // Update the message symbol with the correct offset
    coil::Symbol updatedMessageSymbol = obj.getSymbol(messageSymIndex);
    updatedMessageSymbol.value = 0;  // Offset 0 in the data section
    obj.updateSymbol(messageSymIndex, updatedMessageSymbol);
    
    // Add instructions to the text section

    // Helper function first (just returns the parameter)
    // SYM helper
    std::vector<coil::Operand> helperSymOperands = {coil::Operand::createSymbol(helperSymIndex)};
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::SYM, helperSymOperands));
    
    // MOV %0, #0 (Copy the parameter to return register)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::MOV, {
        coil::Operand::createRegister(0, coil::Type::RGP),
        coil::Operand::createVariable(0)
    }));
    
    // RET
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::RET, {}));
    
    // Main function
    // SYM main
    std::vector<coil::Operand> mainSymOperands = {coil::Operand::createSymbol(mainSymIndex)};
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::SYM, mainSymOperands));
    
    // Update the main symbol with the correct offset
    coil::Symbol updatedMainSymbol = obj.getSymbol(mainSymIndex);
    updatedMainSymbol.value = obj.getSection(textSectIndex).data.size() - 2;  // Offset in text section
    obj.updateSymbol(mainSymIndex, updatedMainSymbol);
    
    // SCOPEE (Enter scope)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::SCOPEE, {}));
    
    // VAR #1, INT32, 42 (Create a variable)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::VAR, {
        coil::Operand::createVariable(1),
        coil::Operand::createImmediate<uint16_t>(coil::Type::INT32),
        coil::Operand::createImmediate<int32_t>(42)
    }));
    
    // PUSH #1 (Push the variable value)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::PUSH, {
        coil::Operand::createVariable(1)
    }));
    
    // CALL helper (Call the helper function with the value on stack)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::CALL, {
        coil::Operand::createSymbol(helperSymIndex)
    }));
    
    // INC #1 (Increment the variable)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::INC, {
        coil::Operand::createVariable(1)
    }));
    
    // SCOPEL (Leave scope)
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::SCOPEL, {}));
    
    // RET
    obj.addInstruction(textSectIndex, coil::Instruction(coil::Opcode::RET, {}));
    
    // Create an error manager for validation
    coil::ErrorManager errorManager;
    
    // Validate the COIL object
    bool isValid = coil::utils::Validation::validateCoilObject(obj, errorManager);
    
    if (!isValid) {
        std::cerr << "COIL object validation failed:" << std::endl;
        for (const auto& error : errorManager.getErrors()) {
            std::cerr << error.toString() << std::endl;
        }
        return false;
    }
    
    // Encode the object to binary
    std::vector<uint8_t> binary = obj.encode();
    
    // Write the binary to a file
    std::string outputFile = "integration_test.coil";
    if (!writeBinaryFile(outputFile, binary)) {
        std::cerr << "Failed to write binary to file" << std::endl;
        return false;
    }
    
    // Read the binary back and decode it
    std::vector<uint8_t> readBinary = readBinaryFile(outputFile);
    
    if (readBinary.empty()) {
        std::cerr << "Failed to read binary file" << std::endl;
        return false;
    }
    
    try {
        coil::CoilObject decodedObj = coil::CoilObject::decode(readBinary);
        
        // Validate the decoded object
        coil::ErrorManager decodeErrorManager;
        bool decodedValid = coil::utils::Validation::validateCoilObject(decodedObj, decodeErrorManager);
        
        if (!decodedValid) {
            std::cerr << "Decoded COIL object validation failed:" << std::endl;
            for (const auto& error : decodeErrorManager.getErrors()) {
                std::cerr << error.toString() << std::endl;
            }
            return false;
        }
        
        // Verify object properties
        TEST_ASSERT(decodedObj.getSymbolCount() == obj.getSymbolCount());
        TEST_ASSERT(decodedObj.getSectionCount() == obj.getSectionCount());
        
        // Verify specific symbols
        TEST_ASSERT(decodedObj.findSymbol("main") != UINT16_MAX);
        TEST_ASSERT(decodedObj.findSymbol("helper") != UINT16_MAX);
        TEST_ASSERT(decodedObj.findSymbol("message") != UINT16_MAX);
        
        // Verify data section content
        const coil::Section& decodedDataSect = decodedObj.getSection(dataSectIndex);
        TEST_ASSERT(decodedDataSect.data.size() == messageData.size());
        TEST_ASSERT(std::equal(
            decodedDataSect.data.begin(),
            decodedDataSect.data.end(),
            messageData.begin()
        ));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error decoding COIL object: " << e.what() << std::endl;
        return false;
    }
}

bool testVariableIntegration() {
    // Create a variable manager
    coil::VariableManager varManager;
    
    // Create a COIL object
    coil::CoilObject obj = createTestCoilObject();
    
    // Create variables of different types
    uint16_t int32VarId = varManager.createVariable(coil::Type::INT32, {0x01, 0x02, 0x03, 0x04});
    uint16_t fp64VarId = varManager.createVariable(coil::Type::FP64);
    
    // Enter a new scope
    varManager.enterScope();
    
    // Create a local variable
    uint16_t localVarId = varManager.createVariable(coil::Type::INT16, {0x05, 0x06});
    
    // Get variables
    const coil::Variable* int32Var = varManager.getVariable(int32VarId);
    const coil::Variable* fp64Var = varManager.getVariable(fp64VarId);
    const coil::Variable* localVar = varManager.getVariable(localVarId);
    
    TEST_ASSERT(int32Var != nullptr);
    TEST_ASSERT(fp64Var != nullptr);
    TEST_ASSERT(localVar != nullptr);
    
    // Add variable declarations to the executable section
    std::vector<uint8_t> int32VarDecl = int32Var->createDeclaration();
    std::vector<uint8_t> fp64VarDecl = fp64Var->createDeclaration();
    std::vector<uint8_t> localVarDecl = localVar->createDeclaration();
    
    // Add scope enter/exit instructions
    coil::Instruction scopeEnterInstr(coil::Opcode::SCOPEE, {});
    coil::Instruction scopeLeaveInstr(coil::Opcode::SCOPEL, {});
    
    // Clear section data first to ensure clean state
    obj.clearSectionData(0);
    
    // Add instructions to section
    obj.addInstruction(0, scopeEnterInstr);
    
    // Create and add VAR instructions directly
    // Variable 1 (INT32)
    std::vector<coil::Operand> int32VarOperands = {
        coil::Operand::createVariable(int32VarId),
        coil::Operand::createImmediate<uint16_t>(coil::Type::INT32),
        coil::Operand::createImmediate<int32_t>(0x04030201)  // Matches the initializer {0x01, 0x02, 0x03, 0x04}
    };
    obj.addInstruction(0, coil::Instruction(coil::Opcode::VAR, int32VarOperands));
    
    // Variable 2 (FP64)
    std::vector<coil::Operand> fp64VarOperands = {
        coil::Operand::createVariable(fp64VarId),
        coil::Operand::createImmediate<uint16_t>(coil::Type::FP64)
    };
    obj.addInstruction(0, coil::Instruction(coil::Opcode::VAR, fp64VarOperands));
    
    // Enter inner scope
    obj.addInstruction(0, scopeEnterInstr);
    
    // Variable 3 (INT16) - local variable
    std::vector<coil::Operand> localVarOperands = {
        coil::Operand::createVariable(localVarId),
        coil::Operand::createImmediate<uint16_t>(coil::Type::INT16),
        coil::Operand::createImmediate<int16_t>(0x0605)  // Matches the initializer {0x05, 0x06}
    };
    obj.addInstruction(0, coil::Instruction(coil::Opcode::VAR, localVarOperands));
    
    // Add some operations on variables
    obj.addInstruction(0, coil::Instruction(coil::Opcode::INC, {
        coil::Operand::createVariable(int32VarId)
    }));
    
    obj.addInstruction(0, coil::Instruction(coil::Opcode::MOV, {
        coil::Operand::createVariable(fp64VarId),
        coil::Operand::createImmediate<double>(3.14159)
    }));
    
    obj.addInstruction(0, scopeLeaveInstr);  // Leave inner scope
    obj.addInstruction(0, scopeLeaveInstr);  // Leave outer scope
    
    // Validate the object
    coil::ErrorManager errorManager;
    bool isValid = coil::utils::Validation::validateCoilObject(obj, errorManager);
    
    if (!isValid) {
        std::cerr << "COIL object validation failed:" << std::endl;
        for (const auto& error : errorManager.getErrors()) {
            std::cerr << error.toString() << std::endl;
        }
        return false;
    }
    
    return true;
}

bool testErrorIntegration() {
    // Create a COIL object with intentional errors
    coil::CoilObject obj = createTestCoilObject();
    
    // Add an invalid instruction (wrong operand count)
    std::vector<coil::Operand> invalidOperands = {
        coil::Operand::createVariable(1)
    };
    coil::Instruction invalidInstr(coil::Opcode::ADD, invalidOperands);  // ADD needs 3 operands
    obj.addInstruction(0, invalidInstr);
    
    // Add an invalid relocation (invalid symbol index)
    coil::Relocation invalidReloc;
    invalidReloc.offset = 0;
    invalidReloc.symbol_index = 100;  // Invalid symbol index
    invalidReloc.section_index = 0;
    invalidReloc.type = coil::RelocationType::ABSOLUTE;
    invalidReloc.size = 4;
    
    obj.addRelocation(invalidReloc);
    
    // Create an error manager
    coil::ErrorManager errorManager;
    
    // Validate the object (should fail)
    bool isValid = coil::utils::Validation::validateCoilObject(obj, errorManager);
    
    // Should not be valid
    TEST_ASSERT(!isValid);
    
    // Should have errors
    TEST_ASSERT(errorManager.hasErrors());
    
    // Should have specific types of errors
    bool hasInstructionError = false;
    bool hasRelocationError = false;
    
    for (const auto& error : errorManager.getErrors()) {
        uint8_t category = coil::ErrorManager::getErrorCategory(error.error_code);
        uint8_t subcategory = coil::ErrorManager::getErrorSubcategory(error.error_code);
        
        if (category == coil::ErrorCategory::VALIDATION && 
            subcategory == coil::ValidationSubcategory::INSTRUCTION_VALIDITY) {
            hasInstructionError = true;
        }
        
        if (category == coil::ErrorCategory::VALIDATION && 
            subcategory == coil::ValidationSubcategory::RELOCATION) {
            hasRelocationError = true;
        }
    }
    
    TEST_ASSERT(hasInstructionError);
    TEST_ASSERT(hasRelocationError);
    
    return true;
}

int main() {
    std::vector<std::pair<std::string, std::function<bool()>>> tests = {
        {"Complete Workflow", testCompleteWorkflow},
        {"Variable Integration", testVariableIntegration},
        {"Error Integration", testErrorIntegration}
    };
    
    return runTests(tests);
}