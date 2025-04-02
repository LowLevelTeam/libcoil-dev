# libcoil-dev

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://unlicense.org)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()

## Overview

`libcoil-dev` is the foundational library for the COIL (Computer Oriented Intermediate Language) Toolchain. It provides the core data structures, utilities, and functionality needed by all COIL components.

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
    |    (.casm files -> .coil files)
    |
    ├─── COILP (Processor)
    |      |
    |      v
    |    (.coil files -> .coilo / .cbc files)
    |
    └─── CBC (Bytecode Interpreter)
           |
           v
         (.cbc files - > execution)
```

## Building from Source

### Prerequisites

- C++17 compliant compiler
- Meson build system

### Build Instructions

```bash
meson setup builddir
cd builddir
meson compile
meson install
```

## Usage in Meson Projects

```meson
libcoil_dep = dependency('libcoil-dev')
executable('your_app', 'main.cpp',
           dependencies: [libcoil_dep])
```

## API Reference

### Core Components

- **CoilObject**: Container for all COIL file components
- **Symbol**: Symbol table entries
- **Section**: Code and data section management
- **Relocation**: Relocation entries for linking
- **Instruction**: COIL instruction representation
- **TypeInfo**: Type system utilities
- **VariableManager**: Variable and scope management
- **ErrorManager**: Error tracking and reporting

## License

This project is released under the Unlicense. See [LICENSE](LICENSE) for details.