#pragma once

#include "coil/op.hpp"

namespace coil {
  // ---------------- Definitions ---------------- //
  
  // instruction opcode
  enum op : uint8_t {  
    // Control Flow
    OP_RANGE_CF_START = 0x00,
    OP_RANGE_CF_END = 0x1F,

    OP_NOP  = 0x00, // InstrVoid
    OP_BR   = 0x01, // InstrBr
    OP_CALL = 0x02, // InstrBr
    OP_RET  = 0x03, // InstrVoid
    OP_CMP  = 0x04, // InstrDouble
    OP_TEST = 0x05, // InstrUnary

    OP_RANGE_MEM_START = 0x20,
    OP_RANGE_MEM_END = 0x3F,

    OP_MOV = 0x20,   // InstrDouble
    OP_PUSH = 0x21,  // InstrUnary
    OP_POP = 0x22,   // InstrUnary
    OP_LEA = 0x23,   // InstrDouble
    OP_SCOPE = 0x24, // InstrVoid
    OP_SCOPL = 0x25, // InstrVoid
    OP_VAR = 0x26,   // InstrDef
    OP_XCHG = 0x27,  // InstrDouble
    OP_CAS = 0x28,   // InstrDouble

    OP_RANGE_ARITH_START = 0x40,
    OP_RANGE_ARITH_END = 0x5F,

    OP_ADD = 0x40, // InstrTriple
    OP_SUB = 0x41, // InstrTriple
    OP_MUL = 0x42, // InstrTriple
    OP_DIV = 0x43, // InstrTriple
    OP_MOD = 0x44, // InstrTriple
    OP_INC = 0x45, // InstrDouble
    OP_DEC = 0x46, // InstrDouble

    OP_RANGE_BIT_START = 0x60,
    OP_RANGE_BIT_END = 0x6F,

    OP_AND = 0x60, // InstrTriple
    OP_OR  = 0x61, // InstrTriple
    OP_XOR = 0x62, // InstrTriple
    OP_NOT = 0x63, // InstrDouble
    OP_SHL = 0x64, // InstrDouble
    OP_SHR = 0x65, // InstrDouble
    OP_SAL = 0x66, // InstrDouble
    OP_SAR = 0x67, // InstrDouble
    OP_POPCNT = 0x68, // InstrDouble

    OP_RANGE_MD_START = 0x70,
    OP_RANGE_MD_END = 0x7F,

    OP_GETE = 0x70, // InstrDouble
    OP_SETE = 0x71, // InstrDouble
    OP_DOT  = 0x72, // InstrTriple
    OP_CROS = 0x73, // InstrTriple
    OP_NORM = 0x74, // InstrTriple
    OP_LEN  = 0x75, // InstrDouble
    OP_TRAN = 0x76, // InstrDouble
    OP_INV  = 0x77, // InstrDouble

    OP_RANGE_RAND_START = 0x80,
    OP_RANGE_RAND_END = 0x8F,

    OP_RANGE_RESV_START = 0x90,
    OP_RANGE_RESV_END = 0xAF,

    OP_RANGE_TYPE_START = 0xB0
    OP_RANGE_TYPE_END = 0xBF

    OP_RANGE_EXT_START = 0xC0
    OP_RANGE_EXT_END = 0xEF

    OP_RANGE_PU_START = 0xC0
    OP_RANGE_PU_END = 0xCF

    // Depends on selected processing unit

    OP_RANGE_ARCH_START = 0xD0,
    OP_RANGE_ARCH_END = 0xDF,

    // Depends on selected architecture

    OP_RANGE_DIR_START = 0xE0
    OP_RANGE_DIR_END = 0xFF

    OP_PPDEF  = 0xE0, // conditional compilation
    OP_PPUDEF = 0xE1, // conditional compilation
    OP_PPIF   = 0xE2, // conditional compilation
    OP_PPELIF = 0xE3, // conditional compilation
    OP_PPELSE = 0xE4, // conditional compilation
    OP_PPEIF  = 0xE5, // conditional compilation
    OP_PPINC  = 0xE6, // include
    OP_PPSEC  = 0xE7, // section
    OP_PPDATA = 0xE8, // raw data insertion
    OP_PPPADD = 0xE9, // padd program to byte offset
    
    OP_PPTARG = 0xFC,
    OP_PPABI = 0xFD,

    OP_PPEXT = 0xFF, // similar to gnu __attribute__ for extensions beyond standard COIL 
  } op_t;

