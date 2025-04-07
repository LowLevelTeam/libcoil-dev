#include <coil/type_system.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <sstream>
#include <vector>

namespace coil {

namespace {

/**
 * @brief Computes the memory offset for a tensor element
 * 
 * @param indices Array of indices, one per dimension
 * @param dims Array of dimension sizes
 * @param rank Number of dimensions
 * @param elementSize Size of each element in bytes
 * @return The byte offset from the start of the tensor data
 */
size_t computeTensorOffset(const size_t* indices, const size_t* dims, size_t rank, size_t elementSize) {
    // Calculate strides for each dimension
    std::vector<size_t> strides(rank);
    
    // Last dimension has stride 1 (in elements)
    strides.back() = 1;
    
    // Calculate strides for other dimensions (in elements)
    for (int i = static_cast<int>(rank) - 2; i >= 0; i--) {
        strides[i] = strides[i + 1] * dims[i + 1];
    }
    
    // Calculate linear index
    size_t linearIndex = 0;
    for (size_t i = 0; i < rank; i++) {
        linearIndex += indices[i] * strides[i];
    }
    
    // Return byte offset
    return linearIndex * elementSize;
}

/**
 * @brief Validates tensor indices to ensure they are within bounds
 * 
 * @param indices Array of indices, one per dimension
 * @param dims Array of dimension sizes
 * @param rank Number of dimensions
 * @throws std::out_of_range if any index is out of bounds
 */
void validateTensorIndices(const size_t* indices, const size_t* dims, size_t rank) {
    for (size_t i = 0; i < rank; i++) {
        if (indices[i] >= dims[i]) {
            std::ostringstream oss;
            oss << "Tensor index out of bounds: [";
            for (size_t j = 0; j < rank; j++) {
                if (j > 0) oss << ",";
                oss << indices[j];
            }
            oss << "] for tensor of dimensions [";
            for (size_t j = 0; j < rank; j++) {
                if (j > 0) oss << ",";
                oss << dims[j];
            }
            oss << "]";
            throw std::out_of_range(oss.str());
        }
    }
}

/**
 * @brief Calculates total number of elements in a tensor
 * 
 * @param dims Array of dimension sizes
 * @param rank Number of dimensions
 * @return Total number of elements
 */
size_t calculateTensorElements(const size_t* dims, size_t rank) {
    if (rank == 0) return 0;
    
    size_t elements = 1;
    for (size_t i = 0; i < rank; i++) {
        elements *= dims[i];
    }
    return elements;
}

/**
 * @brief Implementation of tensor operations for different element types
 */
class TensorOperations {
public:
    /**
     * @brief Get element from tensor
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data
     * @param indices Array of indices, one per dimension
     * @param dims Array of dimension sizes
     * @param rank Number of dimensions
     * @return Reference to the element
     */
    template<typename T>
    static T& getElement(void* data, const size_t* indices, const size_t* dims, size_t rank) {
        validateTensorIndices(indices, dims, rank);
        size_t offset = computeTensorOffset(indices, dims, rank, sizeof(T));
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset);
    }
    
    /**
     * @brief Set element in tensor
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data
     * @param indices Array of indices, one per dimension
     * @param value Value to set
     * @param dims Array of dimension sizes
     * @param rank Number of dimensions
     */
    template<typename T>
    static void setElement(void* data, const size_t* indices, const T& value, const size_t* dims, size_t rank) {
        validateTensorIndices(indices, dims, rank);
        size_t offset = computeTensorOffset(indices, dims, rank, sizeof(T));
        *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset) = value;
    }
    
