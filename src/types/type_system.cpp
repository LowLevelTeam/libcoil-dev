#include <coil/type_system.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace coil {

// Type implementation classes
namespace {

/**
 * @brief Base class with common functionality for all type implementations
 */
class TypeImpl : public Type {
public:
    explicit TypeImpl(TypeExtension extension = TypeExtension())
        : extension_(extension) {}
    
    TypeExtension getExtension() const override {
        return extension_;
    }
    
    void setExtension(const TypeExtension& extension) override {
        extension_ = extension;
    }
    
protected:
    TypeExtension extension_;
};

/**
 * @brief Implementation for fixed-width integer types
 */
class FixedWidthIntType : public TypeImpl {
public:
    FixedWidthIntType(OpCode opcode, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), opcode_(opcode) {}
    
    OpCode getOpCode() const override {
        return opcode_;
    }
    
    size_t getSize() const override {
        switch (opcode_) {
            case OpCode::INT8:
            case OpCode::UNT8:
                return 1;
            case OpCode::INT16:
            case OpCode::UNT16:
                return 2;
            case OpCode::INT32:
            case OpCode::UNT32:
                return 4;
            case OpCode::INT64:
            case OpCode::UNT64:
                return 8;
            default:
                return 0; // Should never happen
        }
    }
    
    size_t getAlignment() const override {
        return getSize();
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(opcode_));
        result.push_back(extension_.getValue());
        return result;
    }
    
    std::string toString() const override {
        switch (opcode_) {
            case OpCode::INT8: return "INT8";
            case OpCode::UNT8: return "UNT8";
            case OpCode::INT16: return "INT16";
            case OpCode::UNT16: return "UNT16";
            case OpCode::INT32: return "INT32";
            case OpCode::UNT32: return "UNT32";
            case OpCode::INT64: return "INT64";
            case OpCode::UNT64: return "UNT64";
            default: return "UNKNOWN_INT_TYPE";
        }
    }
    
private:
    OpCode opcode_;
};

/**
 * @brief Implementation for fixed-width floating-point types
 */
class FixedWidthFloatType : public TypeImpl {
public:
    FixedWidthFloatType(OpCode opcode, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), opcode_(opcode) {}
    
    OpCode getOpCode() const override {
        return opcode_;
    }
    
    size_t getSize() const override {
        switch (opcode_) {
            case OpCode::FP32:
                return 4;
            case OpCode::FP64:
                return 8;
            default:
                return 0; // Should never happen
        }
    }
    
    size_t getAlignment() const override {
        return getSize();
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(opcode_));
        result.push_back(extension_.getValue());
        return result;
    }
    
    std::string toString() const override {
        switch (opcode_) {
            case OpCode::FP32: return "FP32";
            case OpCode::FP64: return "FP64";
            default: return "UNKNOWN_FLOAT_TYPE";
        }
    }
    
private:
    OpCode opcode_;
};

/**
 * @brief Implementation for vector types
 */
class VectorType : public TypeImpl {
public:
    VectorType(OpCode opcode, std::shared_ptr<Type> elementType, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), opcode_(opcode), elementType_(elementType) {}
    
    OpCode getOpCode() const override {
        return opcode_;
    }
    
    size_t getSize() const override {
        switch (opcode_) {
            case OpCode::V128:
                return 16;
            case OpCode::V256:
                return 32;
            case OpCode::V512:
                return 64;
            default:
                return 0; // Should never happen
        }
    }
    
    size_t getAlignment() const override {
        return getSize();
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(opcode_));
        result.push_back(extension_.getValue());
        
        // Append element type encoding
        std::vector<uint8_t> elementEncoding = elementType_->encode();
        result.insert(result.end(), elementEncoding.begin(), elementEncoding.end());
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        switch (opcode_) {
            case OpCode::V128: oss << "V128("; break;
            case OpCode::V256: oss << "V256("; break;
            case OpCode::V512: oss << "V512("; break;
            default: oss << "UNKNOWN_VECTOR("; break;
        }
        oss << elementType_->toString() << ")";
        return oss.str();
    }
    
    std::shared_ptr<Type> getElementType() const {
        return elementType_;
    }
    
private:
    OpCode opcode_;
    std::shared_ptr<Type> elementType_;
};

/**
 * @brief Implementation for custom vector types
 */
class CustomVectorType : public TypeImpl {
public:
    CustomVectorType(std::shared_ptr<Type> elementType, uint16_t count, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), elementType_(elementType), count_(count) {}
    
    OpCode getOpCode() const override {
        return OpCode::CVEC;
    }
    
    size_t getSize() const override {
        return elementType_->getSize() * count_;
    }
    
    size_t getAlignment() const override {
        return elementType_->getAlignment();
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::CVEC));
        result.push_back(extension_.getValue());
        
        // Append element type encoding
        std::vector<uint8_t> elementEncoding = elementType_->encode();
        result.insert(result.end(), elementEncoding.begin(), elementEncoding.end());
        
        // Append count
        result.push_back(count_ & 0xFF);
        result.push_back((count_ >> 8) & 0xFF);
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "CVEC(" << elementType_->toString() << ", " << count_ << ")";
        return oss.str();
    }
    
    std::shared_ptr<Type> getElementType() const {
        return elementType_;
    }
    
    uint16_t getCount() const {
        return count_;
    }
    
