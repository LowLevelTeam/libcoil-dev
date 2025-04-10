#pragma once


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

    TOP_PARAM3 = 0xFA
    TOP_PARAM2 = 0xFC
    TOP_PARAM1 = 0xFD
    TOP_PARAM0 = 0xFE

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
  
  // ---------------- Helper Functionality ---------------- //
  int is_runtime(uint8_t ctrl) { return !is_compiletime(); }
  int is_compiletime(uint8_t ctrl) { return ctrl & TCTRL_EXP; }

  int is_fw(uint8_t op) { return op >= TOP_FW_RANGE_START && op <= TOP_FW_RANGE_END; }
  int is_plt(uint8_t op) { return op >= TOP_PLT_RANGE_START && op <= TOP_PLT_RANGE_END; }
  int is_opt(uint8_t op) { return op >= TOP_OPT_RANGE_START && op <= TOP_OPT_RANGE_END; }
  int is_coil(uint8_t op) { return op >= TOP_COIL_RANGE_START && op <= TOP_COIL_RANGE_END; }
  int is_param(uint8_t op) { return op >= TOP_P_RANGE_START && op <= TOP_P_RANGE_END;  }
  int is_void(uint8_t op) { return op == 0xFF; }

  // ---------------- Operands ---------------- //
  struct OpFW {
    uint8_t op;
    uint8_t ctrl;
    union {
      // TCTRL_IMM
      uint64_t ImmU[2]; // higher and lower for 128 bit integers
      int64_t ImmI[2]; // higher and lower for 128 bit integers
      long double ImmF; // hopefully 128 bits

      // TCTRL_VAR
      uint64_t VarID;

      // TCTRL_SYM
      uint64_t SymbolTableOffset;

      // TCTRL_EXP
      uint64_t ExpID;

      // TCTRL_REG
      uint32_t RegID;
    } data;
  };

  struct OpFWVec {
    uint8_t op;
    uint8_t ctrl;
    uint8_t eop; // element type opcode
    // for composite types there will have to be some type data
    // fixed width vectors will not support vector types
    // 
    union {
      // TCTRL_IMM
      uint64_t Imm[8]; // 512 bit vector

      // TCTRL_VAR
      uint64_t VarID;

      // TCTRL_SYM
      uint64_t SymbolTableOffset;

      // TCTRL_EXP
      uint64_t ExpID;

      // TCTRL_REG
      uint32_t RegID;
    } data;
  };

  // struct OpComp; struct OpAComp; struct OpAlias;

  struct OpPlt {
    uint8_t op;
    uint8_t ctrl;
    union {
      // TCTRL_IMM
      uint64_t ImmU; // immediate SIZE
      int64_t ImmI; // immediate SSIZE
      void *ImmP; // immediate pointer

      // TCTRL_VAR
      uint64_t VarID;

      // TCTRL_SYM
      uint64_t SymbolTableOffset;

      // TCTRL_EXP
      uint64_t ExpID;

      // TCTRL_REG
      uint32_t RegID;
    } data;
  };

  struct OpOpt {
    uint8_t op;
    uint8_t ctrl;
    union {
      bool bit;
    } data;
  };
  
  struct OpCOIL {
    uint8_t op;
    uint8_t ctrl;
    // because the value for COIL types is the same no immediate is mentioned
    // i.e. a value at TCTRL_VAR utilizes the VarID and the immediate value in the case of
    //      op = [TOP_VAR][TCTRL_IMM] the immediate would also be VarID
    union {
      // TCTRL_VAR
      uint64_t VarID;

      // TCTRL_SYM
      uint64_t SymbolTableOffset;

      // TCTRL_EXP
      uint64_t ExpID;

      // TCTRL_REG
      uint32_t RegID;
    } data;
  };

  struct OpParam {
    uint8_t op;
    uint8_t ctrl;
    // Used to control execution of instructions
    enum cond_p {
      COND_EQ,  // equal
      COND_NEQ, // not equal
      COND_GT,  // greater than
      COND_GTE, // greater than or equal
      COND_LT,  // less than
      COND_LTE, // less than or equal
      COND_O,   // overflow
      COND_NO,  // no overflow
      COND_C,   // carry
      COND_NC,  // no carry
    };
    // Used to control arithmetic instructions
    enum arith_p {
      ARITH_SAT, // saturate
    };
    union {
      uint8_t Imm;
      uint64_t ExpID;
    } data;
  };

  struct OpVOID {
    uint8_t op;
    uint8_t ctrl;
  };
};