# COIL Library

COIL (Compiler Optimization & Instruction Library) is a minimalist C++ library designed for working with a custom intermediate representation and binary format. It provides efficient abstractions for manipulating code, similar to LLVM IR or WebAssembly, but with an emphasis on performance and low overhead.

## Design Principles

- **Minimal Allocations**: Avoid heap allocations where possible
- **Stack First**: Prefer stack allocations and references
- **Clean API**: Simple, intuitive interfaces
- **Performance**: Zero-cost abstractions that compile to efficient machine code
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
- Section-based layout
- Strong typing
- Minimal metadata

### Utilities

- Stream abstraction for I/O
- Minimal error handling
- Lean logging

## Getting Started

### Building

```bash
meson setup build
cd build
ninja
```

### Basic Usage

```cpp
#include "coil/coil.hpp"
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include "coil/error.hpp"

int main() {
    // Create an object file
    coil::Object obj = coil::Object::create(coil::ObjType::Relocatable);
    
    // Add a code section
    const uint8_t code[] = { 0x01, 0x02, 0x03, 0x04 };
    obj.addSection(".text", coil::SectionType::Code, 
                  coil::SectionFlag::Executable, code, sizeof(code));
    
    // Save to file
    coil::FileStream fs("output.obj", coil::StreamMode::Write);
    obj.save(fs);
    
    return 0;
}
```

## License

This project is in the public domain. See the LICENSE file for details.