    /**
     * @brief Extract a slice from a tensor
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data
     * @param sliceDim Dimension along which to slice
     * @param sliceIndex Index in the slice dimension
     * @param dims Array of dimension sizes
     * @param rank Number of dimensions
     * @param result Pointer to store the slice data
     * @return True if slice was successful, false otherwise
     */
    template<typename T>
    static bool extractSlice(const void* data, size_t sliceDim, size_t sliceIndex,
                            const size_t* dims, size_t rank, void* result) {
        if (sliceDim >= rank) return false;
        if (sliceIndex >= dims[sliceDim]) return false;
        
        // Calculate dimensions of the resulting slice
        size_t newRank = rank - 1;
        std::vector<size_t> newDims(newRank);
        
        size_t dimsIdx = 0;
        for (size_t i = 0; i < rank; i++) {
            if (i != sliceDim) {
                newDims[dimsIdx++] = dims[i];
            }
        }
        
        // Calculate strides for the original tensor
        std::vector<size_t> strides(rank);
        strides.back() = 1;
        for (int i = static_cast<int>(rank) - 2; i >= 0; i--) {
            strides[i] = strides[i + 1] * dims[i + 1];
        }
        
        // Calculate strides for the slice
        std::vector<size_t> newStrides(newRank);
        if (newRank > 0) {
            newStrides.back() = 1;
            for (int i = static_cast<int>(newRank) - 2; i >= 0; i--) {
                newStrides[i] = newStrides[i + 1] * newDims[i + 1];
            }
        }
        
        // Calculate total number of elements in the slice
        size_t elements = calculateTensorElements(newDims.data(), newRank);
        
        // Extract the slice elements
        const T* srcData = reinterpret_cast<const T*>(data);
        T* destData = reinterpret_cast<T*>(result);
        
        // Create indices arrays for source and destination
        std::vector<size_t> srcIndices(rank, 0);
        std::vector<size_t> destIndices(newRank, 0);
        
        // Set the slice dimension index
        srcIndices[sliceDim] = sliceIndex;
        
        // Extract all elements from the slice
        for (size_t i = 0; i < elements; i++) {
            // Calculate source linear index
            size_t srcLinearIndex = 0;
            for (size_t j = 0; j < rank; j++) {
                srcLinearIndex += srcIndices[j] * strides[j];
            }
            
            // Calculate destination linear index
            size_t destLinearIndex = 0;
            for (size_t j = 0; j < newRank; j++) {
                destLinearIndex += destIndices[j] * newStrides[j];
            }
            
            // Copy the element
            destData[destLinearIndex] = srcData[srcLinearIndex];
            
            // Increment indices
            // For destination, increment the last dimension and carry over
            if (newRank > 0) {
                size_t d = newRank - 1;
                destIndices[d]++;
                while (d > 0 && destIndices[d] >= newDims[d]) {
                    destIndices[d] = 0;
                    destIndices[--d]++;
                }
            }
            
            // For source, do the same but skip the slice dimension
            for (size_t j = 0, d = 0; j < rank; j++) {
                if (j == sliceDim) continue;
                srcIndices[j] = destIndices[d++];
            }
        }
        
        return true;
    }
    
