#include <coil/binary_format.h>
#include <coil/object_file.h>
#include <coil/type_system.h>
#include <iostream>
#include <vector>

using namespace coil;

int main() {
    std::cout << "COIL Simple Example" << std::endl;
    std::cout << "===================" << std::endl << std::endl;
    
    // Create types we'll use
    auto int32Type = Type::createInt32();
    auto fp32Type = Type::createFp32();
    
    // Create a binary builder
    BinaryBuilder builder;
    
    // Create variables
    uint64_t var1 = builder.createVariable(int32Type);
    uint64_t var2 = builder.createVariable(int32Type);
    uint64_t var3 = builder.createVariable(int32Type);
    uint64_t resultVar = builder.createVariable(int32Type);
    
    // Add instructions: result = (var1 + var2) * var3
    
    // Create operands
    Operand var1Operand = Operand::fromVariable(var1, int32Type);
    Operand var2Operand = Operand::fromVariable(var2, int32Type);
    Operand var3Operand = Operand::fromVariable(var3, int32Type);
    Operand resultOperand = Operand::fromVariable(resultVar, int32Type);
    Operand imm10 = Operand::fromImmediate(int32Type, {10, 0, 0, 0}); // 10 as INT32
    Operand imm20 = Operand::fromImmediate(int32Type, {20, 0, 0, 0}); // 20 as INT32
    Operand imm3 = Operand::fromImmediate(int32Type, {3, 0, 0, 0});   // 3 as INT32
    
    // Instructions
    std::cout << "Creating instructions:" << std::endl;
    
    // var1 = 10
    Instruction mov1(Instruction::OpCode::MOV, {var1Operand, imm10});
    std::cout << "  " << mov1.toString() << std::endl;
    builder.addInstruction(mov1);
    
    // var2 = 20
    Instruction mov2(Instruction::OpCode::MOV, {var2Operand, imm20});
    std::cout << "  " << mov2.toString() << std::endl;
    builder.addInstruction(mov2);
    
    // var3 = 3
    Instruction mov3(Instruction::OpCode::MOV, {var3Operand, imm3});
    std::cout << "  " << mov3.toString() << std::endl;
    builder.addInstruction(mov3);
    
    // result = var1 + var2
    Instruction add(Instruction::OpCode::ADD, {resultOperand, var1Operand, var2Operand});
    std::cout << "  " << add.toString() << std::endl;
    builder.addInstruction(add);
    
    // result = result * var3
    Instruction mul(Instruction::OpCode::MUL, {resultOperand, resultOperand, var3Operand});
    std::cout << "  " << mul.toString() << std::endl;
    builder.addInstruction(mul);
    
    // Generate binary
    std::vector<uint8_t> binary = builder.getBinary();
    std::cout << "\nGenerated binary size: " << binary.size() << " bytes" << std::endl;
    
    // Write to a file
    std::string binaryFilename = "simple_program.coil";
    if (builder.writeToFile(binaryFilename)) {
        std::cout << "Binary written to " << binaryFilename << std::endl;
    } else {
        std::cout << "Failed to write binary file" << std::endl;
        return 1;
    }
    
    // Now read it back
    std::cout << "\nReading binary back:" << std::endl;
    BinaryReader reader(binaryFilename);
    
    if (!reader.isValid()) {
        std::cout << "Failed to read binary file: " << reader.getError() << std::endl;
        return 1;
    }
    
    std::vector<Instruction> instructions = reader.getInstructions();
    std::cout << "Read " << instructions.size() << " instructions:" << std::endl;
    
    for (const auto& instruction : instructions) {
        std::cout << "  " << instruction.toString() << std::endl;
    }
    
    // Create an object file with this program
    std::cout << "\nCreating object file:" << std::endl;
    
    ObjectFile objFile;
    
    // Add a code section with our instructions
    uint32_t codeSection = objFile.addCodeSection(".text", instructions);
    std::cout << "Added code section with index " << codeSection << std::endl;
    
    // Add a symbol for the code
    Symbol mainSymbol(
        "main",                       // name
        codeSection,                  // section index
        0,                            // value (offset in section)
        binary.size(),                // size
        SymbolType::FUNCTION,         // type
        SymbolBinding::GLOBAL,        // binding
        SymbolVisibility::DEFAULT     // visibility
    );
    
    uint32_t mainIndex = objFile.addSymbol(mainSymbol);
    std::cout << "Added main symbol with index " << mainIndex << std::endl;
    
    // Set entry point to main
    objFile.setEntryPoint(0);
    
    // Set target platform (CPU, x86, 64-bit)
    objFile.setTargetPlatform(0, 0, 3);
    
    // Save object file
    std::string objFilename = "simple_program.o";
    if (objFile.saveToFile(objFilename)) {
        std::cout << "Object file written to " << objFilename << std::endl;
    } else {
        std::cout << "Failed to write object file" << std::endl;
        return 1;
    }
    
    std::cout << "\nExample completed successfully!" << std::endl;
    return 0;
}
