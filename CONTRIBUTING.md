# Contributing to libcoil-dev

Thank you for considering contributing to the libcoil-dev library. This document outlines the current state of the project, areas that need further work, and guidelines for contributors.

## Current Implementation Status

The current implementation provides the core foundation for the COIL toolchain, including:

- Binary format encoding/decoding
- Type system representation and validation
- Instruction set encoding/decoding
- Variable system and scope management
- Error reporting and validation
- Utility functions for binary operations and validation

## Areas Requiring Further Development

### 1. Validation System Completion

The validation system has some placeholder methods that need to be implemented more robustly:

- **Variable Usage Validation**: The `Validation::validateVariableUsage()` method is currently a stub. It needs to be expanded to track variable declarations and uses throughout the program, respecting scope boundaries.

- **Type Validation for Instructions**: Additional type checking for instruction operands is needed, especially for more complex instructions like vector operations.

- **ABI Validation**: The ABI system needs comprehensive validation to ensure correct parameter passing and return value handling.

### 2. Memory Management

- **Memory Model Implementation**: The current code primarily focuses on a flat memory model. Implementations for segmented, hierarchical, and distributed memory models would be valuable additions.

- **Alignment Handling**: While the foundation for alignment checking exists, more robust handling of platform-specific alignment requirements is needed.

### 3. Advanced Features

- **Debug Information**: The debug information format is defined in the spec but not fully implemented in the library.

- **CBC Compilation**: Support for compiling COIL to CBC (COIL Byte Code) format is not implemented.

- **Multi-Device Support**: Currently, the library focuses on CPU instructions. Expanding to support GPU, TPU, and other device types is a future enhancement.

### 4. Testing and Documentation

- **Unit Tests**: Comprehensive unit tests for each component are needed. The `tests` directory contains a placeholder structure.

- **Integration Tests**: Tests that validate the entire toolchain flow from CASM to final binary.

- **API Documentation**: While the code has inline documentation, a more comprehensive API guide would be beneficial.

## Implementation Notes for Contributors

### Error System

When extending the error system:

1. Add new error codes to the appropriate category in `error_codes.h`
2. Implement meaningful error messages in `ErrorManager::getStandardErrorMessage()`
3. Use the error manager to provide context-rich error information

### Variable System

The variable system currently tracks variables within scopes but has limitations:

1. It doesn't perform dataflow analysis to track uninitialized variables
2. It doesn't analyze variable lifetime across different execution paths
3. Advanced optimization like register allocation is not implemented

### Instruction Set

When enhancing the instruction set:

1. Add new opcodes to the appropriate section in `Opcode` namespace
2. Update `InstructionSet::getExpectedOperandCount()` and `InstructionSet::getInstructionName()`
3. Enhance validation in `validateInstruction()` method

### Binary Format

The binary format implementation can be extended by:

1. Adding support for more complex relocation types
2. Implementing section merging for linking
3. Adding support for debug information sections

## Coding Standards

- Follow the existing code style for consistency
- Use clear, descriptive variable and function names
- Add comprehensive comments for complex logic
- Write unit tests for new functionality
- Update documentation when adding new features

## Getting Started

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Add tests that validate your changes
5. Submit a pull request with a clear description of the changes

Thank you for contributing to libcoil-dev!