private:
    std::shared_ptr<Type> elementType_;
    uint16_t count_;
};

/**
 * @brief Implementation for matrix types
 */
class MatrixType : public TypeImpl {
public:
    MatrixType(std::shared_ptr<Type> elementType, uint16_t rows, uint16_t cols, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), elementType_(elementType), rows_(rows), cols_(cols) {}
    
    OpCode getOpCode() const override {
        return OpCode::MAT;
    }
    
    size_t getSize() const override {
        return elementType_->getSize() * rows_ * cols_;
    }
    
    size_t getAlignment() const override {
        // Align to element alignment or 16 bytes, whichever is larger
        return std::max(elementType_->getAlignment(), static_cast<size_t>(16));
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::MAT));
        result.push_back(extension_.getValue());
        
        // Append element type encoding
        std::vector<uint8_t> elementEncoding = elementType_->encode();
        result.insert(result.end(), elementEncoding.begin(), elementEncoding.end());
        
        // Append dimensions
        result.push_back(rows_ & 0xFF);
        result.push_back((rows_ >> 8) & 0xFF);
        result.push_back(cols_ & 0xFF);
        result.push_back((cols_ >> 8) & 0xFF);
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "MAT(" << elementType_->toString() << ", " << rows_ << "x" << cols_ << ")";
        return oss.str();
    }
    
    std::shared_ptr<Type> getElementType() const {
        return elementType_;
    }
    
    uint16_t getRows() const {
        return rows_;
    }
    
    uint16_t getCols() const {
        return cols_;
    }
    
private:
    std::shared_ptr<Type> elementType_;
    uint16_t rows_;
    uint16_t cols_;
};

/**
 * @brief Implementation for square matrix types
 */
class SquareMatrixType : public TypeImpl {
public:
    SquareMatrixType(std::shared_ptr<Type> elementType, uint16_t size, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), elementType_(elementType), size_(size) {}
    
    OpCode getOpCode() const override {
        return OpCode::SQMAT;
    }
    
    size_t getSize() const override {
        return elementType_->getSize() * size_ * size_;
    }
    
    size_t getAlignment() const override {
        // Align to element alignment or 16 bytes, whichever is larger
        return std::max(elementType_->getAlignment(), static_cast<size_t>(16));
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::SQMAT));
        result.push_back(extension_.getValue());
        
        // Append element type encoding
        std::vector<uint8_t> elementEncoding = elementType_->encode();
        result.insert(result.end(), elementEncoding.begin(), elementEncoding.end());
        
        // Append size
        result.push_back(size_ & 0xFF);
        result.push_back((size_ >> 8) & 0xFF);
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "SQMAT(" << elementType_->toString() << ", " << size_ << ")";
        return oss.str();
    }
    
    std::shared_ptr<Type> getElementType() const {
        return elementType_;
    }
    
    uint16_t getSize() const {
        return size_;
    }
    
private:
    std::shared_ptr<Type> elementType_;
    uint16_t size_;
};

/**
 * @brief Implementation for tensor types
 */
class TensorType : public TypeImpl {
public:
    TensorType(std::shared_ptr<Type> elementType, const std::vector<uint16_t>& dimensions, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), elementType_(elementType), dimensions_(dimensions) {
        // Validate dimensions
        if (dimensions.size() > 8) {
            throw std::invalid_argument("Tensor rank cannot exceed 8");
        }
    }
    
    OpCode getOpCode() const override {
        return OpCode::TENSOR;
    }
    
    size_t getSize() const override {
        size_t totalElements = 1;
        for (uint16_t dim : dimensions_) {
            totalElements *= dim;
        }
        return elementType_->getSize() * totalElements;
    }
    
    size_t getAlignment() const override {
        // Align to element alignment or 16 bytes, whichever is larger
        return std::max(elementType_->getAlignment(), static_cast<size_t>(16));
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::TENSOR));
        result.push_back(extension_.getValue());
        
        // Append element type encoding
        std::vector<uint8_t> elementEncoding = elementType_->encode();
        result.insert(result.end(), elementEncoding.begin(), elementEncoding.end());
        
        // Append rank
        result.push_back(static_cast<uint8_t>(dimensions_.size()));
        
        // Append dimensions
        for (uint16_t dim : dimensions_) {
            result.push_back(dim & 0xFF);
            result.push_back((dim >> 8) & 0xFF);
        }
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "TENSOR(" << elementType_->toString() << ", [";
        for (size_t i = 0; i < dimensions_.size(); ++i) {
            if (i > 0) oss << ",";
            oss << dimensions_[i];
        }
        oss << "])";
        return oss.str();
    }
    
    std::shared_ptr<Type> getElementType() const {
        return elementType_;
    }
    
    const std::vector<uint16_t>& getDimensions() const {
        return dimensions_;
    }
    
