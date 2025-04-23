# COIL Instruction Set Architecture
An explicit and full fledged definition for the COIL instructions defining a format to decode and encode instructions.

### Control Flow

#### NOP
Fullname - No operation
Brief - Does nothing

Maps to a No operation instruction if available or is ignored.

```
[opcode: u8]
```

#### BR
Fullname - Branch
Brief - Conditional jump to address

Optimized for conditional

```
[opcode: u8][condition: u8][op: Operand]
```

#### JMP
Fullname - Jump
Brief - Un-Conditional jump to address

Optimized for unconditional

```
[opcode: u8][op: Operand]
```

#### CALL
Fullname - Call
Brief - Un-Conditional jump to address with stored return address

```
[opcode: u8][condition: u8][op: Operand]
```

#### RET
Fullname - Return
Brief - Un-Conditional jump to stored return address

```
[opcode: u8][condition: u8]
```

#### CMP
Fullname - Compare
Brief - Compare two values, sets flags

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### TEST
Fullname - Test
Brief - Test and with flag values

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

### Memory

#### MOV
Fullname - Move/Copy
Brief - Move/Copy value from source to destination

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### PUSH
Fullname - Push
Brief - Push operand to stack

```
[opcode: u8][condition: u8][op: Operand]
```

#### POP
Fullname - Pop
Brief - Pop operand from stack

```
[opcode: u8][condition: u8][op: Operand]
```

#### LEA
Fullname - Load Effective Address
Brief - Loads the effective address

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

### Arithmetic

#### ADD
Fullname - Addition
Brief - dest += src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### ADD
Fullname - Subtraction
Brief - dest -= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### MUL
Fullname - Multiplication
Brief - dest *= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### DIV
Fullname - Division
Brief - dest /= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### MOD
Fullname - Modulus
Brief - dest %= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### INC
Fullname - Increment
Brief - ++op

```
[opcode: u8][condition: u8][op: Operand]
```

#### DEC
Fullname - Decrement
Brief - (--op)

```
[opcode: u8][condition: u8][op: Operand]
```

#### NEG
Fullname - Negate
Brief - (-op)

```
[opcode: u8][condition: u8][op: Operand]
```

### Bitwise

#### AND
Fullname - And
Brief - dest &= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### OR
Fullname - Or
Brief - dest |= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### XOR
Fullname - Xor
Brief - dest ^= src

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### NOT
Fullname - Not
Brief - (~op)

```
[opcode: u8][condition: u8][op: Operand]
```

#### SHL
Fullname - Shift Left
Brief - ...

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### SHR
Fullname - Shift Right
Brief - ...

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### SAL
Fullname - Shift Arithmetic Left
Brief - ...

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

#### SAR
Fullname - Shift Arithmetic Right
Brief - ...

```
[opcode: u8][condition: u8][op: Operand][op: Operand]
```

### Multi-Dimensional

NotImplementedYet

### Cryptography and Random Numbers

NotImplementedYet

### Type

#### CVT
Fullname - Convert Type
Brief - Converts a type and precision

```
[opcode: u8][value_type: u8][op: Operand][op: Operand]
```

#### SIZE
Fullname - Size of Type
Brief - Store the sizeof the type

```
[opcode: u8][expId: u64][value_type: u8]
```

#### ALLIGN
Fullname - Allign of Type
Brief - Store the Allign of the type

```
[opcode: u8][expId: u64][value_type: u8]
```

### PU
#### CPU
##### Any
###### INT
Fullname - CPU Interrupt
Brief - Cause a CPU Software Interrupt

```
[opcode: u8][condition: u8][value: u8][op: Operand]
```

###### IRET
Fullname - CPU Interrupt Return
Brief - Return from a CPU Software Interrupt

```
[opcode: u8][condition: u8]
```

###### CLI
Fullname - Clear Inerrupt Flag
Brief - Stops CPU interrupts

```
[opcode: u8][condition: u8]
```

###### STI
Fullname - Set Inerrupt Flag
Brief - Starts CPU interrupts

```
[opcode: u8][condition: u8]
```

###### SYSCALL
Fullname - Supervisor Interrupt
Brief - Contact the supervisor

EdgeCases - Could be an alias for a software interrupt and value (like 'int 0x80')

```
[opcode: u8][condition: u8]
```

###### SYSCALL
Fullname - Return from Supervisor Interrupt
Brief - Return from supervisor contact

EdgeCases - Could be an alias for an IRET

```
[opcode: u8][condition: u8]
```

###### RDTSC
Fullname - Read Timestamp Counter
Brief - Read the timestamp counter

```
[opcode: u8][condition: u8][dest: Operand]
```

##### x86

NotImplementedYet

##### ARM

NotImplementedYet

#### GPU
NotImplementedYet

### Directive

#### DEF
Fullname - Define
Brief - Define an expression

```
[opcode: u8][expId: u64][op: Operand]
```

#### UDEF
Fullname - Undefine
Brief - Undefine an expression

```
[opcode: u8][expId: u64]
```

#### ABI
Fullname - Define Application Binary Interface
Brief - Defins an ABI for usage in future functions

NotImplementedYet

#### SPARAM
Fullname - Set Parameter
Brief - Sets a value in a location depending on the ABI

NotImplementedYet

#### GPARAM
Fullname - Get Parameter
Brief - Gets a value in a location depending on the ABI

NotImplementedYet

#### SPARAM
Fullname - Set Parameter
Brief - Sets a value in a location depending on the ABI

NotImplementedYet

#### GPARAM
Fullname - Get Parameter
Brief - Gets a value in a location depending on the ABI

NotImplementedYet



