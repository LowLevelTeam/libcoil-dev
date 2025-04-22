# COIL Library

COIL (Computer Oriented Intermediate Language) is a modern C library designed for working with COIL objects and COIL instructions both encoding and decoding.

## Design Principles

- **Performance-Focused C Design**: Optimized for speed and memory efficiency
- **Strong Type Safety**: Prevent errors through proper type design
- **Clean API**: Simple, intuitive interfaces with good documentation
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

### Error Handling

Comprehensive error reporting and handling:
- Typed error codes
- Position tracking for accurate error reporting

## License

This project is in the public domain. See the LICENSE file for details.