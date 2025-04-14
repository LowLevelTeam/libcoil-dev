#pragma once

#include "coil/op.hpp"

namespace coil {
  // ---------------- Definitions ---------------- //
  // type opcode  
  enum top : uint8_t {
    // Fixed Width
    TOP_FW_RANGE_START = 0x00,
    TOP_FW_RANGE_END   = 0x9F,

    TOP_I8 = 0x00,
    TOP_I16 = 0x01,
    TOP_I32 = 0x02,
    TOP_I64 = 0x03,
    TOP_I128 = 0x04,

    TOP_U8 = 0x10,
    TOP_U16 = 0x11,
    TOP_U32 = 0x12,
    TOP_U64 = 0x13,
    TOP_U128 = 0x14,

    TOP_F8e5m2 = 0x20,
    TOP_F8e4m3 = 0x21,
    TOP_F16 = 0x22,
    TOP_FB16 = 0x23,
    TOP_F32 = 0x24,
    TOP_FT32 = 0x25,
    TOP_F64 = 0x26,
    TOP_F80 = 0x27,
    TOP_F128 = 0x28,

    // Fixed Width Vector
    TOP_V128 = 0x50,
    TOP_V256 = 0x51,
    TOP_V512 = 0x52,

    // Composite (TODO)
    // TOP_COMP_RANGE_START = 0xC0,
    // TOP_COMP_RANGE_END   = 0xCF,

    // TOP_STRCT  = 0xC0, // structure of types
    // TOP_ASTRCT = 0xC1, // anonyomus/inline structure of types
    // TOP_UNION  = 0xC2, // union of types
    // TOP_AUNION = 0xC3, // anonyomus/inline union of types
    // TOP_PACK   = 0xC4, // pack of types (structure with no padding)
    // TOP_APACK  = 0xC5, // anonyomus/inline pack of types
    // TOP_ALIAS  = 0xC6, // Alias type (real type is stored at type id)

    // platform
    TOP_PLT_RANGE_START = 0xD0,
    TOP_PLT_RANGE_END   = 0xDF,

    TOP_PTR = 0xD0, // platform pointer
    TOP_SIZE = 0xD1, // largest unsigned integer
    TOP_SSIZE = 0xD2, // largest signed integer

    // optimized
    TOP_OPT_RANGE_START = 0xE0,
    TOP_OPT_RANGE_END   = 0xEF,

    TOP_BIT = 0xE0,

    // COIL specific
    TOP_COIL_RANGE_START = 0xF0,
    TOP_COIL_RANGE_END   = 0xF9,

    TOP_VAR = 0xF0, // variable
    TOP_SYM = 0xF1, // symbol
    TOP_EXP = 0xF2, // expression
    TOP_REG = 0xF3, // register

    // parameter
    TOP_P_RANGE_START = 0xFA,
    TOP_P_RANGE_END   = 0xFE,

    TOP_PARAM3 = 0xFA,
    TOP_PARAM2 = 0xFC,
    TOP_PARAM1 = 0xFD,
    TOP_PARAM0 = 0xFE,

    // special
    TOP_VOID = 0xFF
  } top_t;

  // 'type control' controls what data is after the type data
  enum tctrl : uint8_t {
    TCTRL_CONST  = (1 << 0),
    TCTRL_VOL    = (1 << 1),
    TCTRL_ATOMIC = (1 << 2),
    TCTRL_REG    = (1 << 3),
    TCTRL_IMM    = (1 << 4),
    TCTRL_VAR    = (1 << 5),
    TCTRL_SYM    = (1 << 6),
    TCTRL_EXP    = (1 << 7),
  } tctrl_t;

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

    OP_MOV = 0x20,   // InstrDestSrc
    OP_PUSH = 0x21,  // InstrUnary
    OP_POP = 0x22,   // InstrUnary
    OP_LEA = 0x23,   // InstrDestSrc
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

    OP_GETE = 0x70, // InstrTriple
    OP_SETE = 0x71, // InstrTriple
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

    OP_PPDEF  = 0xE0, // InstrDef (ID, Operand)
    OP_PPUDEF = 0xE1, // InstrId
    OP_PPIF   = 0xE2, // InstrIf
    OP_PPELIF = 0xE3, // InstrIf
    OP_PPELSE = 0xE4, // InstrVoid
    OP_PPEIF  = 0xE5, // InstrVoid
    OP_PPINC  = 0xE6, // include
    OP_PPSEC  = 0xE7, // section
    OP_PPDATA = 0xE8, // raw data insertion
    OP_PPPADD = 0xE9, // padd program to byte offset
    
    OP_PPTARG = 0xFC,
    OP_PPABI = 0xFD,

