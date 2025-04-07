#include <coil/binary_format.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace coil {

// Operand implementation

Operand Operand::fromImmediate(std::shared_ptr<Type> type, const std::vector<uint8_t>& value) {
    TypeExtension ext = type->getExtension();
    ext.set(TypeExtensionFlag::IMMEDIATE);
    
    auto operand = Operand(type);
    operand.type_->setExtension(ext);
    operand.value_ = value;
    
    return operand;
}

Operand Operand::fromVariable(uint64_t variableId, std::shared_ptr<Type> type) {
    TypeExtension ext = type->getExtension();
    ext.set(TypeExtensionFlag::VARIABLE);
    
    auto operand = Operand(type);
    operand.type_->setExtension(ext);
    
    // Encode variable ID
    operand.value_.resize(8);
    for (int i = 0; i < 8; i++) {
        operand.value_[i] = (variableId >> (i * 8)) & 0xFF;
    }
    
    return operand;
}

Operand Operand::fromSymbol(uint64_t symbolId, std::shared_ptr<Type> type) {
    TypeExtension ext = type->getExtension();
    ext.set(TypeExtensionFlag::SYMBOL);
    
    auto operand = Operand(type);
    operand.type_->setExtension(ext);
    
    // Encode symbol ID
    operand.value_.resize(8);
    for (int i = 0; i < 8; i++) {
        operand.value_[i] = (symbolId >> (i * 8)) & 0xFF;
    }
    
    return operand;
}

Operand Operand::fromType(std::shared_ptr<Type> type) {
    return Operand(type);
}

Operand::Operand(std::shared_ptr<Type> type, const std::vector<uint8_t>& value)
    : type_(type), value_(value) {
}

std::shared_ptr<Type> Operand::getType() const {
    return type_;
}

const std::vector<uint8_t>& Operand::getValue() const {
    return value_;
}

bool Operand::hasValue() const {
    return !value_.empty();
}

std::vector<uint8_t> Operand::encode() const {
    // Encode type
    std::vector<uint8_t> result = type_->encode();
    
    // If this operand has a value, append it
    if (hasValue()) {
        result.insert(result.end(), value_.begin(), value_.end());
    }
    
    return result;
}

std::string Operand::toString() const {
    std::ostringstream oss;
    
    TypeExtension ext = type_->getExtension();
    
    // Handle special cases based on extension flags
    if (ext.isSet(TypeExtensionFlag::IMMEDIATE)) {
        oss << "IMM(" << type_->toString() << ", ";
        
        // For simple types, try to format the value
        Type::OpCode opcode = type_->getOpCode();
        if (opcode == Type::OpCode::INT8 || opcode == Type::OpCode::UNT8 ||
            opcode == Type::OpCode::INT16 || opcode == Type::OpCode::UNT16 ||
            opcode == Type::OpCode::INT32 || opcode == Type::OpCode::UNT32 ||
            opcode == Type::OpCode::INT64 || opcode == Type::OpCode::UNT64) {
            
            uint64_t val = 0;
            for (size_t i = 0; i < std::min(value_.size(), size_t(8)); i++) {
                val |= static_cast<uint64_t>(value_[i]) << (i * 8);
            }
            oss << val;
        } else if (opcode == Type::OpCode::FP32) {
            // Interpret as float
            if (value_.size() >= 4) {
                float val;
                std::memcpy(&val, value_.data(), sizeof(float));
                oss << val;
            } else {
                oss << "[invalid float]";
            }
        } else if (opcode == Type::OpCode::FP64) {
            // Interpret as double
            if (value_.size() >= 8) {
                double val;
                std::memcpy(&val, value_.data(), sizeof(double));
                oss << val;
            } else {
                oss << "[invalid double]";
            }
        } else {
            // For other types, just show byte count
            oss << "[" << value_.size() << " bytes]";
        }
        
        oss << ")";
    } else if (ext.isSet(TypeExtensionFlag::VARIABLE)) {
        if (value_.size() >= 8) {
            uint64_t varId = 0;
            for (int i = 0; i < 8; i++) {
                varId |= static_cast<uint64_t>(value_[i]) << (i * 8);
            }
            oss << "VAR(" << type_->toString() << ", " << varId << ")";
        } else {
            oss << "VAR(" << type_->toString() << ", [invalid ID])";
        }
    } else if (ext.isSet(TypeExtensionFlag::SYMBOL)) {
        if (value_.size() >= 8) {
            uint64_t symId = 0;
            for (int i = 0; i < 8; i++) {
                symId |= static_cast<uint64_t>(value_[i]) << (i * 8);
            }
            oss << "SYM(" << type_->toString() << ", " << symId << ")";
        } else {
            oss << "SYM(" << type_->toString() << ", [invalid ID])";
        }
    } else {
        // Just the type
        oss << type_->toString();
    }
    
    return oss.str();
}

