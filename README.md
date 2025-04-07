# libcoil-dev

A C++ library for creating, manipulating, and reading COIL (Computer Oriented Intermediate Language) files according to the COIL specification v1.1.0.

## Overview

libcoil-dev provides a comprehensive API for working with the COIL binary format, type system, instruction sets, and object files. It enables developers to:

- Parse and generate COIL binary instructions
- Create and manipulate COIL types
- Assemble and disassemble COIL code
- Create, read, and link COIL object files
- Work with COIL's memory model

## Features

- **Complete Type System**: Full implementation of the COIL type system, including fixed-width integers, floating-point types, vectors, matrices, tensors, and composite types.
- **Instruction Set Support**: Support for all COIL instruction sets (Universal, Extended, Compiler).
- **Binary Format Handling**: Low-level APIs for working with COIL's binary encoding.
- **Object File Operations**: Parsing, creating, and modifying COIL object files.
- **Linking Support**: Utilities for linking multiple COIL object files.
- **ABI Definitions**: Support for creating and managing Application Binary Interface definitions.
- **Cross-Platform**: Designed to work on Linux, macOS, and Windows.
- **Modern C++**: Written in C++17 with clean, consistent interfaces.

## Building

libcoil-dev uses the Meson build system.

### Prerequisites

- A C++17-compatible compiler (GCC 8+, Clang 6+, MSVC 19.14+)
- Meson (0.50.0 or later)
- Ninja (1.8.2 or later)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/libcoil-dev.git
cd libcoil-dev

# Configure the build
meson setup builddir

# Build the library
meson compile -C builddir

# Run tests
meson test -C builddir

# Install the library
meson install -C builddir
```

## Usage Examples

### Creating a Simple COIL Program

```cpp
#include <coil/binary_builder.h>
#include <coil/instruction.h>
#include <coil/types.h>

using namespace coil;

int main() {
    // Create a binary builder
    BinaryBuilder builder;
    
    // Define a variable
    Variable var1 = builder.createVariable(Type::INT32);
    
    // Add an instruction (MOV var1, 42)
    builder.addInstruction(Instruction::MOV, {var1, Immediate(Type::INT32, 42)});
    
    // Write to a file
    builder.writeToFile("example.coil");
    
    return 0;
}
```

### Reading a COIL Object File

```cpp
#include <coil/object_file.h>
#include <iostream>

using namespace coil;

int main() {
    // Open a COIL object file
    ObjectFile obj("input.coil");
    
    // Print information about sections
    for (const auto& section : obj.getSections()) {
        std::cout << "Section: " << section.getName() 
                  << ", Size: " << section.getSize() << std::endl;
    }
    
    // Print information about symbols
    for (const auto& symbol : obj.getSymbols()) {
        std::cout << "Symbol: " << symbol.getName() 
                  << ", Value: " << symbol.getValue() << std::endl;
    }
    
    return 0;
}
```

## Documentation

Comprehensive API documentation is available at [https://yourdomain.com/libcoil-dev-docs](https://yourdomain.com/libcoil-dev-docs).

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request
