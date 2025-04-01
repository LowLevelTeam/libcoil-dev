# libcoil-dev

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

`libcoil-dev` is the foundational library for the COIL (Computer Oriented Intermediate Language) Toolchain. It provides the core data structures, utilities, and functionality needed by all COIL components.

This library implements the COIL specification, ensuring consistent implementation across all tools in the ecosystem.

## Features

- **Binary Format**: Complete implementation of the COIL binary format
- **Type System**: Comprehensive type system with all COIL types
- **Instruction Set**: Encoding and decoding of COIL instructions
- **Variable System**: Implementation of COIL's variable abstraction
- **Error Handling**: Standardized error classification and reporting

## COIL Toolchain Integration

This library serves as the foundation for all components in the LLT COIL Toolchain:

```
[libcoil-dev]
    |
    ├─── CASM (Assembler)
    |      |
    |      v
    |    (.coil files)
    |
    ├─── COILP (Processor)
    |      |
    |      v
    |    (.coilo files)
    |
    └─── CBC (Bytecode Interpreter)
           |
           v
         (.cbc files)
```

## Installation

### Prerequisites

- C++17 compliant compiler
- CMake 3.15+

### Building from Source

```bash
git clone https://github.com/LLT/libcoil-dev.git
cd libcoil-dev
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

## Usage

### In CMake Projects

```cmake
find_package(libcoil-dev REQUIRED)
target_link_libraries(your_target PRIVATE libcoil-dev)
```

### In Code

```cpp
#include <coil/binary_format.h>
#include <coil/type_system.h>
#include <coil/instruction_set.h>

// Create a COIL object
coil::CoilObject obj;

// Add a section
coil::Section textSection;
textSection.name_index = 1;  // Assuming symbol index 1 is ".text"
textSection.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
obj.addSection(textSection);

// Add an instruction
std::vector<coil::Operand> operands = {
    coil::Operand::createVariable(1),
    coil::Operand::createImmediate(42, coil::Type::INT32)
};
coil::Instruction instrMov(coil::Opcode::MOV, operands);
obj.addInstruction(0, instrMov.getOpcode(), instrMov.encode());

// Encode to binary
std::vector<uint8_t> binary = obj.encode();
```

## API Reference

### Core Components

- **CoilObject**: Container for all COIL file components
- **Symbol**: Symbol table entries for names and references
- **Section**: Code and data section management
- **Relocation**: Relocation entries for linking
- **Instruction**: COIL instruction representation
- **Operand**: Instruction operand representation
- **TypeInfo**: Type system utilities
- **VariableManager**: Variable and scope management
- **ErrorManager**: Error tracking and reporting

### Utility Components

- **BinaryUtils**: Binary data manipulation utilities
- **Validation**: Object validation utilities

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on how to contribute to the project.

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.