/**
* @file builder.hpp
* @brief Builds COIL instructions and can write them to a stream
*/

#pragma once
#include "coil/types.hpp"
#include "coil/instr.hpp"

#include <vector>

namespace coil {
  struct Builder {
    Builder() = default;
    Builder(size_t bufsize) : vec(bufsize) {}

    void reserve(size_t bufsize) { vec.reserve(bufsize); }

    void push_header(Opcode op) {
      vec.push_back((uint8_t)op);
    }
    void push_header(Opcode op, uint8_t opcount) {
      vec.push_back((uint8_t)op);
      vec.push_back(opcount);
    }

    void push_operand(
      OperandType optype, ValueType valtype, TypeModifier mod, 
      void *data
    ) {
      vec.push_back((uint8_t)optype);
      vec.push_back((uint8_t)valtype);
      vec.push_back((uint8_t)mod);

      push_value(optype, valtype, data);
    }
    void push_operand_off(
      OperandType optype, ValueType valtype, TypeModifier mod, 
      i64 index, i64 scale, i64 displacement,
      void *data
    ) {
      vec.push_back((uint8_t)OperandType::Off);
      vec.push_back((uint8_t)optype);
      vec.push_back((uint8_t)valtype);
      vec.push_back((uint8_t)mod);

      push64(index);
      push64(scale);
      push64(displacement);

      push_value(optype, valtype, data);
    }

    void push_value(OperandType type, ValueType valtype, void *data) {
      if (type == OperandType::None) {
        return;
      } else if (type == OperandType::Reg) {
        push32(*((u32*)data));
      } else if (
        type == OperandType::Var ||
        type == OperandType::Exp ||
        type == OperandType::Sym
      ) {
        push64(*((u64*)data));
      } else if (type == OperandType::Imm) {
        push_value_imm(valtype, data)
      }
    }
    void push_value_imm(ValueType valtype, void *data) {
      switch (valtype) {
        case Flag0:
        case Flag1:
        case Flag2:
        case Flag3:
        case Bit:
        case I8:
        case U8:
          vec.push_back(*((u8*)data));
          return;
        case I16:
        case U16:
          push16(*((u16*)data));
          return;
        case REG:
        case I32:
        case U32:
          push32(*((u32*)data));  
          return;
        case VAR:
        case SYM:
        case EXP:
        case STR:
        case PTR:
        case SIZE:
        case SSIZE:
        case I64:
        case U64:
          push64(*((u64*)data));
          return;
        case F32:
          pushf32(*((f32*)data));
          return;
        case F64:
          pushf64(*((f32*)data));
          return;
        case Void:
        default:
      };
    }
  private:
    void push16(u16 value) {
      vec.push_back(static_cast<u8>(value >> (0 * 8)));
      vec.push_back(static_cast<u8>(value >> (1 * 8)));
    }  
    void push32(u32 value) {
      vec.push_back(static_cast<u8>(value >> (0 * 8)));
      vec.push_back(static_cast<u8>(value >> (1 * 8)));
      vec.push_back(static_cast<u8>(value >> (2 * 8)));
      vec.push_back(static_cast<u8>(value >> (3 * 8)));
    }
    void push64(u64 value) {
      vec.push_back(static_cast<u8>(value >> (0 * 8)));
      vec.push_back(static_cast<u8>(value >> (1 * 8)));
      vec.push_back(static_cast<u8>(value >> (2 * 8)));
      vec.push_back(static_cast<u8>(value >> (3 * 8)));
      vec.push_back(static_cast<u8>(value >> (4 * 8)));
      vec.push_back(static_cast<u8>(value >> (5 * 8)));
      vec.push_back(static_cast<u8>(value >> (6 * 8)));
      vec.push_back(static_cast<u8>(value >> (7 * 8)));
    }
    void pushf32(float value) {
      uint8_t bytes[sizeof(float)];
      std::memcpy(bytes, &value, sizeof(float));
      vec.insert(vec.end(), bytes, bytes + sizeof(float));
    }
    void pushf64(double value) {
      uint8_t bytes[sizeof(double)];
      std::memcpy(bytes, &value, sizeof(double));
      vec.insert(vec.end(), bytes, bytes + sizeof(double));
    }

    std::vector<u8> vec;
  };
};


