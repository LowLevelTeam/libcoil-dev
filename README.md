# libcoil-dev

A zero-dependency development library for the COIL (Computer Oriented Intermediate Language) ecosystem.

## Overview

`libcoil-dev` provides foundational support for COIL toolchain development without any external dependencies - not even the C standard library. The library uses direct system calls for all operations, giving maximum control and performance while maintaining a clean, consistent API across platforms.

The library is organized into two main components:

- **core/**: Replaces functionality typically found in standard libraries with zero-dependency implementations
  - Memory management
  - Error handling
  - Platform abstraction
  - Stream I/O operations
  - Base types and utilities

- **format/**: Provides COIL-specific functionality for toolchain development
  - Instruction encoding/decoding
  - Binary section handling
  - Type system implementation
  - COIL binary validation

## Design Principles

- **No Dependencies**: No external libraries or standard library usage
- **Raw System Calls**: Direct system call implementation for all operations
- **Platform-Specific Optimization**: Structure layouts can vary across platforms to optimize for stack allocation
- **Explicit Memory Management**: No implicit heap allocations
- **User Control**: Memory allocation must always be explicit and controlled by the user
- **No Nested Allocations**: Functions do not allocate memory internally
- **Consistent API**: Maintain functional API consistency across platforms
- **Stack-Only Structures**: All structures are defined in headers and can be stack-allocated

## Building

`libcoil-dev` uses the Meson build system:

```bash
meson setup builddir
cd builddir
meson compile
```

## Platform Support

Currently supported platforms:
- Linux (x86_64, ARM64)
- Windows (x86_64)
- macOS (x86_64, ARM64)

Platform-specific macros are defined by the Meson build system:
- `__COIL_LINUX`
- `__COIL_WIN32`
- `__COIL_MACOS`

## Usage Example

```c
#include <coil/core/memory.h>
#include <coil/format/instructions.h>

int main(void) {
    // Allocate memory with direct system calls
    CoilMemContext mem_ctx;
    void* buffer = coil_mem_alloc(1024, COIL_MEM_READ | COIL_MEM_WRITE, &mem_ctx);
    
    // Create an instruction (on the stack)
    CoilInstruction instr;
    coil_instruction_init(&instr, 0x60, 3); // ADD instruction with 3 operands
    
    // Set up operands
    // ... (code to configure operands)
    
    // Encode the instruction to the buffer
    size_t bytes_written;
    coil_instruction_encode(&instr, buffer, 1024, &bytes_written);
    
    // Process the encoded instruction...
    
    // Free allocated memory
    coil_mem_free(buffer, &mem_ctx);
    
    return 0;
}
```

## License

This is free and unencumbered software released into the public domain. See the LICENSE file for details.
