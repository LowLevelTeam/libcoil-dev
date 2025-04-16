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

TODO

## License

This project is in the public domain. See the LICENSE file for details.