private:
    std::shared_ptr<Type> elementType_;
    std::vector<uint16_t> dimensions_;
};

/**
 * @brief Implementation for struct types
 */
class StructType : public TypeImpl {
public:
    using Member = std::pair<uint16_t, std::shared_ptr<Type>>;
    
    StructType(const std::vector<Member>& members, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), members_(members) {
        // Validate members
        if (members.empty() || members.size() > 65535) {
            throw std::invalid_argument("Struct must have between 1 and 65535 members");
        }
    }
    
    OpCode getOpCode() const override {
        return OpCode::STRUCT;
    }
    
    size_t getSize() const override {
        size_t total = 0;
        size_t currentAlignment = 1;
        
        for (const auto& member : members_) {
            const auto& type = member.second;
            size_t memberAlignment = type->getAlignment();
            
            // Align the current offset
            total = (total + memberAlignment - 1) & ~(memberAlignment - 1);
            
            // Add member size
            total += type->getSize();
            
            // Track largest alignment
            currentAlignment = std::max(currentAlignment, memberAlignment);
        }
        
        // Final size must be a multiple of the struct's alignment
        return (total + currentAlignment - 1) & ~(currentAlignment - 1);
    }
    
    size_t getAlignment() const override {
        // Alignment is the largest alignment of any member
        size_t maxAlignment = 1;
        for (const auto& member : members_) {
            maxAlignment = std::max(maxAlignment, member.second->getAlignment());
        }
        return maxAlignment;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::STRUCT));
        result.push_back(extension_.getValue());
        
        // Append member count
        uint16_t count = static_cast<uint16_t>(members_.size());
        result.push_back(count & 0xFF);
        result.push_back((count >> 8) & 0xFF);
        
        // Append members
        for (const auto& member : members_) {
            // Member ID
            result.push_back(member.first & 0xFF);
            result.push_back((member.first >> 8) & 0xFF);
            
            // Member type encoding
            std::vector<uint8_t> typeEncoding = member.second->encode();
            result.insert(result.end(), typeEncoding.begin(), typeEncoding.end());
        }
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "STRUCT(";
        for (size_t i = 0; i < members_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << members_[i].first << ":" << members_[i].second->toString();
        }
        oss << ")";
        return oss.str();
    }
    
    const std::vector<Member>& getMembers() const {
        return members_;
    }
    
private:
    std::vector<Member> members_;
};

/**
 * @brief Implementation for union types
 */
class UnionType : public TypeImpl {
public:
    using Member = std::pair<uint16_t, std::shared_ptr<Type>>;
    
    UnionType(const std::vector<Member>& members, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), members_(members) {
        // Validate members
        if (members.empty() || members.size() > 65535) {
            throw std::invalid_argument("Union must have between 1 and 65535 members");
        }
    }
    
    OpCode getOpCode() const override {
        return OpCode::UNION;
    }
    
    size_t getSize() const override {
        // Size is the size of the largest member
        size_t maxSize = 0;
        for (const auto& member : members_) {
            maxSize = std::max(maxSize, member.second->getSize());
        }
        
        // Final size must be a multiple of the union's alignment
        size_t alignment = getAlignment();
        return (maxSize + alignment - 1) & ~(alignment - 1);
    }
    
    size_t getAlignment() const override {
        // Alignment is the largest alignment of any member
        size_t maxAlignment = 1;
        for (const auto& member : members_) {
            maxAlignment = std::max(maxAlignment, member.second->getAlignment());
        }
        return maxAlignment;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::UNION));
        result.push_back(extension_.getValue());
        
        // Append member count
        uint16_t count = static_cast<uint16_t>(members_.size());
        result.push_back(count & 0xFF);
        result.push_back((count >> 8) & 0xFF);
        
        // Append members
        for (const auto& member : members_) {
            // Member ID
            result.push_back(member.first & 0xFF);
            result.push_back((member.first >> 8) & 0xFF);
            
            // Member type encoding
            std::vector<uint8_t> typeEncoding = member.second->encode();
            result.insert(result.end(), typeEncoding.begin(), typeEncoding.end());
        }
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "UNION(";
        for (size_t i = 0; i < members_.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << members_[i].first << ":" << members_[i].second->toString();
        }
        oss << ")";
        return oss.str();
    }
    
    const std::vector<Member>& getMembers() const {
        return members_;
    }
    
private:
    std::vector<Member> members_;
};

/**
 * @brief Implementation for bit type
 */
class BitType : public TypeImpl {
public:
    explicit BitType(TypeExtension extension = TypeExtension())
        : TypeImpl(extension) {}
    
    OpCode getOpCode() const override {
        return OpCode::BIT;
    }
    
    size_t getSize() const override {
        return 1; // 1 byte for storage, although logically 1 bit
    }
    
    size_t getAlignment() const override {
        return 1;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::BIT));
        result.push_back(extension_.getValue());
        return result;
    }
    
    std::string toString() const override {
        return "BIT";
    }
};

/**
 * @brief Implementation for void type
 */