// Instruction implementation

Instruction::Instruction(OpCode opcode, const std::vector<Operand>& operands)
    : opcode_(opcode), operands_(operands) {
}

Instruction::Instruction(OpCode opcode, uint8_t extendedOpcode, const std::vector<Operand>& operands)
    : opcode_(opcode), extendedOpcode_(extendedOpcode), hasExtendedOpcode_(true), operands_(operands) {
}

Instruction::OpCode Instruction::getOpCode() const {
    return opcode_;
}

uint8_t Instruction::getExtendedOpcode() const {
    return extendedOpcode_;
}

bool Instruction::hasExtendedOpcode() const {
    return hasExtendedOpcode_;
}

const std::vector<Operand>& Instruction::getOperands() const {
    return operands_;
}

size_t Instruction::getOperandCount() const {
    return operands_.size();
}

std::vector<uint8_t> Instruction::encode() const {
    std::vector<uint8_t> result;
    
    // Add the primary opcode
    result.push_back(static_cast<uint8_t>(opcode_));
    
    // Add extended opcode if present
    if (hasExtendedOpcode_) {
        result.push_back(extendedOpcode_);
    }
    
    // Add operand count
    result.push_back(static_cast<uint8_t>(operands_.size()));
    
    // Add each operand
    for (const auto& operand : operands_) {
        std::vector<uint8_t> operandData = operand.encode();
        result.insert(result.end(), operandData.begin(), operandData.end());
    }
    
    return result;
}

