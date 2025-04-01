# libcoil-dev

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

`libcoil-dev` is the foundational library for the COIL (Computer Oriented Intermediate Language) Toolchain. It provides the core data structures, utilities, and functionality needed by all other COIL components.

As the base layer of the LLT COIL Toolchain, this library ensures consistent implementation of the COIL specification across all tools in the ecosystem.

## Features

- **Binary Format**: Complete implementation of the COIL binary format
- **Type System**: Comprehensive type system with all COIL types
- **Instruction Set**: Encoding and decoding of COIL instructions
- **Variable System**: Implementation of COIL's variable abstraction
- **Error Handling**: Standardized error classification and reporting
- **Extensibility**: Clean interfaces for implementing platform-specific features

## COIL Toolchain Integration

This library is the foundation for all components in the LLT COIL Toolchain:

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
textSection.nameIndex = 1;  // Assuming symbol index 1 is ".text"
textSection.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
obj.addSection(textSection);

// Add an instruction
std::vector<coil::Operand> operands = {
    coil::Operand::createVariable(1),
    coil::Operand::createImmediate(42, coil::Type::INT32)
};
obj.addInstruction(coil::Instruction::MOV, operands);

// Encode to binary
std::vector<uint8_t> binary = obj.encode();
```

## Core Components

### Binary Format

The `coil::binary_format` namespace provides structures for working with COIL binary files:

- `CoilHeader`: COIL object file header
- `CoilOHeader`: COIL output object header
- `Section`: Section definition
- `Symbol`: Symbol definition
- `Relocation`: Relocation entry

### Type System

The `coil::type_system` namespace defines the COIL type system:

- Type encodings (16-bit format)
- Type categories (integer, floating-point, vector, etc.)
- Type compatibility rules
- Type conversion utilities

### Instruction Set

The `coil::instruction_set` namespace provides:

- Instruction definitions and opcodes
- Operand encoding and decoding
- Instruction validation

## Documentation

Documentation is available in the `docs/` directory:

- [API Reference](docs/api/index.html)
- [Binary Format Specification](docs/spec/binary_format.md)
- [Type System Specification](docs/spec/type_system.md)
- [Instruction Set Reference](docs/ref/instruction_set.md)

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute.

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.