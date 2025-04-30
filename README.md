# libcoil-dev - COIL Development Library

The COIL Development Library (libcoil-dev) provides a comprehensive API for working with the COIL (Computer Oriented Intermediate Language) format, supporting manipulation of intermediate representations, target-specific code, and cross-platform development.

## Overview

libcoil-dev is the core library for the COIL toolchain, providing functionality to:

- Read, write, and manipulate COIL files and intermediate representations
- Support target-specific code with comprehensive architecture metadata
- Define a clean API for processing COIL instructions and sections
- Enable cross-platform compilation and optimization

This library serves as the foundation for other COIL tools, including:
- COIL C Compiler (ccc)
- COIL Object Processor (cop)
- COIL Latent Linker (cll)

## Features

- **COIL File Format**: API for reading and writing the COIL intermediate representation format
- **Target Metadata System**: Ability to associate target architecture metadata with sections for optimal code generation
- **Native Machine Code Support**: Store and manage native machine code with proper architecture metadata
- **Cross-Platform Targeting**: Support for multiple processing units and architectures
- **Instruction Set**: Complete API for COIL instruction encoding and decoding
- **Memory Management**: Optimized memory handling for sections and objects
- **Error Handling**: Comprehensive error reporting and handling

## Native Machine Code and Target Metadata

The COIL object format supports both intermediate COIL code and native machine code through its target metadata system:

- **DATA Sections with Target Metadata**: When a section has both the `COIL_SECTION_FLAG_TARGET` flag and target metadata set (PU, architecture, feature flags), the section is interpreted as containing native machine code for that target. This allows embedding and executing native code with proper architecture awareness.

- **Dual Use of Target Metadata**: Target metadata serves two purposes:
  1. For COIL code sections: Provides compilation target information for the COIL compiler
  2. For native code sections: Provides execution environment information for proper loading and execution

- **Multiple Target Support**: A single COIL object can contain multiple sections targeting different architectures, enabling multi-architecture binaries and libraries.

## Library Structure

The library is organized into several modules:

- **Base**: Common types, memory operations, and utilities
- **Error Handling**: Error codes and reporting
- **File I/O**: File operations and descriptor management
- **Logging**: Configurable logging system
- **Sections**: Section management for different types of content
- **Objects**: COIL object format management
- **Instructions**: Instruction encoding and decoding

## Building

### Prerequisites

- C99 compatible compiler
- make
- git (for obtaining the source)

### Build Commands

```bash
# Clone the repository
git clone https://github.com/coil-project/libcoil-dev.git
cd libcoil-dev

# Build the library
make

# Run tests
make check

# Install the library (may require sudo)
make install
```

### Build Options

- `DEBUG=1`: Enable debug build with additional logging and symbols
- `PREFIX=/custom/path`: Set custom installation prefix (default: /usr/local)

## Usage Examples

### Basic COIL Object Creation

```c
#include <coil/obj.h>
#include <coil/sect.h>

int main() {
  // Initialize object
  coil_object_t obj;
  coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  
  // Set target defaults (x86_64 with AVX2)
  coil_obj_set_target_defaults(&obj, COIL_PU_CPU, COIL_CPU_x86_64, COIL_CPU_X86_AVX2);
  
  // Create a section
  coil_section_t sect;
  coil_section_init(&sect, 1024);
  
  // Write data to section
  const char *data = "Hello, COIL!";
  coil_section_write(&sect, (coil_byte_t *)data, strlen(data), NULL);
  
  // Add section to object
  coil_u16_t index;
  coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".data", 
                         COIL_SECTION_FLAG_NONE, &sect, &index);
  
  // Save object to file
  int fd = open("output.coil", O_RDWR | O_CREAT | O_TRUNC, 0644);
  coil_obj_save_file(&obj, fd);
  close(fd);
  
  // Cleanup
  coil_section_cleanup(&sect);
  coil_obj_cleanup(&obj);
  
  return 0;
}
```

### Creating Target-Specific Sections

```c
#include <coil/obj.h>
#include <coil/sect.h>

int main() {
  // Initialize object
  coil_object_t obj;
  coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  
  // Create an x86_64 specific section
  coil_obj_set_target_defaults(&obj, COIL_PU_CPU, COIL_CPU_x86_64, 
                              COIL_CPU_X86_AVX2 | COIL_CPU_X86_SSE4_2);
  
  coil_section_t x86_sect;
  coil_section_init(&x86_sect, 1024);
  coil_section_write(&x86_sect, (coil_byte_t *)"x86_64 code", 11, NULL);
  
  coil_u16_t x86_index;
  coil_obj_create_section(&obj, COIL_SECTION_TARGET, ".x86_code", 
                         COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_TARGET, 
                         &x86_sect, &x86_index);
  
  // Create an ARM64 specific section
  coil_obj_set_target_defaults(&obj, COIL_PU_CPU, COIL_CPU_ARM64, 
                              COIL_CPU_ARM_NEON | COIL_CPU_ARM_SVE);
  
  coil_section_t arm_sect;
  coil_section_init(&arm_sect, 1024);
  coil_section_write(&arm_sect, (coil_byte_t *)"ARM64 code", 10, NULL);
  
  coil_u16_t arm_index;
  coil_obj_create_section(&obj, COIL_SECTION_TARGET, ".arm_code", 
                         COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_TARGET, 
                         &arm_sect, &arm_index);
  
  // Save object to file
  int fd = open("multi_target.coil", O_RDWR | O_CREAT | O_TRUNC, 0644);
  coil_obj_save_file(&obj, fd);
  close(fd);
  
  // Cleanup
  coil_section_cleanup(&x86_sect);
  coil_section_cleanup(&arm_sect);
  coil_obj_cleanup(&obj);
  
  return 0;
}
```