  enum cpu_op : uint8_t {
    OP_INT      = 0xC0,
    OP_IRET    = 0xC1,
    OP_CLI     = 0xC2,
    OP_STI     = 0xC3,
    OP_SYSCALL = 0xC4, // int 0x80 on systems without a direct supervisor interrupt
    OP_SYSRET  = 0xC5, 
    OP_RDTSC   = 0xC6,
  };
    enum cpu_x86_op : uint8_t {
      CPUID = 0xD0,
      RDMSR = 0xD1,
      WRMSR = 0xD2,
      LGDT  = 0xD3,
      SGDT  = 0xD4,
      LIDT  = 0xD5,
      SIDT  = 0xD6,
      RDPMC = 0xD7,
    };
    enum cpu_arm_op : uint8_t {
      SEV = 0xD0,
      WFE = 0xD1,
      MRS = 0xD2,
      MSR = 0xD3,
    };
  enum gpu_op : uint8_t {
    // TODO
  };
    enum gpu_nv_op : uint8_t {
    };
    enum gpu_amd_op : uint8_t {
    };

  // ---------------- Helper Functionality ---------------- //
  int is_runtime(uint8_t op) { return !is_dir(); }
  int is_cf(uint8_t op) { return op >= OP_RANGE_CF_START && op <= OP_RANGE_CF_END; }
  int is_mem(uint8_t op) { return op >= OP_RANGE_MEM_START && op <= OP_RANGE_MEM_END; }
  int is_arith(uint8_t op) { return op >= OP_RANGE_ARITH_START && op <= OP_RANGE_ARITH_END; }
  int is_bmi(uint8_t op) { return op >= OP_RANGE_BIT_START && op <= OP_RANGE_BIT_END; }
  int is_md(uint8_t op) { return op >= OP_RANGE_MD_START && op <= OP_RANGE_MD_END;  }
  int is_rand(uint8_t op) { return op >= OP_RANGE_RAND_START && op <= OP_RANGE_RAND_END;  }
  int is_reserved(uint8_t op) { return op >= OP_RANGE_RESV_START && op <= OP_RANGE_RESV_END;  }
  int is_type(uint8_t op) { return op >= OP_RANGE_TYPE_START && op <= OP_RANGE_TYPE_END;  }
  int is_ext(uint8_t op) { return op >= OP_RANGE_EXT_START && op <= OP_RANGE_EXT_END;  }
  int is_pu(uint8_t op) { return op >= OP_RANGE_PU_START && op <= OP_RANGE_PU_END;  }
  int is_arch(uint8_t op) { return op >= OP_RANGE_ARCH_START && op <= OP_RANGE_ARCH_END;  }
  int is_mode(uint8_t op) { return op >= OP_RANGE_MODE_START && op <= OP_RANGE_MODE_END;  }
  int is_dir(uint8_t op) { return op >= OP_RANGE_DIR_START && op <= OP_RANGE_DIR_END;  }

  // ---------------- Instructions ---------------- //
  // Base instruction. To differentiate the child utilize the opcode
  typedef op_t Instr;

  // Control Flow
  struct InstrBr {
    op_t opcode;
    uint8_t operand_count;

    union {
      OpFw fw;
      OpPlt plt;
    } target;

    OpParam param0; // cond_p
  };

  // Definition
  struct InstrDef {
    op_t opcode;

    union ArithOp {
      OpFw fw;
      OpPlt plt;
      OpCOIL coil;
    } op0;

    OpParam param0;
    OpParam param1;
    OpParam param2;
    OpParam param3;
  };

  // Default
  struct InstrUnary {
    op_t opcode;

    union ArithOp { 
      OpFw fw;
      OpPlt plt;
      OpCOIL coil;
    } op0;

    OpParam param0;
    OpParam param1;
    OpParam param2;
    OpParam param3;
  };
  struct InstrDouble {
    op_t opcode;

    union ArithOp { 
      OpFw fw;
      OpPlt plt;
      OpCOIL coil;
    };
    ArithOp op0;
    ArithOp op1;

    OpParam param0;
    OpParam param1;
    OpParam param2;
    OpParam param3;
  };
  struct InstrTriple {
    op_t opcode;

    union ArithOp { 
      OpFw fw;
      OpPlt plt;
      OpCOIL coil;
    };
    ArithOp op0;
    ArithOp op1;
    ArithOp op2;

    OpParam param0;
    OpParam param1;
    OpParam param2;
    OpParam param3;
  };
};