std::string Instruction::toString() const {
    std::ostringstream oss;
    
    // Convert opcode to string
    switch (opcode_) {
        // Control flow
        case OpCode::NOP: oss << "NOP"; break;
        case OpCode::BR: oss << "BR"; break;
        case OpCode::CALL: oss << "CALL"; break;
        case OpCode::RET: oss << "RET"; break;
        case OpCode::CMP: oss << "CMP"; break;
        case OpCode::TEST: oss << "TEST"; break;
        
        // Memory operations
        case OpCode::MOV: oss << "MOV"; break;
        case OpCode::PUSH: oss << "PUSH"; break;
        case OpCode::POP: oss << "POP"; break;
        case OpCode::LEA: oss << "LEA"; break;
        case OpCode::SCOPE: oss << "SCOPE"; break;
        case OpCode::SCOPL: oss << "SCOPL"; break;
        case OpCode::VAR: oss << "VAR"; break;
        case OpCode::XCHG: oss << "XCHG"; break;
        case OpCode::CAS: oss << "CAS"; break;
        
        // Arithmetic operations
        case OpCode::ADD: oss << "ADD"; break;
        case OpCode::SUB: oss << "SUB"; break;
        case OpCode::MUL: oss << "MUL"; break;
        case OpCode::DIV: oss << "DIV"; break;
        case OpCode::MOD: oss << "MOD"; break;
        case OpCode::INC: oss << "INC"; break;
        case OpCode::DEC: oss << "DEC"; break;
        case OpCode::NEG: oss << "NEG"; break;
        case OpCode::ABS: oss << "ABS"; break;
        case OpCode::SQRT: oss << "SQRT"; break;
        case OpCode::CEIL: oss << "CEIL"; break;
        case OpCode::FLOR: oss << "FLOR"; break;
        case OpCode::ROND: oss << "ROND"; break;
        
        // Bit manipulation
        case OpCode::AND: oss << "AND"; break;
        case OpCode::OR: oss << "OR"; break;
        case OpCode::XOR: oss << "XOR"; break;
        case OpCode::NOT: oss << "NOT"; break;
        case OpCode::SHL: oss << "SHL"; break;
        case OpCode::SHR: oss << "SHR"; break;
        case OpCode::SAL: oss << "SAL"; break;
        case OpCode::SAR: oss << "SAR"; break;
        
        // Vector/array operations
        case OpCode::GETE: oss << "GETE"; break;
        case OpCode::SETE: oss << "SETE"; break;
        case OpCode::DOT: oss << "DOT"; break;
        case OpCode::CROSS: oss << "CROSS"; break;
        case OpCode::NORM: oss << "NORM"; break;
        case OpCode::LEN: oss << "LEN"; break;
        case OpCode::SHUF: oss << "SHUF"; break;
        case OpCode::EXTRACT: oss << "EXTRACT"; break;
        case OpCode::INSERT: oss << "INSERT"; break;
        case OpCode::TRANS: oss << "TRANS"; break;
        case OpCode::INV: oss << "INV"; break;
        case OpCode::DET: oss << "DET"; break;
        case OpCode::ROW: oss << "ROW"; break;
        case OpCode::COL: oss << "COL"; break;
        case OpCode::DIAG: oss << "DIAG"; break;
        
        // Special operations
        case OpCode::RNG: oss << "RNG"; break;
        case OpCode::HASH: oss << "HASH"; break;
        case OpCode::ENCRYPT: oss << "ENCRYPT"; break;
        case OpCode::DECRYPT: oss << "DECRYPT"; break;
        case OpCode::CRYPT: oss << "CRYPT"; break;
        case OpCode::SIGN: oss << "SIGN"; break;
        case OpCode::VERIFY: oss << "VERIFY"; break;
        case OpCode::COMPRESS: oss << "COMPRESS"; break;
        case OpCode::EXPAND: oss << "EXPAND"; break;
        case OpCode::CHECKSUM: oss << "CHECKSUM"; break;
        
        // Type operations
        case OpCode::TYPE: oss << "TYPE"; break;
        case OpCode::SIZEOF: oss << "SIZEOF"; break;
        case OpCode::ALIGNOF: oss << "ALIGNOF"; break;
        case OpCode::DTYPE: oss << "DTYPE"; break;
        
        // Compiler directives
        case OpCode::IF: oss << "IF"; break;
        case OpCode::ELIF: oss << "ELIF"; break;
        case OpCode::ELSE: oss << "ELSE"; break;
        case OpCode::EIF: oss << "EIF"; break;
        case OpCode::INCL: oss << "INCL"; break;
        case OpCode::SECT: oss << "SECT"; break;
        case OpCode::DATA: oss << "DATA"; break;
        case OpCode::PADD: oss << "PADD"; break;
        case OpCode::ABI: oss << "ABI"; break;
        case OpCode::DEF: oss << "DEF"; break;
        case OpCode::UDEF: oss << "UDEF"; break;
        case OpCode::TARGET: oss << "TARGET"; break;
        case OpCode::ETARGET: oss << "ETARGET"; break;
        case OpCode::PRAGMA: oss << "PRAGMA"; break;
        
        // Extension
        case OpCode::EXT: oss << "EXT"; break;
        
        default: oss << "UNKNOWN"; break;
    }
    
    // Add extended opcode if present
    if (hasExtendedOpcode_) {
        oss << "." << static_cast<int>(extendedOpcode_);
    }
    
    // Add operands
    for (const auto& operand : operands_) {
        oss << " " << operand.toString();
    }
    
    return oss.str();
}