class VoidType : public TypeImpl {
public:
    explicit VoidType(TypeExtension extension = TypeExtension())
        : TypeImpl(extension) {}
    
    OpCode getOpCode() const override {
        return OpCode::VOID;
    }
    
    size_t getSize() const override {
        return 0;
    }
    
    size_t getAlignment() const override {
        return 1;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::VOID));
        result.push_back(extension_.getValue());
        return result;
    }
    
    std::string toString() const override {
        return "VOID";
    }
};

/**
 * @brief Implementation for variable reference
 */
class VariableRefType : public TypeImpl {
public:
    VariableRefType(uint64_t variableId, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), variableId_(variableId) {
        extension_.set(TypeExtensionFlag::VARIABLE);
    }
    
    OpCode getOpCode() const override {
        return OpCode::VARID;
    }
    
    size_t getSize() const override {
        return 8; // 64-bit variable ID
    }
    
    size_t getAlignment() const override {
        return 8;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::VARID));
        result.push_back(extension_.getValue());
        
        // Append variable ID
        for (int i = 0; i < 8; i++) {
            result.push_back((variableId_ >> (i * 8)) & 0xFF);
        }
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "VARID(" << variableId_ << ")";
        return oss.str();
    }
    
    uint64_t getVariableId() const {
        return variableId_;
    }
    
private:
    uint64_t variableId_;
};

/**
 * @brief Implementation for symbol reference
 */
class SymbolRefType : public TypeImpl {
public:
    SymbolRefType(uint64_t symbolId, TypeExtension extension = TypeExtension())
        : TypeImpl(extension), symbolId_(symbolId) {
        extension_.set(TypeExtensionFlag::SYMBOL);
    }
    
    OpCode getOpCode() const override {
        return OpCode::SYMBOL;
    }
    
    size_t getSize() const override {
        return 8; // 64-bit symbol ID
    }
    
    size_t getAlignment() const override {
        return 8;
    }
    
    std::vector<uint8_t> encode() const override {
        std::vector<uint8_t> result;
        result.push_back(static_cast<uint8_t>(OpCode::SYMBOL));
        result.push_back(extension_.getValue());
        
        // Append symbol ID
        for (int i = 0; i < 8; i++) {
            result.push_back((symbolId_ >> (i * 8)) & 0xFF);
        }
        
        return result;
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << "SYMBOL(" << symbolId_ << ")";
        return oss.str();
    }
    
    uint64_t getSymbolId() const {
        return symbolId_;
    }
    
private:
    uint64_t symbolId_;
};

} // anonymous namespace

// Type factory methods implementation
std::shared_ptr<Type> Type::createInt8() {
    return std::make_shared<FixedWidthIntType>(OpCode::INT8);
}

std::shared_ptr<Type> Type::createUnt8() {
    return std::make_shared<FixedWidthIntType>(OpCode::UNT8);
}

std::shared_ptr<Type> Type::createInt16() {
    return std::make_shared<FixedWidthIntType>(OpCode::INT16);
}

std::shared_ptr<Type> Type::createUnt16() {
    return std::make_shared<FixedWidthIntType>(OpCode::UNT16);
}

std::shared_ptr<Type> Type::createInt32() {
    return std::make_shared<FixedWidthIntType>(OpCode::INT32);
}

std::shared_ptr<Type> Type::createUnt32() {
    return std::make_shared<FixedWidthIntType>(OpCode::UNT32);
}

std::shared_ptr<Type> Type::createInt64() {
    return std::make_shared<FixedWidthIntType>(OpCode::INT64);
}

std::shared_ptr<Type> Type::createUnt64() {
    return std::make_shared<FixedWidthIntType>(OpCode::UNT64);
}

std::shared_ptr<Type> Type::createFp32() {
    return std::make_shared<FixedWidthFloatType>(OpCode::FP32);
}

std::shared_ptr<Type> Type::createFp64() {
    return std::make_shared<FixedWidthFloatType>(OpCode::FP64);
}

std::shared_ptr<Type> Type::createBit() {
    return std::make_shared<BitType>();
}

std::shared_ptr<Type> Type::createVoid() {
    return std::make_shared<VoidType>();
}

std::shared_ptr<Type> Type::createV128(std::shared_ptr<Type> elementType) {
    return std::make_shared<VectorType>(OpCode::V128, elementType);
}

std::shared_ptr<Type> Type::createV256(std::shared_ptr<Type> elementType) {
    return std::make_shared<VectorType>(OpCode::V256, elementType);
}

std::shared_ptr<Type> Type::createV512(std::shared_ptr<Type> elementType) {
    return std::make_shared<VectorType>(OpCode::V512, elementType);
}

std::shared_ptr<Type> Type::createVector(std::shared_ptr<Type> elementType, uint16_t count) {
    return std::make_shared<CustomVectorType>(elementType, count);
}

std::shared_ptr<Type> Type::createMatrix(std::shared_ptr<Type> elementType, uint16_t rows, uint16_t cols) {
    return std::make_shared<MatrixType>(elementType, rows, cols);
}

