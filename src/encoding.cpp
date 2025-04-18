/**
* @file encoding.cpp
* @brief Implementation of COIL instruction encoding and decoding
*/

#include "coil/encoding.hpp"
#include "coil/err.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace coil {

// Instruction Layout
// Instruction layouts are organised into a couple major layouts being the void and varadic as the most used
// the instruction layout can be decoded from the opcode.
// - VoidType: [encoding: u8]
// - Varadic : [encoding: u8][operands: u8][[operand: Operand]...operands]

// Apart from the major layouts there are also the specialized layouts.
// Since the instruction set is so simple right now these do not have to be worried about

// -------------------------------- Instruction Encoding -------------------------------- //
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