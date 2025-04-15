# COIL Instruction Set Architecture

## Overview

The COIL Instruction Set Architecture (ISA) is designed as an intermediate representation for compilers, offering a balance between high-level abstraction and low-level control. It features a comprehensive type system and operations that map efficiently to modern CPU architectures while remaining lightweight and predictable.

## Design Principles

- **Minimalist Design**: Each instruction does exactly one thing
- **Explicit Operations**: No hidden side effects or implicit behaviors
- **Predictable Encoding**: Fixed-size instruction formats where possible
- **Platform Awareness**: Extensible for different hardware targets
- **Zero-Cost Abstractions**: IR operations translate directly to efficient machine code

## Type System

### Basic Types

COIL supports a wide range of numeric types:

| Category | Types | Description |
|----------|-------|-------------|
| Signed Integers | I8, I16, I32, I64, I128 | 8-bit to 128-bit signed integers |
| Unsigned Integers | U8, U16, U32, U64, U128 | 8-bit to 128-bit unsigned integers |
| Floating Point | F8E5M2, F8E4M3, F16, FB16, F32, FT32, F64, F80, F128 | Various floating point formats |
| Vectors | V128, V256, V512 | Fixed-width vector types for SIMD operations |
| Platform Types | PTR, SIZE, SSIZE | Platform-specific types for pointers and sizes |
| Special Types | BIT, VOID | Bit (boolean) and void types |

### Type Controls

Types can be modified with control flags:

- `CONST`: Value is constant (check at compile time for modifications)
- `VOL`: Value is volatile (don't optimize)
- `ATOMIC`: Value has atomicity guarantees
- `REG`: Value is in a register
- `IMM`: Value is an immediate constant
- `VAR`: Value is a variable reference
- `SYM`: Value is a symbol reference
- `EXP`: Value is an expression reference

## Instruction Format

COIL instructions have a straightforward memory layout for efficiency:

1. **Void Instructions**: Just an opcode (1 byte)
```
[opcode: uint8_t]
```

2. **Unary Instructions**: Opcode + operand count + single operand
```
[opcode: uint8_t][opcount: uint8_t][operand: variable]
```

3. **Binary Instructions**: Opcode + operand count + two operands
```
[opcode: uint8_t][opcount: uint8_t][operand1: variable][operand2: variable]
```

4. **Ternary Instructions**: Opcode + operand count + three operands
```
[opcode: uint8_t][opcount: uint8_t][operand1: variable][operand2: variable][operand3: variable]
```

Each instruction may also include a condition parameter for conditional execution.

## Instruction Categories

### Control Flow Operations (0x00-0x1F)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0x00 | NOP | No operation | Void |
| 0x01 | BR | Branch to location | CtxChange |
| 0x02 | CALL | Call function | CtxChange |
| 0x03 | RET | Return from function | VoidParam |
| 0x04 | CMP | Compare values | Binary |
| 0x05 | TEST | Test value (logical AND with flags only) | Binary |

### Memory Operations (0x20-0x3F)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0x20 | MOV | Move data between locations | Binary |
| 0x21 | PUSH | Push value onto stack | Unary |
| 0x22 | POP | Pop value from stack | Unary |
| 0x23 | LEA | Load effective address | Binary |
| 0x24 | SCOPE | Begin scope | VoidParam |
| 0x25 | SCOPL | End scope | VoidParam |
| 0x26 | VAR | Define variable | Binary |
| 0x27 | XCHG | Exchange values | Binary |
| 0x28 | CAS | Compare and swap (atomic) | Ternary |

### Arithmetic Operations (0x40-0x5F)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0x40 | ADD | Addition | Ternary |
| 0x41 | SUB | Subtraction | Ternary |
| 0x42 | MUL | Multiplication | Ternary |
| 0x43 | DIV | Division | Ternary |
| 0x44 | MOD | Modulo | Ternary |
| 0x45 | INC | Increment | Unary |
| 0x46 | DEC | Decrement | Unary |

### Bitwise Operations (0x60-0x6F)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0x60 | AND | Bitwise AND | Ternary |
| 0x61 | OR | Bitwise OR | Ternary |
| 0x62 | XOR | Bitwise XOR | Ternary |
| 0x63 | NOT | Bitwise NOT | Unary |
| 0x64 | SHL | Shift left (logical) | Ternary |
| 0x65 | SHR | Shift right (logical) | Ternary |
| 0x66 | SAL | Shift arithmetic left | Ternary |
| 0x67 | SAR | Shift arithmetic right | Ternary |
| 0x68 | POPCNT | Population count | Binary |

### Vector/Matrix Operations (0x70-0x7F)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0x70 | GETE | Get element from vector/matrix | Ternary |
| 0x71 | SETE | Set element in vector/matrix | Ternary |
| 0x72 | DOT | Dot product | Ternary |
| 0x73 | CROSS | Cross product | Ternary |
| 0x74 | NORM | Normalize vector | Binary |
| 0x75 | LEN | Vector length/magnitude | Binary |
| 0x76 | TRANS | Matrix transpose | Binary |
| 0x77 | INV | Matrix inversion | Binary |

### Directive Operations (0xE0-0xFF)

| Opcode | Mnemonic | Description | Format |
|--------|----------|-------------|--------|
| 0xE0 | PPDEF | Define preprocessor macro | Binary |
| 0xE1 | PPUDEF | Undefine preprocessor macro | Unary |
| 0xE2 | PPIF | Preprocessor if | Binary |
| 0xE3 | PPELIF | Preprocessor else if | Binary |
| 0xE4 | PPELSE | Preprocessor else | Void |
| 0xE5 | PPEIF | Preprocessor end if | Void |
| 0xE6 | PPINC | Include file | PPINC |
| 0xE7 | PPSEC | Begin section | PPSEC |
| 0xE8 | PPDATA | Raw data insertion | PPDATA |
| 0xE9 | PPPADD | Pad program to byte offset | PPPADD |

## Platform-Specific Extensions

COIL supports architecture-specific instructions through extension opcodes organized by processor type:

### CPU Common Operations (0xC0-0xCF)

Instructions like INT, IRET, CLI, STI for general CPU control.

```cpp
namespace coil::Instr::CPU {
    struct INT {
        uint8_t opcode;
        uint8_t opcount;   // 0, 1 or 2
        uint8_t interrupt;
        Operand location;
        OpParam condition;
    };
    
    // Other CPU instructions defined similarly
}
```

### x86-Specific Extensions (0xD0-0xDF)

Instructions like CPUID, RDMSR, WRMSR specific to x86 architecture.

### ARM-Specific Extensions (0xD0-0xDF)

Instructions like SEV, WFE, MRS, MSR specific to ARM architecture.

## Instruction Encoding Examples

### Simple Instruction Encoding

```cpp
// Encoding a NOP instruction
coil::Instr::NOP nop;
nop.opcode = static_cast<uint8_t>(coil::Opcode::NOP);

// Encoding an INC instruction
coil::Instr::Unary inc;
inc.opcode = static_cast<uint8_t>(coil::Opcode::INC);
inc.opcount = 1;
inc.op.top = static_cast<uint8_t>(coil::TypeOpcode::I32);
inc.op.ctrl = coil::TypeControl::VAR;
inc.op.typedata = nullptr;
inc.op.data.VarID = 42;  // Variable ID to increment
```

### Complex Instruction Encoding

```cpp
// Encoding an ADD instruction
coil::Instr::Tenary add;
add.opcode = static_cast<uint8_t>(coil::Opcode::ADD);
add.opcount = 3;

// Destination register
add.opD.top = static_cast<uint8_t>(coil::TypeOpcode::I64);
add.opD.ctrl = coil::TypeControl::REG;
add.opD.typedata = nullptr;
add.opD.data.RegID = 1;  // Register 1

// First source (register)
add.opL.top = static_cast<uint8_t>(coil::TypeOpcode::I64);
add.opL.ctrl = coil::TypeControl::REG;
add.opL.typedata = nullptr;
add.opL.data.RegID = 2;  // Register 2

// Second source (immediate)
add.opR.top = static_cast<uint8_t>(coil::TypeOpcode::I64);
add.opR.ctrl = coil::TypeControl::IMM;
add.opR.typedata = nullptr;
add.opR.data.Imm.i64 = 42;  // Immediate value 42
```

## Conditional Execution

Many instructions support conditional execution based on flags, controlled by an optional parameter:

```cpp
// Conditional MOV if equal
coil::Instr::Binary mov;
mov.opcode = static_cast<uint8_t>(coil::Opcode::MOV);
mov.opcount = 2;
// Set up operands...

// Make the instruction conditional
mov.condition.top = static_cast<uint8_t>(coil::TypeOpcode::PARAMC);
mov.condition.data = static_cast<uint16_t>(coil::TypeParamCond::TPCOND_EQ);
```

Available conditions:
- `TPCOND_EQ`: Equal
- `TPCOND_NEQ`: Not equal
- `TPCOND_LT`: Less than
- `TPCOND_LTE`: Less than or equal
- `TPCOND_GT`: Greater than
- `TPCOND_GTE`: Greater than or equal

## Memory Model

COIL uses a linearized memory model with support for:

- Stack-based allocation (PUSH/POP)
- Scope-based memory management (SCOPE/SCOPL)
- Variable declarations (VAR)
- Atomic operations (Type system atomic control)

## ABI Considerations

While COIL itself is architecture-neutral, it provides directives for specifying:

- Target platform (PPTARG)
- ABI conventions (PPABI)
- Extension attributes (PPEXT)

These help when lowering COIL to specific machine code.