### Instruction Encoding

```c
#include <coil/instr.h>

// Create a simple instruction sequence
void encode_instructions(coil_section_t *sect) {
  // NOP instruction
  coil_instr_encode(sect, COIL_OP_NOP);
  
  // MOV instruction with operands
  coil_instrflag_encode(sect, COIL_OP_MOV, COIL_INSTRFLAG_NONE);
  
  // Register operand (destination)
  coil_operand_encode(sect, COIL_TYPEOP_REG, COIL_VAL_I32, COIL_MOD_NONE);
  coil_u32_t reg_dest = 0; // EAX
  coil_operand_encode_data(sect, &reg_dest, sizeof(reg_dest));
  
  // Immediate operand (source)
  coil_operand_encode(sect, COIL_TYPEOP_IMM, COIL_VAL_I32, COIL_MOD_CONST);
  coil_i32_t value = 42;
  coil_operand_encode_data(sect, &value, sizeof(value));
}
```

### Working with COIL Code and Native Machine Code

```c
#include <coil/obj.h>
#include <coil/sect.h>

int main() {
  // Initialize object
  coil_object_t obj;
  coil_obj_init(&obj, COIL_OBJ_INIT_DEFAULT);
  
  // 1. Create a section with COIL code for x86_64 target
  // Set target information that will be used for compilation
  coil_obj_set_target_defaults(&obj, COIL_PU_CPU, COIL_CPU_x86_64, 
                              COIL_CPU_X86_AVX2 | COIL_CPU_X86_SSE4_2);
  
  coil_section_t coil_sect;
  coil_section_init(&coil_sect, 1024);
  
  // Add COIL instructions using the COIL instruction encoding API
  // (Simplified for example)
  coil_instr_encode(&coil_sect, COIL_OP_NOP);
  coil_instrflag_encode(&coil_sect, COIL_OP_MOV, COIL_INSTRFLAG_NONE);
  
  // Add to object as COIL code section (no TARGET flag)
  coil_u16_t coil_index;
  coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".coil_code", 
                         COIL_SECTION_FLAG_CODE, // No TARGET flag - this is COIL code
                         &coil_sect, &coil_index);
  
  // 2. Create a section with native x86_64 machine code
  coil_section_t x86_native;
  coil_section_init(&x86_native, 1024);
  
  // Add pre-compiled native machine code bytes (example)
  // In a real scenario, this would be actual machine code bytes for x86_64
  unsigned char x86_machine_code[] = {
    0x48, 0x89, 0xf8,       // mov rax, rdi
    0x48, 0x83, 0xc0, 0x01, // add rax, 1
    0xc3                    // ret
  };
  coil_section_write(&x86_native, x86_machine_code, sizeof(x86_machine_code), NULL);
  
  // Add to object as native code section WITH the TARGET flag
  coil_u16_t x86_index;
  coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".x86_native", 
                         COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_TARGET, // TARGET flag indicates native code
                         &x86_native, &x86_index);
  
  // 3. Create a section with native ARM64 machine code
  coil_obj_set_target_defaults(&obj, COIL_PU_CPU, COIL_CPU_ARM64, 
                              COIL_CPU_ARM_NEON | COIL_CPU_ARM_SVE);
  
  coil_section_t arm_native;
  coil_section_init(&arm_native, 1024);
  
  // Add pre-compiled ARM64 machine code bytes (example)
  // In a real scenario, this would be actual machine code bytes for ARM64
  unsigned char arm_machine_code[] = {
    0x8b, 0x00, 0x00, 0x91, // add x11, x0, #0
    0x20, 0x00, 0x80, 0xd2, // mov x0, #1
    0xc0, 0x03, 0x5f, 0xd6  // ret
  };
  coil_section_write(&arm_native, arm_machine_code, sizeof(arm_machine_code), NULL);
  
  // Add to object as native ARM code section WITH the TARGET flag
  coil_u16_t arm_index;
  coil_obj_create_section(&obj, COIL_SECTION_PROGBITS, ".arm_native", 
                         COIL_SECTION_FLAG_CODE | COIL_SECTION_FLAG_TARGET, // TARGET flag indicates native code
                         &arm_native, &arm_index);
  
  // Save object to file
  int fd = open("multi_target.coil", O_RDWR | O_CREAT | O_TRUNC, 0644);
  coil_obj_save_file(&obj, fd);
  close(fd);
  
  // Cleanup
  coil_section_cleanup(&coil_sect);
  coil_section_cleanup(&x86_native);
  coil_section_cleanup(&arm_native);
  coil_obj_cleanup(&obj);
  
  return 0;
}
```

## API Documentation

Detailed API documentation is available in the [docs/](docs/) directory, including:
- [API Reference](docs/api-reference.md)
- [File Format Specification](docs/file-format.md)
- [Instruction Set](docs/instructions.md)
- [Target Metadata System](docs/target-metadata.md)

## License

This project is licensed under the terms of the [LICENSE](LICENSE) file.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute to the project.

## Contact

For questions or support, please open an issue on the [GitHub repository](https://github.com/coil-project/libcoil-dev/issues).