bool Instruction::validate() const {
    // Clear any previous validation errors
    validationError_.clear();
    
    // Special case for extension opcode
    if (opcode_ == OpCode::EXT && !hasExtendedOpcode_) {
        validationError_ = "EXT instruction requires an extended opcode";
        return false;
    }
    
    // Check operand count constraints based on opcode
    switch (opcode_) {
        // Instructions with 0 operands
        case OpCode::NOP:
        case OpCode::RET:
        case OpCode::SCOPE:
        case OpCode::SCOPL:
        case OpCode::ELSE:
        case OpCode::EIF:
        case OpCode::ETARGET:
            if (operands_.size() != 0) {
                validationError_ = "Instruction requires 0 operands";
                return false;
            }
            break;
        
        // Instructions with 1 operand
        case OpCode::PUSH:
        case OpCode::POP:
        case OpCode::INC:
        case OpCode::DEC:
        case OpCode::NOT:
        case OpCode::DTYPE:
            if (operands_.size() != 1) {
                validationError_ = "Instruction requires 1 operand";
                return false;
            }
            break;
        
        // Instructions with 1-2 operands
        case OpCode::NEG:
        case OpCode::ABS:
        case OpCode::SQRT:
        case OpCode::CEIL:
        case OpCode::FLOR:
        case OpCode::ROND:
        case OpCode::BR:
        case OpCode::CALL:
            if (operands_.size() < 1 || operands_.size() > 2) {
                validationError_ = "Instruction requires 1-2 operands";
                return false;
            }
            break;
        
        // Instructions with 2 operands
        case OpCode::MOV:
        case OpCode::LEA:
        case OpCode::XCHG:
        case OpCode::CMP:
        case OpCode::TEST:
        case OpCode::LEN:
            if (operands_.size() != 2) {
                validationError_ = "Instruction requires 2 operands";
                return false;
            }
            break;
        
        // Instructions with 2-3 operands
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::AND:
        case OpCode::OR:
        case OpCode::XOR:
        case OpCode::SHL:
        case OpCode::SHR:
        case OpCode::SAL:
        case OpCode::SAR:
            if (operands_.size() < 2 || operands_.size() > 3) {
                validationError_ = "Instruction requires 2-3 operands";
                return false;
            }
            break;
        
        // Instructions with 3 operands
        case OpCode::CAS:
        case OpCode::GETE:
        case OpCode::SETE:
        case OpCode::DOT:
        case OpCode::CROSS:
            if (operands_.size() != 3) {
                validationError_ = "Instruction requires 3 operands";
                return false;
            }
            break;
        
        // Special instructions with variable operand counts
        case OpCode::VAR:
            if (operands_.size() < 2 || operands_.size() > 3) {
                validationError_ = "VAR instruction requires 2-3 operands";
                return false;
            }
            break;
            
        // Default case - allow any number of operands
        default:
            break;
    }
    
    // If we get here, validation succeeded
    return true;
}

std::string Instruction::getValidationError() const {
    return validationError_;
}

// BinaryDecoder implementation

BinaryDecoder::BinaryDecoder(const std::vector<uint8_t>& data)
    : data_(data), position_(0) {
}

std::vector<Instruction> BinaryDecoder::decodeAll() {
    std::vector<Instruction> instructions;
    
    reset();
    
    while (hasMore()) {
        Instruction instruction = decodeInstruction();
        instructions.push_back(instruction);
    }
    
    return instructions;
}

bool BinaryDecoder::decodeNext(Instruction& instruction) {
    if (!hasMore()) {
        return false;
    }
    
    instruction = decodeInstruction();
    return true;
}

bool BinaryDecoder::hasMore() const {
    return position_ < data_.size();
}

size_t BinaryDecoder::getPosition() const {
    return position_;
}

void BinaryDecoder::reset() {
    position_ = 0;
}

Instruction BinaryDecoder::decodeInstruction() {
    if (position_ + 2 > data_.size()) {
        throw std::out_of_range("Insufficient data for instruction at position " + std::to_string(position_));
    }
    
    // Read opcode
    uint8_t opcode = data_[position_++];
    
    // Check for extended opcode
    bool hasExtendedOpcode = (opcode == static_cast<uint8_t>(Instruction::OpCode::EXT));
    uint8_t extendedOpcode = 0;
    
    if (hasExtendedOpcode) {
        if (position_ >= data_.size()) {
            throw std::out_of_range("Insufficient data for extended opcode");
        }
        extendedOpcode = data_[position_++];
    }
    
    // Read operand count
    uint8_t operandCount = data_[position_++];
    
    // Read operands
    std::vector<Operand> operands;
    for (uint8_t i = 0; i < operandCount; i++) {
        operands.push_back(decodeOperand());
    }
    
    // Create instruction
    if (hasExtendedOpcode) {
        return Instruction(static_cast<Instruction::OpCode>(opcode), extendedOpcode, operands);
    } else {
        return Instruction(static_cast<Instruction::OpCode>(opcode), operands);
    }
}

