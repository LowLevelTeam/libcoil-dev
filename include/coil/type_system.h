#ifndef COIL_TYPE_SYSTEM_H
#define COIL_TYPE_SYSTEM_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace coil {

/**
 * @brief Enumeration of type extension flags
 */
enum class TypeExtensionFlag : uint8_t {
    CONST     = (1 << 0),  // Ensure no modifications
    VOLATILE  = (1 << 1),  // Do not optimize expressions with this value
    ATOMIC    = (1 << 2),  // Use atomic variant if possible
    SATURATE  = (1 << 3),  // Cap at largest value instead of overflow
    IMMEDIATE = (1 << 4),  // Value is embedded in instruction
    DEFINITION= (1 << 5),  // Value is a compile-time definition
    VARIABLE  = (1 << 6),  // Refers to a variable id
    SYMBOL    = (1 << 7),  // Refers to a named memory address
};

/**
 * @brief Type extension bitmap
 */
class TypeExtension {
public:
    explicit TypeExtension(uint8_t value = 0) : value_(value) {}
    
    bool isSet(TypeExtensionFlag flag) const {
        return (value_ & static_cast<uint8_t>(flag)) != 0;
    }
    
    void set(TypeExtensionFlag flag, bool value = true) {
        if (value) {
            value_ |= static_cast<uint8_t>(flag);
        } else {
            value_ &= ~static_cast<uint8_t>(flag);
        }
    }
    
    uint8_t getValue() const { return value_; }
    
private:
    uint8_t value_;
};

/**
 * @brief Base class for all COIL types
 */
class Type {
public:
    /**
     * @brief Type opcode categories
     */
    enum class OpCode : uint8_t {
        // Fixed-width integral types (0x00-0x0F)
        INT8    = 0x00,
        UNT8    = 0x01,
        INT16   = 0x02,
        UNT16   = 0x03,
        INT32   = 0x04,
        UNT32   = 0x05,
        INT64   = 0x06,
        UNT64   = 0x07,
        
        // Fixed-width floating-point types (0x10-0x1F)
        FP32    = 0x10,
        FP64    = 0x11,
        
        // Fixed-width vector types (0x20-0x2F)
        V128    = 0x20,
        V256    = 0x21,
        V512    = 0x22,
        
        // Complex types (0x30-0x3F)
        CINT    = 0x30,
        CUNT    = 0x31,
        CFP     = 0x32,
        CVEC    = 0x33,
        MAT     = 0x34,
        SQMAT   = 0x35,
        DIAGMAT = 0x36,
        TENSOR  = 0x37,
        SYMTENSOR = 0x38,
        
        // Composite types (0xA0-0xAF)
        STRUCT  = 0xA0,
        PACK    = 0xA1,
        UNION   = 0xA2,
        ALIAS   = 0xA3,
        
        // Optimized types (0xF0-0xF9)
        BIT     = 0xF0,
        VARID   = 0xF1,
        SYMBOL  = 0xF2,
        DEFID   = 0xF3,
        STRING  = 0xF4,
        REG     = 0xF9,
        
        // Special types (0xFA-0xFF)
        SIZEOF  = 0xE1,
        ALIGNOF = 0xE2,
        PARAM4  = 0xFA,
        PARAM3  = 0xFB,
        PARAM2  = 0xFC,
        PARAM1  = 0xFD,
        PARAM0  = 0xFE,
        VOID    = 0xFF,
    };
    
    // Factory methods for common types
    static std::shared_ptr<Type> createInt8();
    static std::shared_ptr<Type> createUnt8();
    static std::shared_ptr<Type> createInt16();
    static std::shared_ptr<Type> createUnt16();
    static std::shared_ptr<Type> createInt32();
    static std::shared_ptr<Type> createUnt32();
    static std::shared_ptr<Type> createInt64();
    static std::shared_ptr<Type> createUnt64();
    static std::shared_ptr<Type> createFp32();
    static std::shared_ptr<Type> createFp64();
    static std::shared_ptr<Type> createBit();
    static std::shared_ptr<Type> createVoid();
    
    // Factory methods for complex types
    static std::shared_ptr<Type> createV128(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createV256(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createV512(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createVector(std::shared_ptr<Type> elementType, uint16_t count);
    static std::shared_ptr<Type> createMatrix(std::shared_ptr<Type> elementType, uint16_t rows, uint16_t cols);
    static std::shared_ptr<Type> createSquareMatrix(std::shared_ptr<Type> elementType, uint16_t size);
    static std::shared_ptr<Type> createTensor(std::shared_ptr<Type> elementType, const std::vector<uint16_t>& dimensions);
    
    // Factory methods for composite types
    static std::shared_ptr<Type> createStruct(const std::vector<std::pair<uint16_t, std::shared_ptr<Type>>>& members);
    static std::shared_ptr<Type> createUnion(const std::vector<std::pair<uint16_t, std::shared_ptr<Type>>>& members);
    
    // Factory methods for reference types
    static std::shared_ptr<Type> createVariableRef(uint64_t variableId);
    static std::shared_ptr<Type> createSymbolRef(uint64_t symbolId);
    
    // Core methods all types must implement
    virtual OpCode getOpCode() const = 0;
    virtual TypeExtension getExtension() const = 0;
    virtual void setExtension(const TypeExtension& extension) = 0;
    virtual size_t getSize() const = 0;
    virtual size_t getAlignment() const = 0;
    virtual std::vector<uint8_t> encode() const = 0;
    
    // Virtual destructor
    virtual ~Type() = default;
    
    // Type comparison
    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;
    
    // Check type compatibility
    bool isCompatibleWith(const Type& other) const;
    
    // Type conversion
    std::shared_ptr<Type> convertTo(std::shared_ptr<Type> targetType) const;
    
    // String representation
    virtual std::string toString() const = 0;
};

/**
 * @brief Registry for type IDs and definitions
 */
class TypeRegistry {
public:
    // Singleton access
    static TypeRegistry& instance();
    
    // Register a type with an ID
    void registerType(uint64_t typeId, std::shared_ptr<Type> type);
    
    // Lookup a type by ID
    std::shared_ptr<Type> lookupType(uint64_t typeId) const;
    
    // Remove a type definition
    void removeType(uint64_t typeId);
    
    // Check if a type ID exists
    bool hasType(uint64_t typeId) const;
    
private:
    TypeRegistry() = default;
    ~TypeRegistry() = default;
    
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    
    std::unordered_map<uint64_t, std::shared_ptr<Type>> types_;
};

/**
 * @brief Type decoder for binary format
 */
class TypeDecoder {
public:
    // Decode a type from binary data
    static std::shared_ptr<Type> decode(const std::vector<uint8_t>& data, size_t& offset);
    
private:
    // Helper methods for decoding various type categories
    static std::shared_ptr<Type> decodeFixedWidthType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset);
    static std::shared_ptr<Type> decodeVectorType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset);
    static std::shared_ptr<Type> decodeComplexType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset);
    static std::shared_ptr<Type> decodeCompositeType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset);
    static std::shared_ptr<Type> decodeSpecialType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset);
};

} // namespace coil

#endif // COIL_TYPE_SYSTEM_H