std::shared_ptr<Type> Type::createSquareMatrix(std::shared_ptr<Type> elementType, uint16_t size) {
    return std::make_shared<SquareMatrixType>(elementType, size);
}

std::shared_ptr<Type> Type::createTensor(std::shared_ptr<Type> elementType, const std::vector<uint16_t>& dimensions) {
    return std::make_shared<TensorType>(elementType, dimensions);
}

std::shared_ptr<Type> Type::createStruct(const std::vector<std::pair<uint16_t, std::shared_ptr<Type>>>& members) {
    return std::make_shared<StructType>(members);
}

std::shared_ptr<Type> Type::createUnion(const std::vector<std::pair<uint16_t, std::shared_ptr<Type>>>& members) {
    return std::make_shared<UnionType>(members);
}

std::shared_ptr<Type> Type::createVariableRef(uint64_t variableId) {
    return std::make_shared<VariableRefType>(variableId);
}

std::shared_ptr<Type> Type::createSymbolRef(uint64_t symbolId) {
    return std::make_shared<SymbolRefType>(symbolId);
}

bool Type::operator==(const Type& other) const {
    // Check opcode first
    if (getOpCode() != other.getOpCode()) {
        return false;
    }
    
    // Check extension flags (ignore IMMEDIATE, VARIABLE, etc.)
    TypeExtension thisExt = getExtension();
    TypeExtension otherExt = other.getExtension();
    
    uint8_t thisMask = static_cast<uint8_t>(TypeExtensionFlag::CONST) |
                        static_cast<uint8_t>(TypeExtensionFlag::VOLATILE) |
                        static_cast<uint8_t>(TypeExtensionFlag::ATOMIC) |
                        static_cast<uint8_t>(TypeExtensionFlag::SATURATE);
    
    uint8_t thisFlags = thisExt.getValue() & thisMask;
    uint8_t otherFlags = otherExt.getValue() & thisMask;
    
    if (thisFlags != otherFlags) {
        return false;
    }
    
    // Specific type equality checks based on opcode
    switch (getOpCode()) {
        case OpCode::INT8:
        case OpCode::UNT8:
        case OpCode::INT16:
        case OpCode::UNT16:
        case OpCode::INT32:
        case OpCode::UNT32:
        case OpCode::INT64:
        case OpCode::UNT64:
        case OpCode::FP32:
        case OpCode::FP64:
        case OpCode::BIT:
        case OpCode::VOID:
            // For simple types, opcode comparison is sufficient
            return true;
            
        case OpCode::V128:
        case OpCode::V256:
        case OpCode::V512: {
            const auto* thisVec = dynamic_cast<const VectorType*>(this);
            const auto* otherVec = dynamic_cast<const VectorType*>(&other);
            if (!thisVec || !otherVec) return false;
            return *thisVec->getElementType() == *otherVec->getElementType();
        }
            
        case OpCode::CVEC: {
            const auto* thisVec = dynamic_cast<const CustomVectorType*>(this);
            const auto* otherVec = dynamic_cast<const CustomVectorType*>(&other);
            if (!thisVec || !otherVec) return false;
            return *thisVec->getElementType() == *otherVec->getElementType() &&
                   thisVec->getCount() == otherVec->getCount();
        }
            
        case OpCode::MAT: {
            const auto* thisMat = dynamic_cast<const MatrixType*>(this);
            const auto* otherMat = dynamic_cast<const MatrixType*>(&other);
            if (!thisMat || !otherMat) return false;
            return *thisMat->getElementType() == *otherMat->getElementType() &&
                   thisMat->getRows() == otherMat->getRows() &&
                   thisMat->getCols() == otherMat->getCols();
        }
            
        case OpCode::SQMAT: {
            const auto* thisMat = dynamic_cast<const SquareMatrixType*>(this);
            const auto* otherMat = dynamic_cast<const SquareMatrixType*>(&other);
            if (!thisMat || !otherMat) return false;
            return *thisMat->getElementType() == *otherMat->getElementType() &&
                   thisMat->getSize() == otherMat->getSize();
        }
            
        case OpCode::TENSOR: {
            const auto* thisTensor = dynamic_cast<const TensorType*>(this);
            const auto* otherTensor = dynamic_cast<const TensorType*>(&other);
            if (!thisTensor || !otherTensor) return false;
            return *thisTensor->getElementType() == *otherTensor->getElementType() &&
                   thisTensor->getDimensions() == otherTensor->getDimensions();
        }
            
        case OpCode::STRUCT: {
            const auto* thisStruct = dynamic_cast<const StructType*>(this);
            const auto* otherStruct = dynamic_cast<const StructType*>(&other);
            if (!thisStruct || !otherStruct) return false;
            
            const auto& thisMembers = thisStruct->getMembers();
            const auto& otherMembers = otherStruct->getMembers();
            
            if (thisMembers.size() != otherMembers.size()) {
                return false;
            }
            
            for (size_t i = 0; i < thisMembers.size(); ++i) {
                if (thisMembers[i].first != otherMembers[i].first ||
                    *thisMembers[i].second != *otherMembers[i].second) {
                    return false;
                }
            }
            
            return true;
        }
            
        case OpCode::UNION: {
            const auto* thisUnion = dynamic_cast<const UnionType*>(this);
            const auto* otherUnion = dynamic_cast<const UnionType*>(&other);
            if (!thisUnion || !otherUnion) return false;
            
            const auto& thisMembers = thisUnion->getMembers();
            const auto& otherMembers = otherUnion->getMembers();
            
            if (thisMembers.size() != otherMembers.size()) {
                return false;
            }
            
            for (size_t i = 0; i < thisMembers.size(); ++i) {
                if (thisMembers[i].first != otherMembers[i].first ||
                    *thisMembers[i].second != *otherMembers[i].second) {
                    return false;
                }
            }
            
            return true;
        }
            
        case OpCode::VARID: {
            const auto* thisVar = dynamic_cast<const VariableRefType*>(this);
            const auto* otherVar = dynamic_cast<const VariableRefType*>(&other);
            if (!thisVar || !otherVar) return false;
            return thisVar->getVariableId() == otherVar->getVariableId();
        }
            
        case OpCode::SYMBOL: {
            const auto* thisSym = dynamic_cast<const SymbolRefType*>(this);
            const auto* otherSym = dynamic_cast<const SymbolRefType*>(&other);
            if (!thisSym || !otherSym) return false;
            return thisSym->getSymbolId() == otherSym->getSymbolId();
        }
            
        default:
            // For unhandled types, just compare the binary encoding
            return encode() == other.encode();
    }
}

