# COIL Instruction Set Architecture

## Overview

The COIL Instruction Set Architecture (ISA) is designed as an intermediate representation for compilers, offering a balance between high-level abstraction and low-level control. It features a comprehensive type system and operations that map efficiently to modern CPU architectures.

## Type System

### Basic Types

COIL supports a wide range of numeric types:

| Category | Types | Description |
|----------|-------|-------------|
| Signed Integers | I8, I16, I32, I64, I128 | 8-bit to 128-bit signed integers |
| Unsigned Integers | U8, U16, U32, U64, U128 | 8-bit to 128-bit unsigned integers |
| Floating Point | F8E5M2, F8E4M3, F16, FB16, F32, FT32, F64, F80, F128 | Various floating point formats |
| Vectors | V128, V256, V512 | Fixed-width vector types for same type multiple element types |
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

COIL instructions have a flexible format depending on the operation type:

1. **Void Instructions**: Just an opcode (e.g., NOP)
2. **Unary Instructions**: Opcode + single operand (e.g., PUSH, POP)
3. **Binary Instructions**: Opcode + two operands (e.g., MOV, CMP)
4. **Ternary Instructions**: Opcode + three operands (e.g., ADD, SUB)

Each instruction may also optionally include a condition parameter for conditional execution.

## Instruction Categories

### Control Flow Operations (0x00-0x1F)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x00 | NOP | No operation |
| 0x01 | BR | Branch to location |
| 0x02 | CALL | Call function |
| 0x03 | RET | Return from function |
| 0x04 | CMP | Compare values |
| 0x05 | TEST | Test value (logical AND with flags only) |

### Memory Operations (0x20-0x3F)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x20 | MOV | Move data between locations |
| 0x21 | PUSH | Push value onto stack |
| 0x22 | POP | Pop value from stack |
| 0x23 | LEA | Load effective address |
| 0x24 | SCOPE | Begin scope |
| 0x25 | SCOPL | End scope |
| 0x26 | VAR | Define variable |
| 0x27 | XCHG | Exchange values |
| 0x28 | CAS | Compare and swap (atomic) |

### Arithmetic Operations (0x40-0x5F)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x40 | ADD | Addition |
| 0x41 | SUB | Subtraction |
| 0x42 | MUL | Multiplication |
| 0x43 | DIV | Division |
| 0x44 | MOD | Modulo |
| 0x45 | INC | Increment |
| 0x46 | DEC | Decrement |

### Bitwise Operations (0x60-0x6F)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x60 | AND | Bitwise AND |
| 0x61 | OR | Bitwise OR |
| 0x62 | XOR | Bitwise XOR |
| 0x63 | NOT | Bitwise NOT |
| 0x64 | SHL | Shift left (logical) |
| 0x65 | SHR | Shift right (logical) |
| 0x66 | SAL | Shift arithmetic left |
| 0x67 | SAR | Shift arithmetic right |
| 0x68 | POPCNT | Population count |

### Vector/Matrix Operations (0x70-0x7F)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x70 | GETE | Get element from vector/matrix |
| 0x71 | SETE | Set element in vector/matrix |
| 0x72 | DOT | Dot product |
| 0x73 | CROSS | Cross product |
| 0x74 | NORM | Normalize vector |
| 0x75 | LEN | Vector length/magnitude |
| 0x76 | TRANS | Matrix transpose |
| 0x77 | INV | Matrix inversion |

### Directive Operations (0xE0-0xFF)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xE0 | PPDEF | Define preprocessor macro |
| 0xE1 | PPUDEF | Undefine preprocessor macro |
| 0xE2 | PPIF | Preprocessor if |
| 0xE3 | PPELIF | Preprocessor else if |
| 0xE4 | PPELSE | Preprocessor else |
| 0xE5 | PPEIF | Preprocessor end if |
| 0xE6 | PPINC | Include file |
| 0xE7 | PPSEC | Begin section |
| 0xE8 | PPDATA | Raw data insertion |
| 0xE9 | PPPADD | Pad program to byte offset |

## Platform-Specific Extensions

COIL supports architecture-specific instructions through extension opcodes:

### CPU Common Operations (0xC0-0xCF)

Instructions like INT, IRET, CLI, STI for general CPU control.

### x86-Specific Extensions (0xD0-0xDF)

Instructions like CPUID, RDMSR, WRMSR specific to x86 architecture.

### ARM-Specific Extensions (0xD0-0xDF)

Instructions like SEV, WFE, MRS, MSR specific to ARM architecture.

## Encoding Conventions

### Basic Instruction Format

```
[opcode: uint8_t][operand count: uint8_t][operands: variable]
```

### Operand Encoding

```
[type opcode: uint8_t][type control: uint8_t][type data: variable][operand data: variable]
```

## Conditional Execution

Many instructions support conditional execution based on flags:

- EQ: Equal
- NEQ: Not equal
- LT: Less than
- LTE: Less than or equal
- GT: Greater than
- GTE: Greater than or equal

This allows for predicated execution without explicit branching.

## Memory Model

COIL uses a linearized memory model with support for:

- Stack-based allocation (PUSH/POP)
- Scope-based memory management (SCOPE/SCOPL)
- Variable declarations (VAR)
- Atomic operations (Type system atomic control to turn normal operations into atomic counterparts)

## ABI Considerations

While COIL itself is architecture-neutral, it provides directives for specifying:

- Target platform (PPTARG)
- ABI conventions (PPABI)
- Extension attributes (PPEXT)

These help when lowering COIL to specific machine code.