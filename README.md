# libcoil-dev

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

`libcoil-dev` is the core library that provides the fundamental data structures, utilities, and common functionality for the COIL (Computer Oriented Intermediate Language) ecosystem. It serves as the foundation upon which all other COIL components are built.

This library defines the COIL binary format, type system, error handling mechanisms, and other essential components that ensure interoperability between different COIL implementations.

## Features

- **Binary Format Definitions**: Complete implementation of the COIL binary format
- **Type System**: Comprehensive type system with support for all COIL types
- **Error Handling**: Standardized error classification and reporting
- **Utilities**: Common utilities for binary manipulation, validation, and encoding/decoding
- **Extensibility**: Clean interfaces for implementing platform-specific features

## Installation

### Prerequisites

- C++17 compliant compiler
- CMake 3.15+

### Building from Source

```bash
# Clone the repository
git clone https://github.com/LLT/libcoil-dev.git
cd libcoil-dev

# Create build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build
cmake --build .

# Install
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
#include <coil/error_codes.h>

// Create a COIL object
coil::CoilObject obj;

// Add a section
coil::Section textSection;
textSection.nameIndex = 1;  // Assuming symbol index 1 is ".text"
textSection.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
// ... set other properties
obj.addSection(textSection);

// Encode to binary
std::vector<uint8_t> binary = obj.encode();

// Write to file
std::ofstream file("output.coil", std::ios::binary);
file.write(reinterpret_cast<const char*>(binary.data()), binary.size());
```

## Components

### Binary Format

The `coil::binary_format` namespace provides the structures and functions for working with COIL binary files:

- `CoilHeader`: COIL object file header
- `CoilOHeader`: COIL output object header
- `Section`: Section definition
- `Symbol`: Symbol definition
- `Relocation`: Relocation entry
- `Instruction`: Instruction encoding/decoding

### Type System

The `coil::type_system` namespace defines the COIL type system:

- Type encodings (16-bit format)
- Type categories (integer, floating-point, vector, etc.)
- Type compatibility rules
- Type conversion utilities

### Error Handling

The `coil::error` namespace provides a standardized error handling system:

- Error codes organized by category
- Error reporting functions
- Error information structures
- Diagnostic utilities

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- [API Reference](docs/api/index.html)
- [Binary Format Specification](docs/spec/binary_format.md)
- [Type System Specification](docs/spec/type_system.md)
- [Error Code Reference](docs/ref/error_codes.md)

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute to libcoil-dev.

## Implementation

For details on the implementation approach, architecture, and development plans, see [IMPLEMENTATION.md](IMPLEMENTATION.md).

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.
