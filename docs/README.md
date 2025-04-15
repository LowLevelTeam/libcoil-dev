# COIL Library

## Overview

COIL (Compiler Optimization & Instruction Library) is a C++ library designed for working with a custom intermediate representation and binary format. The library provides zero-cost abstractions for manipulating code, similar to LLVM IR or WebAssembly, but with a focus on performance and extensibility.

## Core Components

### Instruction Set Architecture

COIL defines a complete instruction set architecture (ISA) that serves as an intermediate representation for compilation:

- **Control Flow Operations**: Branch, call, return, and comparison instructions
- **Memory Operations**: Data movement, stack manipulation, and scope control
- **Arithmetic Operations**: Standard mathematical operations
- **Bitwise Operations**: Bit-level manipulations
- **Multi-Dimensional Operations**: For multi scalar operations
- **Type System**: Comprehensive type system with fixed-width integers, floating point, vectors

### Object Format

COIL includes a custom binary object format, inspired by ELF but tailored for the COIL instruction set:

- **Section-based layout**: Code, data, symbols, and other information organized in sections
- **Strong typing**: All data is strongly typed
- **Metadata support**: Version information, target platform
- **Relocation**: Supports various relocation types for linking

### Library Infrastructure

The library provides several utility components:

- **Stream System**: Abstract I/O with FileStream and MemoryStream implementations
- **Logging**: Configurable logging with severity levels
- **Error Handling**: Detailed error reporting with position tracking

## Directory Structure

```
/include/coil/       - Public API headers
  /coil.hpp          - Main header and version information
  /instr.hpp         - Instruction set definitions
  /obj.hpp           - Object format definitions
  /stream.hpp        - Stream abstraction for I/O
  /log.hpp           - Logging facilities
  /err.hpp           - Error handling

/src/                - Implementation files
  /coil.cpp          - Version information implementation
  /log.cpp           - Logger implementation
  /err.cpp           - Error manager implementation
  /stream.cpp        - Stream implementations
  /obj.cpp           - Object format implementation

/tests/              - Test suite
  /test_*.cpp        - Test files for each component
```

## Getting Started

### Building the Library

The library uses the Meson build system:

```bash
meson setup build
cd build
ninja
```

### Using the Library

Here's a simple example of using the library to create an object file:

```cpp
#include "coil/coil.hpp"
#include "coil/stream.hpp"
#include "coil/obj.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"

int main() {
    // Create context
    coil::Logger logger("COIL", stdout, coil::LogLevel::Info);
    coil::ErrorManager errorMgr(logger);
    coil::Context ctx{logger, errorMgr};
    
    // Create an object file
    auto obj = coil::CoilObject::create(coil::obj::CT_REL, 0, ctx);
    
    // Add a section
    const char* code = "Example code data";
    obj->addSection(".text", coil::obj::CST_CODE, coil::obj::CSF_EXEC,
                    (const uint8_t*)code, strlen(code), 0);
    
    // Save the object file
    auto stream = coil::FileStream::create("output.obj", "wb", ctx);
    obj->save(*stream);
    
    // Clean up
    stream->close();
    delete stream;
    delete obj;
    
    return 0;
}
```

## License

This project is in the public domain under the Unlicense.