    /**
     * @brief Insert a slice into a tensor
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data to modify
     * @param sliceDim Dimension along which to insert
     * @param sliceIndex Index in the slice dimension
     * @param sliceData Pointer to the slice data
     * @param dims Array of dimension sizes for the tensor
     * @param rank Number of dimensions of the tensor
     * @return True if insertion was successful, false otherwise
     */
    template<typename T>
    static bool insertSlice(void* data, size_t sliceDim, size_t sliceIndex,
                           const void* sliceData, const size_t* dims, size_t rank) {
        if (sliceDim >= rank) return false;
        if (sliceIndex >= dims[sliceDim]) return false;
        
        // Calculate dimensions of the slice
        size_t newRank = rank - 1;
        std::vector<size_t> newDims(newRank);
        
        size_t dimsIdx = 0;
        for (size_t i = 0; i < rank; i++) {
            if (i != sliceDim) {
                newDims[dimsIdx++] = dims[i];
            }
        }
        
        // Calculate strides for the tensor
        std::vector<size_t> strides(rank);
        strides.back() = 1;
        for (int i = static_cast<int>(rank) - 2; i >= 0; i--) {
            strides[i] = strides[i + 1] * dims[i + 1];
        }
        
        // Calculate strides for the slice
        std::vector<size_t> newStrides(newRank);
        if (newRank > 0) {
            newStrides.back() = 1;
            for (int i = static_cast<int>(newRank) - 2; i >= 0; i--) {
                newStrides[i] = newStrides[i + 1] * newDims[i + 1];
            }
        }
        
        // Calculate total number of elements in the slice
        size_t elements = calculateTensorElements(newDims.data(), newRank);
        
        // Access the data
        T* destData = reinterpret_cast<T*>(data);
        const T* srcData = reinterpret_cast<const T*>(sliceData);
        
        // Create indices arrays for source and destination
        std::vector<size_t> destIndices(rank, 0);
        std::vector<size_t> srcIndices(newRank, 0);
        
        // Set the slice dimension index
        destIndices[sliceDim] = sliceIndex;
        
        // Insert all elements from the slice
        for (size_t i = 0; i < elements; i++) {
            // Calculate source linear index
            size_t srcLinearIndex = 0;
            for (size_t j = 0; j < newRank; j++) {
                srcLinearIndex += srcIndices[j] * newStrides[j];
            }
            
            // Calculate destination linear index
            size_t destLinearIndex = 0;
            for (size_t j = 0; j < rank; j++) {
                destLinearIndex += destIndices[j] * strides[j];
            }
            
            // Copy the element
            destData[destLinearIndex] = srcData[srcLinearIndex];
            
            // Increment indices
            // For source, increment the last dimension and carry over
            if (newRank > 0) {
                size_t d = newRank - 1;
                srcIndices[d]++;
                while (d > 0 && srcIndices[d] >= newDims[d]) {
                    srcIndices[d] = 0;
                    srcIndices[--d]++;
                }
            }
            
            // For destination, do the same but skip the slice dimension
            for (size_t j = 0, d = 0; j < rank; j++) {
                if (j == sliceDim) continue;
                destIndices[j] = srcIndices[d++];
            }
        }
        
        return true;
    }
    
    /**
     * @brief Apply a function element-wise to a tensor
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data
     * @param dims Array of dimension sizes
     * @param rank Number of dimensions
     * @param op Function to apply to each element
     */
    template<typename T>
    static void applyFunction(void* data, const size_t* dims, size_t rank, std::function<T(T)> op) {
        size_t elements = calculateTensorElements(dims, rank);
        T* typedData = reinterpret_cast<T*>(data);
        
        for (size_t i = 0; i < elements; i++) {
            typedData[i] = op(typedData[i]);
        }
    }
    
    /**
     * @brief Calculate tensor sum along a dimension
     * 
     * @tparam T Element type
     * @param data Pointer to tensor data
     * @param dims Array of dimension sizes
     * @param rank Number of dimensions
     * @param sumDim Dimension along which to sum
     * @param result Pointer to store the result
     * @return True if sum was successful, false otherwise
     */
    template<typename T>
    static bool sum(const void* data, const size_t* dims, size_t rank, 
                   size_t sumDim, void* result) {
        if (sumDim >= rank) return false;
        
        // Calculate dimensions of the result
        size_t newRank = rank - 1;
        std::vector<size_t> newDims(newRank);
        
        size_t dimsIdx = 0;
        for (size_t i = 0; i < rank; i++) {
            if (i != sumDim) {
                newDims[dimsIdx++] = dims[i];
            }
        }
        
        // Calculate total elements in the result
        size_t resultElements = calculateTensorElements(newDims.data(), newRank);
        
        // Initialize result to zero
        std::memset(result, 0, resultElements * sizeof(T));
        
        // Access the data
        const T* srcData = reinterpret_cast<const T*>(data);
        T* destData = reinterpret_cast<T*>(result);
        
        // Create indices arrays for source and destination
        std::vector<size_t> srcIndices(rank, 0);
        
        // For each value in the sum dimension
        for (size_t sumIdx = 0; sumIdx < dims[sumDim]; sumIdx++) {
            srcIndices[sumDim] = sumIdx;
            
            // For each element in the result
            for (size_t destIdx = 0; destIdx < resultElements; destIdx++) {
                // Generate source indices from destination index
                size_t temp = destIdx;
                for (size_t i = 0, j = 0; i < rank; i++) {
                    if (i == sumDim) continue;
                    
                    size_t dimProduct = 1;
                    for (size_t k = j + 1; k < newRank; k++) {
                        dimProduct *= newDims[k];
                    }
                    
                    srcIndices[i] = temp / dimProduct;
                    temp %= dimProduct;
                    j++;
                }
                
                // Calculate source offset
                size_t srcOffset = 0;
                size_t stride = 1;
                for (int i = rank - 1; i >= 0; i--) {
                    srcOffset += srcIndices[i] * stride;
                    stride *= dims[i];
                }
                
                // Add to result
                destData[destIdx] += srcData[srcOffset];
            }
        }
        
        return true;
    }
};

} // anonymous namespace