Operand BinaryDecoder::decodeOperand() {
    // First, decode the type
    size_t startPos = position_;
    std::shared_ptr<Type> type = TypeDecoder::decode(data_, position_);
    
    // Check if this type has a value (IMMEDIATE, VARIABLE, SYMBOL)
    TypeExtension ext = type->getExtension();
    
    std::vector<uint8_t> value;
    
    if (ext.isSet(TypeExtensionFlag::IMMEDIATE)) {
        // Determine the value size based on the type
        size_t valueSize = type->getSize();
        
        if (position_ + valueSize > data_.size()) {
            throw std::out_of_range("Insufficient data for immediate value");
        }
        
        // Extract the value bytes
        value.assign(data_.begin() + position_, data_.begin() + position_ + valueSize);
        position_ += valueSize;
    } else if (ext.isSet(TypeExtensionFlag::VARIABLE) || ext.isSet(TypeExtensionFlag::SYMBOL)) {
        // Variable and symbol references store a 64-bit ID
        if (position_ + 8 > data_.size()) {
            throw std::out_of_range("Insufficient data for variable/symbol ID");
        }
        
        // Extract the ID bytes
        value.assign(data_.begin() + position_, data_.begin() + position_ + 8);
        position_ += 8;
    }
    
    return Operand(type, value);
}

// BinaryBuilder implementation

BinaryBuilder::BinaryBuilder() 
    : nextVariableId_(1) {
}

void BinaryBuilder::addInstruction(const Instruction& instruction) {
    instructions_.push_back(instruction);
}

void BinaryBuilder::addInstruction(Instruction::OpCode opcode, const std::vector<Operand>& operands) {
    instructions_.emplace_back(opcode, operands);
}

void BinaryBuilder::addExtendedInstruction(Instruction::OpCode opcode, uint8_t extendedOpcode, const std::vector<Operand>& operands) {
    instructions_.emplace_back(opcode, extendedOpcode, operands);
}

uint64_t BinaryBuilder::createVariable(std::shared_ptr<Type> type) {
    return nextVariableId_++;
}

std::vector<uint8_t> BinaryBuilder::getBinary() const {
    std::vector<uint8_t> result;
    
    for (const auto& instruction : instructions_) {
        std::vector<uint8_t> encodedInstruction = instruction.encode();
        result.insert(result.end(), encodedInstruction.begin(), encodedInstruction.end());
    }
    
    return result;
}

bool BinaryBuilder::writeToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::vector<uint8_t> binary = getBinary();
    file.write(reinterpret_cast<const char*>(binary.data()), binary.size());
    
    return file.good();
}

void BinaryBuilder::clear() {
    instructions_.clear();
    nextVariableId_ = 1;
}

// BinaryReader implementation

BinaryReader::BinaryReader(const std::string& filename) 
    : valid_(false) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        error_ = "Failed to open file: " + filename;
        return;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    
    // Read entire file
    data_.resize(fileSize);
    file.read(reinterpret_cast<char*>(data_.data()), fileSize);
    
    if (!file) {
        error_ = "Failed to read file: " + filename;
        return;
    }
    
    valid_ = true;
}

BinaryReader::BinaryReader(const std::vector<uint8_t>& data) 
    : data_(data), valid_(true) {
}

bool BinaryReader::isValid() const {
    return valid_;
}

std::string BinaryReader::getError() const {
    return error_;
}

std::vector<Instruction> BinaryReader::getInstructions() const {
    if (!valid_) {
        return {};
    }
    
    BinaryDecoder decoder(data_);
    return decoder.decodeAll();
}

BinaryDecoder BinaryReader::createDecoder() const {
    return BinaryDecoder(data_);
}

} // namespace coil
