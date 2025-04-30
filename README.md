# COIL - Computer Oriented Intermediate Language

COIL (Computer Oriented Intermediate Language) is a modern toolchain for compilation, optimization, and cross-platform development. It provides a unified approach to generating machine code for diverse hardware targets while maintaining portability.

## Overview

COIL is a comprehensive toolchain that provides a platform-independent intermediate representation for code compilation and optimization. Similar to LLVM's approach, COIL defines a binary machine language that serves as a common format between front-end compilers and back-end machine code generators.

The COIL project's key features include:
- A well-defined intermediate representation for compiled code
- Cross-platform compilation and optimization
- Support for multiple processing units (CPU, GPU, etc.) and architectures
- Embedding of native machine code with metadata for specific targets
- A complete toolchain from source code to executable binaries

## Architecture

COIL follows a streamlined, cross-platform compilation process:

```
Source Code → COIL C Compiler (ccc) → COIL IR Files (.coil) → COIL Object Processor (COP) → COIL Native Objects (.coilo) → COIL Latent Linker (cll) → Executable
```

In this architecture:
1. Front-end compilers like CCC translate source code to the COIL intermediate representation
2. COP processes COIL IR into native machine code, but preserves it in the cross-platform COIL Object format (.coilo)
3. CLL handles the final transformation to platform-specific executable formats, managing all native object format complexities

## Native Code Support

COIL introduces a powerful approach to handling native code across multiple platforms:

### Double Value Architecture Specification

COIL uses a dual-level specification for target architectures:
1. **Processing Unit (PU)**: The type of processor (CPU, GPU, TPU, etc.)
2. **Architecture**: The specific architecture within that PU category (x86-64, ARM64, NVIDIA PTX, etc.)

This approach allows COIL to:
- Support a wide range of hardware targets
- Clearly separate platform-specific code
- Enable cross-compilation between different architectures
- Expand to new processing units as they emerge

### Native Code in COIL Objects

COIL object files can contain native machine code alongside the IR:
- Each section can have associated metadata specifying its target PU and architecture
- Multiple native code sections can exist for different targets
- Feature flags allow fine-grained control over architecture-specific optimizations

The `.coilo` extension is used for COIL object files that contain native code, though the underlying format is the same as regular COIL files with additional metadata.

## Components

### Libraries

- **libcoil**: Core library for reading, writing, and manipulating COIL files and objects with native code support.
- **libcop**: API for the COIL Object Processor functionality, translating COIL IR to native code.
- **libcll**: API for the COIL Latent Linker functionality, handling native object formats internally.

### Tools

- **ccc (COIL C Compiler)**: Front-end compiler that translates C code to COIL intermediate representation.
- **cop (COIL Object Processor)**: Processes COIL IR files into COIL objects containing native machine code (.coilo).
- **cll (COIL Latent Linker)**: Transforms COIL objects into platform-specific executables for various targets.

## File Formats

- **.coil**: The COIL intermediate representation file format
- **.coilo**: COIL object file format that contains native machine code with architecture metadata

## Supported Platforms

### Processing Units
- CPU (Central Processing Unit)
- GPU (Graphics Processing Unit)
- TPU (Tensor Processing Unit)
- NPU (Neural Processing Unit)
- DSP (Digital Signal Processor)
- FPGA (Field-Programmable Gate Array)

### CPU Architectures
- x86 (32-bit and 64-bit)
- ARM (32-bit and 64-bit)
- RISC-V (32-bit and 64-bit)
- PowerPC
- MIPS
- WebAssembly

### GPU Architectures
- NVIDIA (PTX, CUDA)
- AMD (GCN, RDNA)
- Intel (Gen9, Xe)
- Vulkan (SPIR-V)
- OpenCL
- Metal

## Getting Started

### Prerequisites

- make
- C99 compatible compiler
- Git

### Build

```bash
git clone https://github.com/coil-project/coil.git
cd coil
mkdir build && cd build
cmake ..
make
```

### Example Usage

#### Basic Compilation

```bash
# Compile C to COIL IR
ccc -o program.coil program.c

# Process COIL IR to COIL object with native code
cop -o program.coilo program.coil

# Link to executable for the current platform
cll -o program program.coilo
```

#### Cross-Platform Compilation

```bash
# Compile for a specific target
cop --pu=CPU --arch=x86-64 -o program_x86_64.coilo program.coil

# Compile for multiple targets in a single object
cop --pu=CPU --arch=x86-64 --pu=CPU --arch=ARM64 -o program_multi.coilo program.coil

# Link to executable for Windows x86-32
cll -o program.exe --format=PE --pu=CPU --arch=x86-32 program.coilo
```

## Advanced Features

### Per-Section Architecture Targeting

COIL allows different sections of code to target different architectures:

```bash
# Create a COIL object with mixed architecture sections
cop --section=.text --pu=CPU --arch=x86-64 \
    --section=.gpu_code --pu=GPU --arch=NV_CUDA \
    -o mixed_program.coilo program.coil
```

### Feature-Specific Optimizations

Enable specific architecture features for optimized code:

```bash
# Generate code optimized for AVX2
cop --pu=CPU --arch=x86-64 --features=AVX2 -o program_avx2.coilo program.coil

# Generate code for ARM with NEON SIMD
cop --pu=CPU --arch=ARM64 --features=NEON -o program_neon.coilo program.coil
```

## Development Status

The COIL project is currently in active development:

- ✅ libcoil: COIL file format reading and writing support with native code capabilities
- 🔄 COP: Implementation with multiple target architecture support
- 🔄 CLL: Design phase with native object format support integrated

## License

This project is licensed under [LICENSE](LICENSE) - see the file for details.

## Documentation

See the [docs/](docs/) directory for detailed documentation including:
- Architecture documentation
- API references
- File format specifications
- Developer guides
- Example code