// Tensor helper functions that can be exposed in a future version

/**
 * @brief Get a tensor element
 * 
 * @param tensor Tensor data
 * @param indices Vector of indices, one per dimension
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes
 * @return Vector of bytes representing the element value
 */
std::vector<uint8_t> getTensorElement(const std::vector<uint8_t>& tensor,
                                     const std::vector<size_t>& indices,
                                     const std::shared_ptr<Type>& elementType,
                                     const std::vector<size_t>& dimensions) {
    size_t rank = dimensions.size();
    
    // Validate rank
    if (rank != indices.size()) {
        throw std::invalid_argument("Number of indices must match tensor rank");
    }
    
    // Validate indices
    validateTensorIndices(indices.data(), dimensions.data(), rank);
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeTensorOffset(indices.data(), dimensions.data(), rank, elementSize);
    
    // Check bounds
    if (offset + elementSize > tensor.size()) {
        throw std::out_of_range("Tensor element access out of bounds");
    }
    
    // Extract the element
    return std::vector<uint8_t>(tensor.begin() + offset,
                               tensor.begin() + offset + elementSize);
}

/**
 * @brief Set a tensor element
 * 
 * @param tensor Tensor data to modify
 * @param indices Vector of indices, one per dimension
 * @param value Element value to set
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes
 */
void setTensorElement(std::vector<uint8_t>& tensor,
                      const std::vector<size_t>& indices,
                      const std::vector<uint8_t>& value,
                      const std::shared_ptr<Type>& elementType,
                      const std::vector<size_t>& dimensions) {
    size_t rank = dimensions.size();
    
    // Validate rank
    if (rank != indices.size()) {
        throw std::invalid_argument("Number of indices must match tensor rank");
    }
    
    // Validate indices
    validateTensorIndices(indices.data(), dimensions.data(), rank);
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeTensorOffset(indices.data(), dimensions.data(), rank, elementSize);
    
    // Check bounds
    if (offset + elementSize > tensor.size()) {
        throw std::out_of_range("Tensor element access out of bounds");
    }
    
    // Check value size
    if (value.size() != elementSize) {
        throw std::invalid_argument("Element value size does not match element type size");
    }
    
    // Copy the value
    std::copy(value.begin(), value.end(), tensor.begin() + offset);
}

/**
 * @brief Extract a slice from a tensor
 * 
 * @param tensor Tensor data
 * @param sliceDimension Dimension along which to slice
 * @param sliceIndex Index in the slice dimension
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes
 * @return Vector of bytes representing the slice
 */
std::vector<uint8_t> extractTensorSlice(const std::vector<uint8_t>& tensor,
                                       size_t sliceDimension,
                                       size_t sliceIndex,
                                       const std::shared_ptr<Type>& elementType,
                                       const std::vector<size_t>& dimensions) {
    size_t rank = dimensions.size();
    
    // Validate slice dimension
    if (sliceDimension >= rank) {
        throw std::invalid_argument("Slice dimension out of bounds");
    }
    
    // Validate slice index
    if (sliceIndex >= dimensions[sliceDimension]) {
        throw std::invalid_argument("Slice index out of bounds");
    }
    
    // Calculate dimensions of the resulting slice
    std::vector<size_t> sliceDimensions;
    for (size_t i = 0; i < rank; i++) {
        if (i != sliceDimension) {
            sliceDimensions.push_back(dimensions[i]);
        }
    }
    
    // Calculate total number of elements in the slice
    size_t elementSize = elementType->getSize();
    size_t sliceElements = 1;
    for (size_t dim : sliceDimensions) {
        sliceElements *= dim;
    }
    
    // Create result buffer
    std::vector<uint8_t> result(sliceElements * elementSize);
    
    bool success = false;
    
    // Extract slice based on element type
    if (elementType->getOpCode() == Type::OpCode::INT8) {
        success = TensorOperations::extractSlice<int8_t>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::INT16) {
        success = TensorOperations::extractSlice<int16_t>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::INT32) {
        success = TensorOperations::extractSlice<int32_t>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        success = TensorOperations::extractSlice<int64_t>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        success = TensorOperations::extractSlice<float>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        success = TensorOperations::extractSlice<double>(
            tensor.data(), sliceDimension, sliceIndex,
            dimensions.data(), rank, result.data());
    } else {
        throw std::invalid_argument("Tensor slice only supported for basic numeric types");
    }
    
    if (!success) {
        throw std::runtime_error("Failed to extract tensor slice");
    }
    
    return result;
}

