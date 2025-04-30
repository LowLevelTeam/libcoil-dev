# libcoil-dev - COIL Development Library

The COIL Development Library (libcoil-dev) provides a comprehensive API for working with the COIL (Computer Oriented Intermediate Language) format, supporting manipulation of intermediate representations, native code integration, and cross-platform development.

## Overview

libcoil-dev is the core library for the COIL toolchain, providing functionality to:

- Read, write, and manipulate COIL files and intermediate representations
- Support native code embedding with architecture metadata
- Define a clean API for processing COIL instructions and sections
- Enable cross-platform compilation and optimization

This library serves as the foundation for other COIL tools, including:
- COIL C Compiler (ccc)
- COIL Object Processor (cop)
- COIL Latent Linker (cll)

## Features

- **COIL File Format**: API for reading and writing the COIL intermediate representation format
- **Native Code Support**: Ability to embed native machine code with architecture metadata
- **Cross-Platform Targeting**: Support for multiple processing units and architectures
- **Instruction Set**: Complete API for COIL instruction encoding and decoding
- **Memory Management**: Optimized memory handling for sections and objects
- **Error Handling**: Comprehensive error reporting and handling

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

## API Documentation

Detailed API documentation is available in the [docs/](docs/) directory, including:
- [API Reference](docs/api-reference.md)
- [File Format Specification](docs/file-format.md)
- [Instruction Set](docs/instructions.md)

## License

This project is licensed under the terms of the [LICENSE](LICENSE) file.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute to the project.

## Contact

For questions or support, please open an issue on the [GitHub repository](https://github.com/coil-project/libcoil-dev/issues).