bool Type::operator!=(const Type& other) const {
    return !(*this == other);
}

bool Type::isCompatibleWith(const Type& other) const {
    // Same types are always compatible
    if (*this == other) {
        return true;
    }
    
    // Integer type compatibility
    if ((static_cast<uint8_t>(getOpCode()) <= static_cast<uint8_t>(OpCode::UNT64)) &&
        (static_cast<uint8_t>(other.getOpCode()) <= static_cast<uint8_t>(OpCode::UNT64))) {
        return true; // Any integer can be compatible with any other integer
    }
    
    // Floating point type compatibility
    if ((getOpCode() == OpCode::FP32 || getOpCode() == OpCode::FP64) &&
        (other.getOpCode() == OpCode::FP32 || other.getOpCode() == OpCode::FP64)) {
        return true; // FP32 and FP64 are compatible
    }
    
    // Integer and floating-point compatibility
    if ((static_cast<uint8_t>(getOpCode()) <= static_cast<uint8_t>(OpCode::UNT64)) &&
        (other.getOpCode() == OpCode::FP32 || other.getOpCode() == OpCode::FP64)) {
        return true; // Integers can be converted to floating-point
    }
    
    if ((static_cast<uint8_t>(other.getOpCode()) <= static_cast<uint8_t>(OpCode::UNT64)) &&
        (getOpCode() == OpCode::FP32 || getOpCode() == OpCode::FP64)) {
        return true; // Floating-point can be converted to integers
    }
    
    // Vector compatibility (same element type and size)
    if ((getOpCode() == OpCode::V128 || getOpCode() == OpCode::V256 || getOpCode() == OpCode::V512 || getOpCode() == OpCode::CVEC) &&
        (other.getOpCode() == OpCode::V128 || other.getOpCode() == OpCode::V256 || other.getOpCode() == OpCode::V512 || other.getOpCode() == OpCode::CVEC)) {
        // For built-in vector types, check size
        if (getSize() == other.getSize()) {
            // Extract element types
            std::shared_ptr<Type> thisElementType;
            std::shared_ptr<Type> otherElementType;
            
            if (getOpCode() == OpCode::CVEC) {
                thisElementType = dynamic_cast<const CustomVectorType*>(this)->getElementType();
            } else {
                thisElementType = dynamic_cast<const VectorType*>(this)->getElementType();
            }
            
            if (other.getOpCode() == OpCode::CVEC) {
                otherElementType = dynamic_cast<const CustomVectorType*>(&other)->getElementType();
            } else {
                otherElementType = dynamic_cast<const VectorType*>(&other)->getElementType();
            }
            
            // Check element type compatibility
            if (thisElementType && otherElementType) {
                return thisElementType->isCompatibleWith(*otherElementType);
            }
        }
    }
    
    // Matrix compatibility
    if ((getOpCode() == OpCode::MAT || getOpCode() == OpCode::SQMAT) &&
        (other.getOpCode() == OpCode::MAT || other.getOpCode() == OpCode::SQMAT)) {
        // Extract dimensions
        uint16_t thisRows, thisCols, otherRows, otherCols;
        std::shared_ptr<Type> thisElementType, otherElementType;
        
        if (getOpCode() == OpCode::MAT) {
            const auto* mat = dynamic_cast<const MatrixType*>(this);
            thisRows = mat->getRows();
            thisCols = mat->getCols();
            thisElementType = mat->getElementType();
        } else {
            const auto* mat = dynamic_cast<const SquareMatrixType*>(this);
            thisRows = mat->getSize();
            thisCols = mat->getSize();
            thisElementType = mat->getElementType();
        }
        
        if (other.getOpCode() == OpCode::MAT) {
            const auto* mat = dynamic_cast<const MatrixType*>(&other);
            otherRows = mat->getRows();
            otherCols = mat->getCols();
            otherElementType = mat->getElementType();
        } else {
            const auto* mat = dynamic_cast<const SquareMatrixType*>(&other);
            otherRows = mat->getSize();
            otherCols = mat->getSize();
            otherElementType = mat->getElementType();
        }
        
        // Check dimensions and element type compatibility
        if (thisRows == otherRows && thisCols == otherCols) {
            return thisElementType->isCompatibleWith(*otherElementType);
        }
    }
    
    // By default, types are not compatible
    return false;
}

