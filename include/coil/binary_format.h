#ifndef COIL_BINARY_FORMAT_H
#define COIL_BINARY_FORMAT_H

#include <coil/type_system.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace coil {

/**
 * @brief Represents an operand in a COIL instruction
 */
class Operand {
public:
    // Factory methods for creating different operand types
    static Operand fromImmediate(std::shared_ptr<Type> type, const std::vector<uint8_t>& value);
    static Operand fromVariable(uint64_t variableId, std::shared_ptr<Type> type);
    static Operand fromSymbol(uint64_t symbolId, std::shared_ptr<Type> type);
    static Operand fromType(std::shared_ptr<Type> type);
    
    // Accessors
    std::shared_ptr<Type> getType() const;
    const std::vector<uint8_t>& getValue() const;
    bool hasValue() const;
    
    // Binary encoding
    std::vector<uint8_t> encode() const;
    
    // String representation
    std::string toString() const;
    
private:
    // Private constructor for internal use
    Operand(std::shared_ptr<Type> type, const std::vector<uint8_t>& value = {});
    
    std::shared_ptr<Type> type_;
    std::vector<uint8_t> value_;
};

/**
 * @brief Represents a COIL instruction
 */
class Instruction {
public:
    enum class OpCode : uint8_t {
        // Control flow (0x00-0x1F)
        NOP  = 0x00,
        BR   = 0x01,
        CALL = 0x02,
        RET  = 0x03,
        CMP  = 0x04,
        TEST = 0x05,
        
        // Memory operations (0x20-0x3F)
        MOV  = 0x20,
        PUSH = 0x21,
        POP  = 0x22,
        LEA  = 0x23,
        SCOPE = 0x24,
        SCOPL = 0x25,
        VAR  = 0x26,
        XCHG = 0x27,
        CAS  = 0x28,
        
        // Arithmetic operations (0x40-0x5F)
        ADD  = 0x40,
        SUB  = 0x41,
        MUL  = 0x42,
        DIV  = 0x43,
        MOD  = 0x44,
        INC  = 0x45,
        DEC  = 0x46,
        NEG  = 0x47,
        ABS  = 0x48,
        SQRT = 0x49,
        CEIL = 0x4A,
        FLOR = 0x4B,
        ROND = 0x4C,
        
        // Bit manipulation (0x60-0x7F)
        AND  = 0x60,
        OR   = 0x61,
        XOR  = 0x62,
        NOT  = 0x63,
        SHL  = 0x64,
        SHR  = 0x65,
        SAL  = 0x66,
        SAR  = 0x67,
        
        // Vector/array operations (0x80-0x8F)
        GETE = 0x80,
        SETE = 0x81,
        DOT  = 0x82,
        CROSS = 0x83,
        NORM = 0x84,
        LEN  = 0x85,
        SHUF = 0x86,
        EXTRACT = 0x87,
        INSERT = 0x88,
        TRANS = 0x89,
        INV  = 0x8A,
        DET  = 0x8B,
        ROW  = 0x8C,
        COL  = 0x8D,
        DIAG = 0x8E,
        
        // Special operations (0x90-0x9F)
        RNG  = 0x90,
        HASH = 0x91,
        ENCRYPT = 0x92,
        DECRYPT = 0x93,
        CRYPT = 0x94,
        SIGN = 0x95,
        VERIFY = 0x96,
        COMPRESS = 0x97,
        EXPAND = 0x98,
        CHECKSUM = 0x99,
        
        // Extended operations (0xA0-0xDF) - PU specific
        
        // Type operations (0xE0-0xEF)
        TYPE = 0xE0,
        SIZEOF = 0xE1,
        ALIGNOF = 0xE2,
        DTYPE = 0xE3,
        
        // Compiler directives (0xF0-0xFE)
        IF   = 0xF0,
        ELIF = 0xF1,
        ELSE = 0xF2,
        EIF  = 0xF3,
        INCL = 0xF4,
        SECT = 0xF5,
        DATA = 0xF6,
        PADD = 0xF7,
        ABI  = 0xF9,
        DEF  = 0xFA,
        UDEF = 0xFB,
        TARGET = 0xFC,
        ETARGET = 0xFD,
        PRAGMA = 0xFE,
        
        // Extension (0xFF)
        EXT  = 0xFF
    };
    
    // Constructors
    Instruction(OpCode opcode, const std::vector<Operand>& operands = {});
    Instruction(OpCode opcode, uint8_t extendedOpcode, const std::vector<Operand>& operands = {});
    
    // Accessors
    OpCode getOpCode() const;
    uint8_t getExtendedOpcode() const;
    bool hasExtendedOpcode() const;
    const std::vector<Operand>& getOperands() const;
    size_t getOperandCount() const;
    
    // Binary encoding
    std::vector<uint8_t> encode() const;
    
    // String representation
    std::string toString() const;
    
    // Instruction validation
    bool validate() const;
    std::string getValidationError() const;
    
private:
    OpCode opcode_;
    uint8_t extendedOpcode_ = 0;
    bool hasExtendedOpcode_ = false;
    std::vector<Operand> operands_;
    mutable std::string validationError_;
};

/**
 * @brief Decoder for COIL binary format
 */
class BinaryDecoder {
public:
    // Constructor for decoding from a binary buffer
    explicit BinaryDecoder(const std::vector<uint8_t>& data);
    
    // Decode all instructions
    std::vector<Instruction> decodeAll();
    
    // Decode a single instruction
    bool decodeNext(Instruction& instruction);
    
    // Check if there are more instructions to decode
    bool hasMore() const;
    
    // Get current position in binary stream
    size_t getPosition() const;
    
    // Reset decoder position
    void reset();
    
private:
    // Helper method to decode an instruction
    Instruction decodeInstruction();
    
    // Helper method to decode an operand
    Operand decodeOperand();
    
    const std::vector<uint8_t>& data_;
    size_t position_ = 0;
};

/**
 * @brief Builder for COIL binary format
 */
class BinaryBuilder {
public:
    // Default constructor
    BinaryBuilder();
    
    // Add an instruction
    void addInstruction(const Instruction& instruction);
    
    // Add a specific instruction with operands
    void addInstruction(Instruction::OpCode opcode, const std::vector<Operand>& operands = {});
    
    // Add an extended instruction
    void addExtendedInstruction(Instruction::OpCode opcode, uint8_t extendedOpcode, const std::vector<Operand>& operands = {});
    
    // Create a variable with a type
    uint64_t createVariable(std::shared_ptr<Type> type);
    
    // Get the encoded binary data
    std::vector<uint8_t> getBinary() const;
    
    // Write to a file
    bool writeToFile(const std::string& filename) const;
    
    // Clear the builder
    void clear();
    
private:
    std::vector<Instruction> instructions_;
    uint64_t nextVariableId_ = 1;
};

/**
 * @brief Reader for COIL binary files
 */
class BinaryReader {
public:
    // Constructor from file
    explicit BinaryReader(const std::string& filename);
    
    // Constructor from memory
    explicit BinaryReader(const std::vector<uint8_t>& data);
    
    // Check if file was successfully opened
    bool isValid() const;
    
    // Get error message
    std::string getError() const;
    
    // Get all instructions
    std::vector<Instruction> getInstructions() const;
    
    // Create a decoder
    BinaryDecoder createDecoder() const;
    
private:
    bool valid_ = false;
    std::string error_;
    std::vector<uint8_t> data_;
};

} // namespace coil

#endif // COIL_BINARY_FORMAT_H
