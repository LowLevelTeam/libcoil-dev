# COIL Library

COIL (Computer Oriented Intermediate Language) is a modern C library designed for working with a custom intermediate representation and binary format. It provides efficient abstractions for manipulating code, similar to LLVM IR or WebAssembly, but with an emphasis on performance and clean design.

## Design Principles

- **Performance-Focused C Design**: Optimized for speed and memory efficiency
- **Arena-Based Memory Management**: Fast allocation with minimal overhead
- **Strong Type Safety**: Prevent errors through proper type design
- **Clean API**: Simple, intuitive interfaces with good documentation
- **Portability**: Works across platforms with consistent behavior

## Core Components

### Arena Allocator

A high-performance memory management system:
- Fast allocation without per-allocation overhead
- Bulk deallocation for better performance
- Alignment support
- Support for both stack and heap allocation strategies

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

### Error Handling

Comprehensive error reporting and handling:
- Typed error codes
- Position tracking for accurate error reporting
- Customizable logging

## Getting Started

### Building with Meson

```bash
meson setup build
meson compile -C build
```

### Running Tests

The library comes with a comprehensive test suite to verify all components:

```bash
meson test -C build
```

To run specific test categories:

```bash
meson test -C build arena_tests
meson test -C build error_tests
meson test -C build instr_tests
meson test -C build obj_tests
meson test -C build coil_tests
```

For verbose test output:

```bash
meson test -C build -v
```

### Basic Usage

```c
#include <ccoil/coil.h>
#include <ccoil/instr.h>
#include <ccoil/obj.h>
#include <ccoil/arena.h>
#include <stdio.h>

int main() {
  // Initialize error system
  coil_error_init();
  
  // Create an arena for memory management
  coil_arena_t* arena = arena_init(4096, 0);
  
  // Create a COIL object
  coil_object_t* obj = coil_object_create(arena);
  
  // Initialize string table
  coil_object_init_string_table(obj, arena);
  
  // Add a section name
  coil_u64_t name_offset = coil_object_add_string(obj, ".text", arena);
  
  // Create some instructions
  uint8_t code[] = {
    // Example instruction encoding
    0x10, 0x02, 0x01, 0x01, 0x00, 0x04, 0x00, 0x2A, // MOV r1, #42
    0x10, 0x02, 0x01, 0x02, 0x00, 0x04, 0x00, 0x0D, // MOV r2, #13
    0x30, 0x02, 0x01, 0x03, 0x00, 0x01, 0x01, 0x00, // ADD r3, r1, r2
  };
  
  // Add a section
  coil_object_add_section(
    obj,
    name_offset,
    COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_ALLOC,
    COIL_SECTION_PROGBITS,
    code,
    sizeof(code),
    arena
  );
  
  // Save to a file
  coil_object_save_to_file(obj, "output.coil");
  
  // Clean up
  coil_object_destroy(obj, arena);
  arena_destroy(arena);
  coil_error_shutdown();
  
  return 0;
}
```

## Memory Management

COIL uses an arena-based memory management strategy for improved performance:

- **Low Allocation Overhead**: Reduced memory management cost for small objects
- **Contiguous Memory**: Better cache locality
- **Bulk Free**: Fast cleanup operation for all allocations
- **Deterministic Lifetime**: All allocations can be freed at once, simplifying resource management

## License

This project is in the public domain. See the LICENSE file for details.