std::shared_ptr<Type> Type::convertTo(std::shared_ptr<Type> targetType) const {
    // Same type, no conversion needed
    if (*this == *targetType) {
        return std::const_pointer_cast<Type>(shared_from_this());
    }
    
    // TODO: Implement conversion logic based on type promotion rules
    
    return nullptr; // Unable to convert
}

// TypeRegistry implementation
TypeRegistry& TypeRegistry::instance() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::registerType(uint64_t typeId, std::shared_ptr<Type> type) {
    types_[typeId] = type;
}

std::shared_ptr<Type> TypeRegistry::lookupType(uint64_t typeId) const {
    auto it = types_.find(typeId);
    if (it != types_.end()) {
        return it->second;
    }
    return nullptr;
}

void TypeRegistry::removeType(uint64_t typeId) {
    types_.erase(typeId);
}

bool TypeRegistry::hasType(uint64_t typeId) const {
    return types_.find(typeId) != types_.end();
}

// TypeDecoder implementation
std::shared_ptr<Type> TypeDecoder::decode(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 2 > data.size()) {
        throw std::out_of_range("Insufficient data for type decoding");
    }
    
    uint8_t opcode = data[offset++];
    uint8_t extension = data[offset++];
    
    TypeExtension typeExtension(extension);
    
    // Determine category and call appropriate decoder
    if (opcode <= 0x0F) {
        return decodeFixedWidthType(opcode, typeExtension, data, offset);
    } else if (opcode <= 0x1F) {
        return decodeFixedWidthType(opcode, typeExtension, data, offset);
    } else if (opcode <= 0x2F) {
        return decodeVectorType(opcode, typeExtension, data, offset);
    } else if (opcode <= 0x3F) {
        return decodeComplexType(opcode, typeExtension, data, offset);
    } else if (opcode >= 0xA0 && opcode <= 0xAF) {
        return decodeCompositeType(opcode, typeExtension, data, offset);
    } else {
        return decodeSpecialType(opcode, typeExtension, data, offset);
    }
}

std::shared_ptr<Type> TypeDecoder::decodeFixedWidthType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset) {
    (void)data; // Unused
    
    switch (opcode) {
        // Integer types
        case static_cast<uint8_t>(Type::OpCode::INT8):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::INT8, extension);
        case static_cast<uint8_t>(Type::OpCode::UNT8):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::UNT8, extension);
        case static_cast<uint8_t>(Type::OpCode::INT16):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::INT16, extension);
        case static_cast<uint8_t>(Type::OpCode::UNT16):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::UNT16, extension);
        case static_cast<uint8_t>(Type::OpCode::INT32):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::INT32, extension);
        case static_cast<uint8_t>(Type::OpCode::UNT32):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::UNT32, extension);
        case static_cast<uint8_t>(Type::OpCode::INT64):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::INT64, extension);
        case static_cast<uint8_t>(Type::OpCode::UNT64):
            return std::make_shared<FixedWidthIntType>(Type::OpCode::UNT64, extension);
            
        // Floating-point types
        case static_cast<uint8_t>(Type::OpCode::FP32):
            return std::make_shared<FixedWidthFloatType>(Type::OpCode::FP32, extension);
        case static_cast<uint8_t>(Type::OpCode::FP64):
            return std::make_shared<FixedWidthFloatType>(Type::OpCode::FP64, extension);
            
        default:
            throw std::invalid_argument("Unknown fixed-width type opcode");
    }
}

std::shared_ptr<Type> TypeDecoder::decodeVectorType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset) {
    // Decode element type
    std::shared_ptr<Type> elementType = decode(data, offset);
    
    switch (opcode) {
        case static_cast<uint8_t>(Type::OpCode::V128):
            return std::make_shared<VectorType>(Type::OpCode::V128, elementType, extension);
        case static_cast<uint8_t>(Type::OpCode::V256):
            return std::make_shared<VectorType>(Type::OpCode::V256, elementType, extension);
        case static_cast<uint8_t>(Type::OpCode::V512):
            return std::make_shared<VectorType>(Type::OpCode::V512, elementType, extension);
        default:
            throw std::invalid_argument("Unknown vector type opcode");
    }
}

