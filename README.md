# COIL Library

COIL (Computer Oriented Intermediate Language) is a modern C++ library designed for working with a custom intermediate representation and binary format. It provides efficient abstractions for manipulating code, similar to LLVM IR or WebAssembly, but with an emphasis on performance and clean design.

## Design Principles

- **Modern C++ Design**: Utilizing C++17 features for clean, maintainable code
- **Exception-based Error Handling**: Clear error reporting and handling
- **Strong Type Safety**: Prevent errors through proper type design
- **Standard Library Integration**: Leveraging standard containers and algorithms
- **Clean API**: Simple, intuitive interfaces with good documentation
- **Performance**: Efficient implementation with minimal overhead
- **Portability**: Works across platforms with consistent behavior

## Core Components

### Instruction Set

A compact instruction set for representing code:
- Control flow operations
- Memory operations
- Arithmetic/logic operations
- Type system with fixed-width numerics

### Object Format

A simple binary format for storing compiled code:
- Section-based layout (inspired by ELF)
- Strong typing
- Minimal metadata
- Symbol and string tables

### Streams

A modern abstraction for I/O operations:
- File and memory-based streams
- Consistent API for different storage types
- Exception-based error handling

### Error Handling

Comprehensive error reporting and handling:
- Typed exceptions for different error categories
- Position tracking for accurate error reporting
- Customizable logging

## Getting Started

### Building with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
cd build
ctest
```

### Basic Usage

```cpp
#include <coil/coil.hpp>
#include <coil/instr.hpp>
#include <coil/obj.hpp>
#include <iostream>

int main() {
  // Initialize the library
  coil::initialize();
  
  // Create an instruction block
  coil::InstructionBlock block;
  
  // Add some instructions
  coil::Operand reg1 = coil::createRegOp(1, coil::ValueType::I32);
  coil::Operand imm = coil::createImmOpInt(42, coil::ValueType::I32);
  coil::Instruction loadInstr = coil::createInstr(coil::Opcode::Load, reg1, imm);
  
  block.addInstruction(loadInstr);
  
  // Create an object file
  coil::Object obj = coil::Object::create();
  
  // Initialize string table
  obj.initStringTable();
  
  // Add a section name
  uint64_t nameOffset = obj.addString(".text");
  
  // Create a data section for the code
  std::vector<uint8_t> codeData;
  // ... populate code data ...
  
  // Add the section
  obj.addSection(
    nameOffset,
    static_cast<uint16_t>(coil::SectionFlag::Code | coil::SectionFlag::Alloc),
    static_cast<uint8_t>(coil::SectionType::ProgBits),
    codeData.size(),
    codeData
  );
  
  // Save to a file
  coil::FileStream stream("output.coil", coil::StreamMode::Write);
  obj.save(stream);
  
  // Clean up
  coil::shutdown();
  return 0;
}
```

## License

This project is in the public domain. See the LICENSE file for details.