    OP_PPEXT = 0xFF, // similar to gnu __attribute__ for extensions beyond standard COIL 
  } op_t;

  enum cpu_op : uint8_t {
    OP_INT     = 0xC0, // InstrBrInt (branch with value) 
    OP_IRET    = 0xC1, // InstrVoid
    OP_CLI     = 0xC2, // InstrVoid
    OP_STI     = 0xC3, // InstrVoid
    OP_SYSCALL = 0xC4, // InstrBr (int 0x80 on systems without a direct supervisor interrupt)
    OP_SYSRET  = 0xC5, // InstrVoid
    OP_RDTSC   = 0xC6, // ...
  };
    enum cpu_x86_op : uint8_t {
      CPUID = 0xD0, // InstrVoid
      RDMSR = 0xD1,
      WRMSR = 0xD2,
      LGDT  = 0xD3,
      SGDT  = 0xD4,
      LIDT  = 0xD5,
      SIDT  = 0xD6,
      RDPMC = 0xD7,
    };
    enum cpu_arm_op : uint8_t {
      SEV = 0xD0, // InstrVoid
      WFE = 0xD1, // InstrVoid
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
  int is_instr_cf(uint8_t op) { return op >= OP_RANGE_CF_START && op <= OP_RANGE_CF_END; }
  int is_instr_mem(uint8_t op) { return op >= OP_RANGE_MEM_START && op <= OP_RANGE_MEM_END; }
  int is_instr_arith(uint8_t op) { return op >= OP_RANGE_ARITH_START && op <= OP_RANGE_ARITH_END; }
  int is_instr_bmi(uint8_t op) { return op >= OP_RANGE_BIT_START && op <= OP_RANGE_BIT_END; }
  int is_instr_md(uint8_t op) { return op >= OP_RANGE_MD_START && op <= OP_RANGE_MD_END;  }
  int is_instr_rand(uint8_t op) { return op >= OP_RANGE_RAND_START && op <= OP_RANGE_RAND_END;  }
  int is_instr_reserved(uint8_t op) { return op >= OP_RANGE_RESV_START && op <= OP_RANGE_RESV_END;  }
  int is_instr_type(uint8_t op) { return op >= OP_RANGE_TYPE_START && op <= OP_RANGE_TYPE_END;  }
  int is_instr_ext(uint8_t op) { return op >= OP_RANGE_EXT_START && op <= OP_RANGE_EXT_END;  }
  int is_instr_pu(uint8_t op) { return op >= OP_RANGE_PU_START && op <= OP_RANGE_PU_END;  }
  int is_instr_arch(uint8_t op) { return op >= OP_RANGE_ARCH_START && op <= OP_RANGE_ARCH_END;  }
  int is_instr_mode(uint8_t op) { return op >= OP_RANGE_MODE_START && op <= OP_RANGE_MODE_END;  }
  int is_instr_dir(uint8_t op) { return op >= OP_RANGE_DIR_START && op <= OP_RANGE_DIR_END;  }

  int is_type_runtime(uint8_t ctrl) { return !is_compiletime(); }
  int is_type_compiletime(uint8_t ctrl) { return ctrl & TCTRL_EXP; }
  int is_type_imm(uint8_t ctrl) { return ctrl & TCTRL_IMM; }
  int is_type_var(uint8_t ctrl) { return ctrl & TCTRL_VAR; }
  int is_type_sym(uint8_t ctrl) { return ctrl & TCTRL_SYM; }
  int is_type_exp(uint8_t ctrl) { return ctrl & TCTRL_EXP; }
  int is_type_reg(uint8_t ctrl) { return ctrl & TCTRL_REG; }
  int is_type_void(uint8_t ctrl) { return !is_imm() && !is_var() && !is_sym() && !is_exp() && !is_reg(); }

  int is_type_fw(uint8_t op) { return op >= TOP_FW_RANGE_START && op <= TOP_FW_RANGE_END; }
  int is_type_plt(uint8_t op) { return op >= TOP_PLT_RANGE_START && op <= TOP_PLT_RANGE_END; }
  int is_type_opt(uint8_t op) { return op >= TOP_OPT_RANGE_START && op <= TOP_OPT_RANGE_END; }
  int is_type_coil(uint8_t op) { return op >= TOP_COIL_RANGE_START && op <= TOP_COIL_RANGE_END; }
  int is_type_param(uint8_t op) { return op >= TOP_P_RANGE_START && op <= TOP_P_RANGE_END;  }
  int is_type_void(uint8_t op) { return op == 0xFF; }

  // ---------------- Instructions ---------------- //
  struct Operand {
    uint8_t top; // type opcode
    uint8_t ctrl; // type control
    union {
      std::vector<uint8_t> immediate; // immediate data    
      uint64_t variableID;
      uint64_t symbol;
      uint64_t ExpID;
      uint32_t RegisterID;
    } typedata;
  };

  struct Instr {
    uint8_t opcode = 0x00;
    uint8_t operand_count = 0;
    std::vector<Operand> oeprands;

    Instr(uint8_t op, uint8_t opcount);

    // Helper
    uint8_t GetOpCount() { return this->operand_count; }
  };
};