std::shared_ptr<Type> TypeDecoder::decodeComplexType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset) {
    switch (opcode) {
        case static_cast<uint8_t>(Type::OpCode::CVEC): {
            // Decode element type
            std::shared_ptr<Type> elementType = decode(data, offset);
            
            // Read count
            if (offset + 2 > data.size()) {
                throw std::out_of_range("Insufficient data for CVEC count");
            }
            uint16_t count = data[offset] | (data[offset + 1] << 8);
            offset += 2;
            
            return std::make_shared<CustomVectorType>(elementType, count, extension);
        }
        
        case static_cast<uint8_t>(Type::OpCode::MAT): {
            // Decode element type
            std::shared_ptr<Type> elementType = decode(data, offset);
            
            // Read dimensions
            if (offset + 4 > data.size()) {
                throw std::out_of_range("Insufficient data for MAT dimensions");
            }
            uint16_t rows = data[offset] | (data[offset + 1] << 8);
            offset += 2;
            uint16_t cols = data[offset] | (data[offset + 1] << 8);
            offset += 2;
            
            return std::make_shared<MatrixType>(elementType, rows, cols, extension);
        }
        
        case static_cast<uint8_t>(Type::OpCode::SQMAT): {
            // Decode element type
            std::shared_ptr<Type> elementType = decode(data, offset);
            
            // Read size
            if (offset + 2 > data.size()) {
                throw std::out_of_range("Insufficient data for SQMAT size");
            }
            uint16_t size = data[offset] | (data[offset + 1] << 8);
            offset += 2;
            
            return std::make_shared<SquareMatrixType>(elementType, size, extension);
        }
        
        case static_cast<uint8_t>(Type::OpCode::TENSOR): {
            // Decode element type
            std::shared_ptr<Type> elementType = decode(data, offset);
            
            // Read rank
            if (offset >= data.size()) {
                throw std::out_of_range("Insufficient data for TENSOR rank");
            }
            uint8_t rank = data[offset++];
            
            // Read dimensions
            if (offset + rank * 2 > data.size()) {
                throw std::out_of_range("Insufficient data for TENSOR dimensions");
            }
            
            std::vector<uint16_t> dimensions(rank);
            for (uint8_t i = 0; i < rank; ++i) {
                dimensions[i] = data[offset] | (data[offset + 1] << 8);
                offset += 2;
            }
            
            return std::make_shared<TensorType>(elementType, dimensions, extension);
        }
        
        default:
            throw std::invalid_argument("Unknown complex type opcode");
    }
}

std::shared_ptr<Type> TypeDecoder::decodeCompositeType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset) {
    switch (opcode) {
        case static_cast<uint8_t>(Type::OpCode::STRUCT):
        case static_cast<uint8_t>(Type::OpCode::UNION): {
            // Read field count
            if (offset + 2 > data.size()) {
                throw std::out_of_range("Insufficient data for composite type field count");
            }
            uint16_t fieldCount = data[offset] | (data[offset + 1] << 8);
            offset += 2;
            
            // Read fields
            std::vector<std::pair<uint16_t, std::shared_ptr<Type>>> fields;
            for (uint16_t i = 0; i < fieldCount; ++i) {
                // Read field ID
                if (offset + 2 > data.size()) {
                    throw std::out_of_range("Insufficient data for field ID");
                }
                uint16_t fieldId = data[offset] | (data[offset + 1] << 8);
                offset += 2;
                
                // Decode field type
                std::shared_ptr<Type> fieldType = decode(data, offset);
                
                fields.emplace_back(fieldId, fieldType);
            }
            
            if (opcode == static_cast<uint8_t>(Type::OpCode::STRUCT)) {
                return std::make_shared<StructType>(fields, extension);
            } else {
                return std::make_shared<UnionType>(fields, extension);
            }
        }
        
        default:
            throw std::invalid_argument("Unknown composite type opcode");
    }
}

std::shared_ptr<Type> TypeDecoder::decodeSpecialType(uint8_t opcode, const TypeExtension& extension, const std::vector<uint8_t>& data, size_t& offset) {
    switch (opcode) {
        case static_cast<uint8_t>(Type::OpCode::BIT):
            return std::make_shared<BitType>(extension);
            
        case static_cast<uint8_t>(Type::OpCode::VOID):
            return std::make_shared<VoidType>(extension);
            
        case static_cast<uint8_t>(Type::OpCode::VARID): {
            // Read variable ID
            if (offset + 8 > data.size()) {
                throw std::out_of_range("Insufficient data for variable ID");
            }
            uint64_t varId = 0;
            for (int i = 0; i < 8; ++i) {
                varId |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
            }
            offset += 8;
            
            return std::make_shared<VariableRefType>(varId, extension);
        }
            
        case static_cast<uint8_t>(Type::OpCode::SYMBOL): {
            // Read symbol ID
            if (offset + 8 > data.size()) {
                throw std::out_of_range("Insufficient data for symbol ID");
            }
            uint64_t symbolId = 0;
            for (int i = 0; i < 8; ++i) {
                symbolId |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
            }
            offset += 8;
            
            return std::make_shared<SymbolRefType>(symbolId, extension);
        }
            
        default:
            throw std::invalid_argument("Unknown special type opcode");
    }
}

} // namespace coil
