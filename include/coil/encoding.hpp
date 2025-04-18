/**
* @file encoding.hpp
* @brief COIL instruction encoding and decoding
* 
* Defines a standard binary format for COIL instructions and provides
* utilities for encoding/decoding between in-memory and binary representations.
*/

#pragma once
#include "coil/instr.hpp"
#include "coil/obj.hpp"
#include "coil/stream.hpp"
#include <vector>
#include <memory>
#include <string_view>

namespace coil {
/**
* @brief Encode a single instruction to binary format
* 
* @param instr The instruction to encode
* @param format Encoding format to use
* @return std::vector<u8> The encoded instruction
*/
std::vector<u8> encodeInstruction(const Instruction& instr);

/**
* @brief Decode a single instruction from binary format
* 
* @param data Pointer to the encoded instruction data
* @param size Size of the encoded data in bytes
* @return Instruction The decoded instruction
*/
Instruction decodeInstruction(const u8* data, size_t size);

/**
* @brief Encode an instruction block to binary format
* 
* @param block The instruction block to encode
* @param format Encoding format to use
* @return std::vector<u8> The encoded instruction data
*/
std::vector<u8> encodeInstructionBlock(const InstructionBlock& block);

/**
* @brief Decode a sequence of instructions from binary format
* 
* @param data Pointer to the encoded instruction data
* @param size Size of the encoded data in bytes
* @return InstructionBlock The decoded instruction block
*/
InstructionBlock decodeInstructionBlock(const u8* data, size_t size);

} // namespace coil