/**
 * @brief Insert a slice into a tensor
 * 
 * @param tensor Tensor data to modify
 * @param sliceDimension Dimension along which to insert
 * @param sliceIndex Index in the slice dimension
 * @param slice Slice data to insert
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes for the tensor
 */
void insertTensorSlice(std::vector<uint8_t>& tensor,
                       size_t sliceDimension,
                       size_t sliceIndex,
                       const std::vector<uint8_t>& slice,
                       const std::shared_ptr<Type>& elementType,
                       const std::vector<size_t>& dimensions) {
    size_t rank = dimensions.size();
    
    // Validate slice dimension
    if (sliceDimension >= rank) {
        throw std::invalid_argument("Slice dimension out of bounds");
    }
    
    // Validate slice index
    if (sliceIndex >= dimensions[sliceDimension]) {
        throw std::invalid_argument("Slice index out of bounds");
    }
    
    // Calculate dimensions of the slice
    std::vector<size_t> sliceDimensions;
    for (size_t i = 0; i < rank; i++) {
        if (i != sliceDimension) {
            sliceDimensions.push_back(dimensions[i]);
        }
    }
    
    // Calculate total number of elements in the slice
    size_t elementSize = elementType->getSize();
    size_t sliceElements = 1;
    for (size_t dim : sliceDimensions) {
        sliceElements *= dim;
    }
    
    // Check slice size
    if (slice.size() != sliceElements * elementSize) {
        throw std::invalid_argument("Slice size does not match expected size");
    }
    
    bool success = false;
    
    // Insert slice based on element type
    if (elementType->getOpCode() == Type::OpCode::INT8) {
        success = TensorOperations::insertSlice<int8_t>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else if (elementType->getOpCode() == Type::OpCode::INT16) {
        success = TensorOperations::insertSlice<int16_t>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else if (elementType->getOpCode() == Type::OpCode::INT32) {
        success = TensorOperations::insertSlice<int32_t>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        success = TensorOperations::insertSlice<int64_t>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        success = TensorOperations::insertSlice<float>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        success = TensorOperations::insertSlice<double>(
            tensor.data(), sliceDimension, sliceIndex,
            slice.data(), dimensions.data(), rank);
    } else {
        throw std::invalid_argument("Tensor slice only supported for basic numeric types");
    }
    
    if (!success) {
        throw std::runtime_error("Failed to insert tensor slice");
    }
}

/**
 * @brief Sum a tensor along a dimension
 * 
 * @param tensor Tensor data
 * @param sumDimension Dimension along which to sum
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes
 * @return Vector of bytes representing the sum
 */
std::vector<uint8_t> sumTensorAlongDimension(const std::vector<uint8_t>& tensor,
                                            size_t sumDimension,
                                            const std::shared_ptr<Type>& elementType,
                                            const std::vector<size_t>& dimensions) {
    size_t rank = dimensions.size();
    
    // Validate sum dimension
    if (sumDimension >= rank) {
        throw std::invalid_argument("Sum dimension out of bounds");
    }
    
    // Calculate dimensions of the result
    std::vector<size_t> resultDimensions;
    for (size_t i = 0; i < rank; i++) {
        if (i != sumDimension) {
            resultDimensions.push_back(dimensions[i]);
        }
    }
    
    // Calculate total number of elements in the result
    size_t elementSize = elementType->getSize();
    size_t resultElements = 1;
    for (size_t dim : resultDimensions) {
        resultElements *= dim;
    }
    
    // Create result buffer
    std::vector<uint8_t> result(resultElements * elementSize);
    
    bool success = false;
    
    // Sum based on element type
    if (elementType->getOpCode() == Type::OpCode::INT32) {
        success = TensorOperations::sum<int32_t>(
            tensor.data(), dimensions.data(), rank,
            sumDimension, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        success = TensorOperations::sum<int64_t>(
            tensor.data(), dimensions.data(), rank,
            sumDimension, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        success = TensorOperations::sum<float>(
            tensor.data(), dimensions.data(), rank,
            sumDimension, result.data());
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        success = TensorOperations::sum<double>(
            tensor.data(), dimensions.data(), rank,
            sumDimension, result.data());
    } else {
        throw std::invalid_argument("Tensor sum only supported for INT32, INT64, FP32, and FP64 elements");
    }
    
    if (!success) {
        throw std::runtime_error("Failed to sum tensor");
    }
    
    return result;
}

/**
 * @brief Apply a type-specific function to all elements in a tensor
 * 
 * This is a utility function that will apply different functions based on element type.
 * 
 * @param tensor Tensor data to modify
 * @param elementType Type of tensor elements
 * @param dimensions Vector of dimension sizes
 * @param op Function identifier (0 = abs, 1 = negate, 2 = square, 3 = sqrt, etc.)
 */
void applyTensorFunction(std::vector<uint8_t>& tensor,
                         const std::shared_ptr<Type>& elementType,
                         const std::vector<size_t>& dimensions,
                         int op) {
    // Calculate total elements
    size_t elements = 1;
    for (size_t dim : dimensions) {
        elements *= dim;
    }
    
    size_t elementSize = elementType->getSize();
    
    // Check tensor size
    if (tensor.size() < elements * elementSize) {
        throw std::out_of_range("Tensor data size is smaller than expected");
    }
    
    // Apply operation based on element type
    if (elementType->getOpCode() == Type::OpCode::INT32) {
        std::function<int32_t(int32_t)> func;
        
        switch (op) {
            case 0: // abs
                func = [](int32_t x) { return std::abs(x); };
                break;
            case 1: // negate
                func = [](int32_t x) { return -x; };
                break;
            case 2: // square
                func = [](int32_t x) { return x * x; };
                break;
            default:
                throw std::invalid_argument("Unsupported operation for INT32");
        }
        
        TensorOperations::applyFunction<int32_t>(
            tensor.data(), dimensions.data(), dimensions.size(), func);
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        std::function<float(float)> func;
        
        switch (op) {
            case 0: // abs
                func = [](float x) { return std::abs(x); };
                break;
            case 1: // negate
                func = [](float x) { return -x; };
                break;
            case 2: // square
                func = [](float x) { return x * x; };
                break;
            case 3: // sqrt
                func = [](float x) { return std::sqrt(x); };
                break;
            case 4: // exp
                func = [](float x) { return std::exp(x); };
                break;
            case 5: // log
                func = [](float x) { return std::log(x); };
                break;
            default:
                throw std::invalid_argument("Unsupported operation for FP32");
        }
        
        TensorOperations::applyFunction<float>(
            tensor.data(), dimensions.data(), dimensions.size(), func);
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        std::function<double(double)> func;
        
        switch (op) {
            case 0: // abs
                func = [](double x) { return std::abs(x); };
                break;
            case 1: // negate
                func = [](double x) { return -x; };
                break;
            case 2: // square
                func = [](double x) { return x * x; };
                break;
            case 3: // sqrt
                func = [](double x) { return std::sqrt(x); };
                break;
            case 4: // exp
                func = [](double x) { return std::exp(x); };
                break;
            case 5: // log
                func = [](double x) { return std::log(x); };
                break;
            default:
                throw std::invalid_argument("Unsupported operation for FP64");
        }
        
        TensorOperations::applyFunction<double>(
            tensor.data(), dimensions.data(), dimensions.size(), func);
    } else {
        throw std::invalid_argument("Tensor function only supported for INT32, FP32, and FP64 elements");
    }
}

} // namespace coil
