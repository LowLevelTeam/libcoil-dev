#include <coil/type_system.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <vector>

namespace coil {

namespace {

/**
 * @brief Computes the memory offset for a matrix element in row-major format
 * 
 * @param row The row index (0-based)
 * @param col The column index (0-based)
 * @param cols The total number of columns in the matrix
 * @param elementSize The size of each element in bytes
 * @return The byte offset from the start of the matrix data
 */
inline size_t computeMatrixOffset(size_t row, size_t col, size_t cols, size_t elementSize) {
    return (row * cols + col) * elementSize;
}

/**
 * @brief Validates matrix indices to ensure they are within bounds
 * 
 * @param row The row index to validate
 * @param col The column index to validate
 * @param rows The total number of rows in the matrix
 * @param cols The total number of columns in the matrix
 * @throws std::out_of_range if indices are out of bounds
 */
inline void validateMatrixIndices(size_t row, size_t col, size_t rows, size_t cols) {
    if (row >= rows || col >= cols) {
        std::ostringstream oss;
        oss << "Matrix index out of bounds: [" << row << "," << col 
            << "] in matrix of size " << rows << "x" << cols;
        throw std::out_of_range(oss.str());
    }
}

/**
 * @brief Implementation of matrix operations for various element types
 */
class MatrixOperations {
public:
    /**
     * @brief Get element from matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param row Row index
     * @param col Column index
     * @param rows Total rows
     * @param cols Total columns
     * @return Reference to the element
     */
    template<typename T>
    static T& getElement(void* data, size_t row, size_t col, size_t rows, size_t cols) {
        validateMatrixIndices(row, col, rows, cols);
        size_t offset = computeMatrixOffset(row, col, cols, sizeof(T));
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset);
    }
    
    /**
     * @brief Set element in matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param row Row index
     * @param col Column index
     * @param value Value to set
     * @param rows Total rows
     * @param cols Total columns
     */
    template<typename T>
    static void setElement(void* data, size_t row, size_t col, const T& value, size_t rows, size_t cols) {
        validateMatrixIndices(row, col, rows, cols);
        size_t offset = computeMatrixOffset(row, col, cols, sizeof(T));
        *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset) = value;
    }
    
    /**
     * @brief Get a row from matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param row Row index
     * @param rows Total rows
     * @param cols Total columns
     * @param result Pointer to store the row data
     */
    template<typename T>
    static void getRow(const void* data, size_t row, size_t rows, size_t cols, T* result) {
        if (row >= rows) {
            throw std::out_of_range("Row index out of bounds: " + std::to_string(row));
        }
        
        size_t offset = computeMatrixOffset(row, 0, cols, sizeof(T));
        std::memcpy(result, static_cast<const uint8_t*>(data) + offset, cols * sizeof(T));
    }
    
    /**
     * @brief Set a row in matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param row Row index
     * @param rows Total rows
     * @param cols Total columns
     * @param values Pointer to the source row data
     */
    template<typename T>
    static void setRow(void* data, size_t row, size_t rows, size_t cols, const T* values) {
        if (row >= rows) {
            throw std::out_of_range("Row index out of bounds: " + std::to_string(row));
        }
        
        size_t offset = computeMatrixOffset(row, 0, cols, sizeof(T));
        std::memcpy(static_cast<uint8_t*>(data) + offset, values, cols * sizeof(T));
    }
    
    /**
     * @brief Get a column from matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param col Column index
     * @param rows Total rows
     * @param cols Total columns
     * @param result Pointer to store the column data
     */
    template<typename T>
    static void getColumn(const void* data, size_t col, size_t rows, size_t cols, T* result) {
        if (col >= cols) {
            throw std::out_of_range("Column index out of bounds: " + std::to_string(col));
        }
        
        for (size_t i = 0; i < rows; i++) {
            size_t offset = computeMatrixOffset(i, col, cols, sizeof(T));
            result[i] = *reinterpret_cast<const T*>(static_cast<const uint8_t*>(data) + offset);
        }
    }
    
    /**
     * @brief Set a column in matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param col Column index
     * @param rows Total rows
     * @param cols Total columns
     * @param values Pointer to the source column data
     */
    template<typename T>
    static void setColumn(void* data, size_t col, size_t rows, size_t cols, const T* values) {
        if (col >= cols) {
            throw std::out_of_range("Column index out of bounds: " + std::to_string(col));
        }
        
        for (size_t i = 0; i < rows; i++) {
            size_t offset = computeMatrixOffset(i, col, cols, sizeof(T));
            *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset) = values[i];
        }
    }
    
    /**
     * @brief Perform matrix transpose in-place (square matrices only)
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @param size Size of the square matrix (rows = cols)
     */
    template<typename T>
    static void transposeInPlace(void* data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            for (size_t j = i + 1; j < size; j++) {
                size_t offset1 = computeMatrixOffset(i, j, size, sizeof(T));
                size_t offset2 = computeMatrixOffset(j, i, size, sizeof(T));
                
                T temp = *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset1);
                *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset1) = 
                    *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset2);
                *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset2) = temp;
            }
        }
    }
    
    /**
     * @brief Perform matrix transpose (general case)
     * 
     * @tparam T Element type
     * @param src Pointer to source matrix data
     * @param dest Pointer to destination matrix data
     * @param rows Source matrix rows (destination columns)
     * @param cols Source matrix columns (destination rows)
     */
    template<typename T>
    static void transpose(const void* src, void* dest, size_t rows, size_t cols) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                size_t srcOffset = computeMatrixOffset(i, j, cols, sizeof(T));
                size_t destOffset = computeMatrixOffset(j, i, rows, sizeof(T));
                
                *reinterpret_cast<T*>(static_cast<uint8_t*>(dest) + destOffset) = 
                    *reinterpret_cast<const T*>(static_cast<const uint8_t*>(src) + srcOffset);
            }
        }
    }
    
    /**
     * @brief Calculate matrix determinant for 2x2 matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @return Determinant value
     */
    template<typename T>
    static T determinant2x2(const void* data) {
        const T* elements = reinterpret_cast<const T*>(data);
        return elements[0] * elements[3] - elements[1] * elements[2];
    }
    
    /**
     * @brief Calculate matrix determinant for 3x3 matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @return Determinant value
     */
    template<typename T>
    static T determinant3x3(const void* data) {
        const T* m = reinterpret_cast<const T*>(data);
        
        return m[0] * (m[4] * m[8] - m[5] * m[7]) -
               m[1] * (m[3] * m[8] - m[5] * m[6]) +
               m[2] * (m[3] * m[7] - m[4] * m[6]);
    }
    
    /**
     * @brief Calculate matrix determinant for 4x4 matrix
     * 
     * @tparam T Element type
     * @param data Pointer to matrix data
     * @return Determinant value
     */
    template<typename T>
    static T determinant4x4(const void* data) {
        const T* m = reinterpret_cast<const T*>(data);
        
        // Calculate determinants of 3x3 submatrices
        T submatrix[9];
        
        // For m[0]
        submatrix[0] = m[5];  submatrix[1] = m[6];  submatrix[2] = m[7];
        submatrix[3] = m[9];  submatrix[4] = m[10]; submatrix[5] = m[11];
        submatrix[6] = m[13]; submatrix[7] = m[14]; submatrix[8] = m[15];
        T det0 = m[0] * determinant3x3<T>(submatrix);
        
        // For m[1]
        submatrix[0] = m[4];  submatrix[1] = m[6];  submatrix[2] = m[7];
        submatrix[3] = m[8];  submatrix[4] = m[10]; submatrix[5] = m[11];
        submatrix[6] = m[12]; submatrix[7] = m[14]; submatrix[8] = m[15];
        T det1 = m[1] * determinant3x3<T>(submatrix);
        
        // For m[2]
        submatrix[0] = m[4];  submatrix[1] = m[5];  submatrix[2] = m[7];
        submatrix[3] = m[8];  submatrix[4] = m[9];  submatrix[5] = m[11];
        submatrix[6] = m[12]; submatrix[7] = m[13]; submatrix[8] = m[15];
        T det2 = m[2] * determinant3x3<T>(submatrix);
        
        // For m[3]
        submatrix[0] = m[4];  submatrix[1] = m[5];  submatrix[2] = m[6];
        submatrix[3] = m[8];  submatrix[4] = m[9];  submatrix[5] = m[10];
        submatrix[6] = m[12]; submatrix[7] = m[13]; submatrix[8] = m[14];
        T det3 = m[3] * determinant3x3<T>(submatrix);
        
        return det0 - det1 + det2 - det3;
    }
    
    /**
     * @brief Invert a 2x2 matrix
     * 
     * @tparam T Element type
     * @param src Pointer to source matrix data
     * @param dest Pointer to destination matrix data
     * @return true if inversion successful, false if matrix is singular
     */
    template<typename T>
    static bool inverse2x2(const void* src, void* dest) {
        const T* s = reinterpret_cast<const T*>(src);
        T* d = reinterpret_cast<T*>(dest);
        
        T det = determinant2x2<T>(src);
        if (std::abs(det) < std::numeric_limits<T>::epsilon()) {
            return false; // Singular matrix
        }
        
        T invDet = static_cast<T>(1) / det;
        
        d[0] = s[3] * invDet;
        d[1] = -s[1] * invDet;
        d[2] = -s[2] * invDet;
        d[3] = s[0] * invDet;
        
        return true;
    }
    
    /**
     * @brief Invert a 3x3 matrix
     * 
     * @tparam T Element type
     * @param src Pointer to source matrix data
     * @param dest Pointer to destination matrix data
     * @return true if inversion successful, false if matrix is singular
     */
    template<typename T>
    static bool inverse3x3(const void* src, void* dest) {
        const T* s = reinterpret_cast<const T*>(src);
        T* d = reinterpret_cast<T*>(dest);
        
        T det = determinant3x3<T>(src);
        if (std::abs(det) < std::numeric_limits<T>::epsilon()) {
            return false; // Singular matrix
        }
        
        T invDet = static_cast<T>(1) / det;
        
        // Calculate cofactors and adjugate
        d[0] = (s[4] * s[8] - s[5] * s[7]) * invDet;
        d[1] = (s[2] * s[7] - s[1] * s[8]) * invDet;
        d[2] = (s[1] * s[5] - s[2] * s[4]) * invDet;
        
        d[3] = (s[5] * s[6] - s[3] * s[8]) * invDet;
        d[4] = (s[0] * s[8] - s[2] * s[6]) * invDet;
        d[5] = (s[2] * s[3] - s[0] * s[5]) * invDet;
        
        d[6] = (s[3] * s[7] - s[4] * s[6]) * invDet;
        d[7] = (s[1] * s[6] - s[0] * s[7]) * invDet;
        d[8] = (s[0] * s[4] - s[1] * s[3]) * invDet;
        
        return true;
    }
    
    /**
     * @brief Perform matrix multiplication
     * 
     * @tparam T Element type
     * @param a Pointer to first matrix data
     * @param b Pointer to second matrix data
     * @param c Pointer to result matrix data
     * @param aRows Rows in first matrix
     * @param aCols Columns in first matrix
     * @param bRows Rows in second matrix
     * @param bCols Columns in second matrix
     * @return true if multiplication is possible, false otherwise
     */
    template<typename T>
    static bool multiply(const void* a, const void* b, void* c, 
                         size_t aRows, size_t aCols, 
                         size_t bRows, size_t bCols) {
        // Check compatibility
        if (aCols != bRows) {
            return false; // Incompatible dimensions
        }
        
        const T* aData = reinterpret_cast<const T*>(a);
        const T* bData = reinterpret_cast<const T*>(b);
        T* cData = reinterpret_cast<T*>(c);
        
        // Initialize result to zero
        std::memset(cData, 0, aRows * bCols * sizeof(T));
        
        // Matrix multiplication
        for (size_t i = 0; i < aRows; i++) {
            for (size_t j = 0; j < bCols; j++) {
                for (size_t k = 0; k < aCols; k++) {
                    cData[i * bCols + j] += aData[i * aCols + k] * bData[k * bCols + j];
                }
            }
        }
        
        return true;
    }
    
    /**
     * @brief Calculate matrix-vector multiplication (matrix * vector)
     * 
     * @tparam T Element type
     * @param matrix Pointer to matrix data
     * @param vector Pointer to vector data
     * @param result Pointer to result vector data
     * @param rows Matrix rows
     * @param cols Matrix columns (must equal vector length)
     * @return true if multiplication is possible, false otherwise
     */
    template<typename T>
    static bool multiplyMatrixVector(const void* matrix, const void* vector, void* result,
                                     size_t rows, size_t cols) {
        const T* matData = reinterpret_cast<const T*>(matrix);
        const T* vecData = reinterpret_cast<const T*>(vector);
        T* resData = reinterpret_cast<T*>(result);
        
        // Initialize result to zero
        std::memset(resData, 0, rows * sizeof(T));
        
        // Matrix-vector multiplication
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                resData[i] += matData[i * cols + j] * vecData[j];
            }
        }
        
        return true;
    }
};

} // anonymous namespace

// Matrix helper functions that can be exposed in a future version

/**
 * @brief Get a matrix element
 * 
 * @param matrix Matrix data
 * @param row Row index
 * @param col Column index
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Vector of bytes representing the element value
 */
std::vector<uint8_t> getMatrixElement(const std::vector<uint8_t>& matrix, 
                                     size_t row, size_t col,
                                     const std::shared_ptr<Type>& elementType,
                                     size_t rows, size_t cols) {
    validateMatrixIndices(row, col, rows, cols);
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeMatrixOffset(row, col, cols, elementSize);
    
    // Check bounds
    if (offset + elementSize > matrix.size()) {
        throw std::out_of_range("Matrix element access out of bounds");
    }
    
    // Extract the element
    return std::vector<uint8_t>(matrix.begin() + offset, 
                               matrix.begin() + offset + elementSize);
}

/**
 * @brief Set a matrix element
 * 
 * @param matrix Matrix data to modify
 * @param row Row index
 * @param col Column index
 * @param value Element value to set
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 */
void setMatrixElement(std::vector<uint8_t>& matrix, 
                      size_t row, size_t col,
                      const std::vector<uint8_t>& value,
                      const std::shared_ptr<Type>& elementType,
                      size_t rows, size_t cols) {
    validateMatrixIndices(row, col, rows, cols);
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeMatrixOffset(row, col, cols, elementSize);
    
    // Check bounds
    if (offset + elementSize > matrix.size()) {
        throw std::out_of_range("Matrix element access out of bounds");
    }
    
    // Check value size
    if (value.size() != elementSize) {
        throw std::invalid_argument("Element value size does not match element type size");
    }
    
    // Copy the value
    std::copy(value.begin(), value.end(), matrix.begin() + offset);
}

/**
 * @brief Get an entire row from a matrix
 * 
 * @param matrix Matrix data
 * @param row Row index
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Vector of bytes representing the row data
 */
std::vector<uint8_t> getMatrixRow(const std::vector<uint8_t>& matrix, 
                                 size_t row,
                                 const std::shared_ptr<Type>& elementType,
                                 size_t rows, size_t cols) {
    if (row >= rows) {
        throw std::out_of_range("Row index out of bounds: " + std::to_string(row));
    }
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeMatrixOffset(row, 0, cols, elementSize);
    size_t rowSize = cols * elementSize;
    
    // Check bounds
    if (offset + rowSize > matrix.size()) {
        throw std::out_of_range("Matrix row access out of bounds");
    }
    
    // Extract the row
    return std::vector<uint8_t>(matrix.begin() + offset, 
                               matrix.begin() + offset + rowSize);
}

/**
 * @brief Set an entire row in a matrix
 * 
 * @param matrix Matrix data to modify
 * @param row Row index
 * @param values Row data to set
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 */
void setMatrixRow(std::vector<uint8_t>& matrix, 
                  size_t row,
                  const std::vector<uint8_t>& values,
                  const std::shared_ptr<Type>& elementType,
                  size_t rows, size_t cols) {
    if (row >= rows) {
        throw std::out_of_range("Row index out of bounds: " + std::to_string(row));
    }
    
    size_t elementSize = elementType->getSize();
    size_t offset = computeMatrixOffset(row, 0, cols, elementSize);
    size_t rowSize = cols * elementSize;
    
    // Check bounds
    if (offset + rowSize > matrix.size()) {
        throw std::out_of_range("Matrix row access out of bounds");
    }
    
    // Check values size
    if (values.size() != rowSize) {
        throw std::invalid_argument("Row values size does not match expected row size");
    }
    
    // Copy the values
    std::copy(values.begin(), values.end(), matrix.begin() + offset);
}

/**
 * @brief Get an entire column from a matrix
 * 
 * @param matrix Matrix data
 * @param col Column index
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Vector of bytes representing the column data
 */
std::vector<uint8_t> getMatrixColumn(const std::vector<uint8_t>& matrix, 
                                    size_t col,
                                    const std::shared_ptr<Type>& elementType,
                                    size_t rows, size_t cols) {
    if (col >= cols) {
        throw std::out_of_range("Column index out of bounds: " + std::to_string(col));
    }
    
    size_t elementSize = elementType->getSize();
    std::vector<uint8_t> result(rows * elementSize);
    
    for (size_t i = 0; i < rows; i++) {
        size_t offset = computeMatrixOffset(i, col, cols, elementSize);
        
        // Check bounds
        if (offset + elementSize > matrix.size()) {
            throw std::out_of_range("Matrix column access out of bounds");
        }
        
        // Copy element
        std::copy(matrix.begin() + offset, 
                 matrix.begin() + offset + elementSize,
                 result.begin() + i * elementSize);
    }
    
    return result;
}

/**
 * @brief Set an entire column in a matrix
 * 
 * @param matrix Matrix data to modify
 * @param col Column index
 * @param values Column data to set
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 */
void setMatrixColumn(std::vector<uint8_t>& matrix, 
                     size_t col,
                     const std::vector<uint8_t>& values,
                     const std::shared_ptr<Type>& elementType,
                     size_t rows, size_t cols) {
    if (col >= cols) {
        throw std::out_of_range("Column index out of bounds: " + std::to_string(col));
    }
    
    size_t elementSize = elementType->getSize();
    
    // Check values size
    if (values.size() != rows * elementSize) {
        throw std::invalid_argument("Column values size does not match expected column size");
    }
    
    for (size_t i = 0; i < rows; i++) {
        size_t offset = computeMatrixOffset(i, col, cols, elementSize);
        
        // Check bounds
        if (offset + elementSize > matrix.size()) {
            throw std::out_of_range("Matrix column access out of bounds");
        }
        
        // Copy element
        std::copy(values.begin() + i * elementSize, 
                 values.begin() + (i + 1) * elementSize,
                 matrix.begin() + offset);
    }
}

/**
 * @brief Transpose a matrix
 * 
 * @param matrix Matrix data
 * @param elementType Type of matrix elements
 * @param rows Number of rows
 * @param cols Number of columns
 * @return Vector of bytes representing the transposed matrix
 */
std::vector<uint8_t> transposeMatrix(const std::vector<uint8_t>& matrix,
                                    const std::shared_ptr<Type>& elementType,
                                    size_t rows, size_t cols) {
    size_t elementSize = elementType->getSize();
    std::vector<uint8_t> result(cols * rows * elementSize);
    
    // Special case for square matrices - could be done in-place
    if (rows == cols) {
        // Copy first
        result = matrix;
        
        // Handle different element types
        if (elementType->getOpCode() == Type::OpCode::INT8) {
            MatrixOperations::transposeInPlace<int8_t>(result.data(), rows);
        } else if (elementType->getOpCode() == Type::OpCode::INT16) {
            MatrixOperations::transposeInPlace<int16_t>(result.data(), rows);
        } else if (elementType->getOpCode() == Type::OpCode::INT32) {
            MatrixOperations::transposeInPlace<int32_t>(result.data(), rows);
        } else if (elementType->getOpCode() == Type::OpCode::INT64) {
            MatrixOperations::transposeInPlace<int64_t>(result.data(), rows);
        } else if (elementType->getOpCode() == Type::OpCode::FP32) {
            MatrixOperations::transposeInPlace<float>(result.data(), rows);
        } else if (elementType->getOpCode() == Type::OpCode::FP64) {
            MatrixOperations::transposeInPlace<double>(result.data(), rows);
        } else {
            // Generic byte-by-byte transpose for unsupported types
            for (size_t i = 0; i < rows; i++) {
                for (size_t j = i + 1; j < cols; j++) {
                    size_t offset1 = computeMatrixOffset(i, j, cols, elementSize);
                    size_t offset2 = computeMatrixOffset(j, i, cols, elementSize);
                    
                    for (size_t k = 0; k < elementSize; k++) {
                        std::swap(result[offset1 + k], result[offset2 + k]);
                    }
                }
            }
        }
    } else {
        // Non-square matrix - need to create a new matrix
        if (elementType->getOpCode() == Type::OpCode::INT8) {
            MatrixOperations::transpose<int8_t>(matrix.data(), result.data(), rows, cols);
        } else if (elementType->getOpCode() == Type::OpCode::INT16) {
            MatrixOperations::transpose<int16_t>(matrix.data(), result.data(), rows, cols);
        } else if (elementType->getOpCode() == Type::OpCode::INT32) {
            MatrixOperations::transpose<int32_t>(matrix.data(), result.data(), rows, cols);
        } else if (elementType->getOpCode() == Type::OpCode::INT64) {
            MatrixOperations::transpose<int64_t>(matrix.data(), result.data(), rows, cols);
        } else if (elementType->getOpCode() == Type::OpCode::FP32) {
            MatrixOperations::transpose<float>(matrix.data(), result.data(), rows, cols);
        } else if (elementType->getOpCode() == Type::OpCode::FP64) {
            MatrixOperations::transpose<double>(matrix.data(), result.data(), rows, cols);
        } else {
            // Generic byte-by-byte transpose for unsupported types
            for (size_t i = 0; i < rows; i++) {
                for (size_t j = 0; j < cols; j++) {
                    size_t srcOffset = computeMatrixOffset(i, j, cols, elementSize);
                    size_t destOffset = computeMatrixOffset(j, i, rows, elementSize);
                    
                    for (size_t k = 0; k < elementSize; k++) {
                        result[destOffset + k] = matrix[srcOffset + k];
                    }
                }
            }
        }
    }
    
    return result;
}

/**
 * @brief Calculate matrix determinant
 * 
 * @param matrix Matrix data
 * @param elementType Type of matrix elements
 * @param size Size of the square matrix
 * @return Vector of bytes representing the determinant value
 */
std::vector<uint8_t> matrixDeterminant(const std::vector<uint8_t>& matrix,
                                      const std::shared_ptr<Type>& elementType,
                                      size_t size) {
    size_t elementSize = elementType->getSize();
    std::vector<uint8_t> result(elementSize);
    
    // Only support square matrices of size 2, 3, or 4
    if (size < 2 || size > 4) {
        throw std::invalid_argument("Determinant only supported for 2x2, 3x3, and 4x4 matrices");
    }
    
    // Check matrix size
    if (matrix.size() < size * size * elementSize) {
        throw std::out_of_range("Matrix data size is smaller than expected");
    }
    
    // Calculate determinant based on element type
    if (elementType->getOpCode() == Type::OpCode::INT32) {
        int32_t det;
        if (size == 2) {
            det = MatrixOperations::determinant2x2<int32_t>(matrix.data());
        } else if (size == 3) {
            det = MatrixOperations::determinant3x3<int32_t>(matrix.data());
        } else { // size == 4
            det = MatrixOperations::determinant4x4<int32_t>(matrix.data());
        }
        std::memcpy(result.data(), &det, sizeof(int32_t));
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        int64_t det;
        if (size == 2) {
            det = MatrixOperations::determinant2x2<int64_t>(matrix.data());
        } else if (size == 3) {
            det = MatrixOperations::determinant3x3<int64_t>(matrix.data());
        } else { // size == 4
            det = MatrixOperations::determinant4x4<int64_t>(matrix.data());
        }
        std::memcpy(result.data(), &det, sizeof(int64_t));
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        float det;
        if (size == 2) {
            det = MatrixOperations::determinant2x2<float>(matrix.data());
        } else if (size == 3) {
            det = MatrixOperations::determinant3x3<float>(matrix.data());
        } else { // size == 4
            det = MatrixOperations::determinant4x4<float>(matrix.data());
        }
        std::memcpy(result.data(), &det, sizeof(float));
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        double det;
        if (size == 2) {
            det = MatrixOperations::determinant2x2<double>(matrix.data());
        } else if (size == 3) {
            det = MatrixOperations::determinant3x3<double>(matrix.data());
        } else { // size == 4
            det = MatrixOperations::determinant4x4<double>(matrix.data());
        }
        std::memcpy(result.data(), &det, sizeof(double));
    } else {
        throw std::invalid_argument("Determinant only supported for INT32, INT64, FP32, and FP64 elements");
    }
    
    return result;
}

/**
 * @brief Calculate matrix inverse
 * 
 * @param matrix Matrix data
 * @param elementType Type of matrix elements
 * @param size Size of the square matrix
 * @return Vector of bytes representing the inverted matrix
 */
std::vector<uint8_t> matrixInverse(const std::vector<uint8_t>& matrix,
                                  const std::shared_ptr<Type>& elementType,
                                  size_t size) {
    size_t elementSize = elementType->getSize();
    std::vector<uint8_t> result(size * size * elementSize);
    
    // Only support square matrices of size 2 or 3
    if (size < 2 || size > 3) {
        throw std::invalid_argument("Matrix inversion only implemented for 2x2 and 3x3 matrices");
    }
    
    // Check matrix size
    if (matrix.size() < size * size * elementSize) {
        throw std::out_of_range("Matrix data size is smaller than expected");
    }
    
    bool success = false;
    
    // Calculate inverse based on element type
    if (elementType->getOpCode() == Type::OpCode::FP32) {
        if (size == 2) {
            success = MatrixOperations::inverse2x2<float>(matrix.data(), result.data());
        } else { // size == 3
            success = MatrixOperations::inverse3x3<float>(matrix.data(), result.data());
        }
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        if (size == 2) {
            success = MatrixOperations::inverse2x2<double>(matrix.data(), result.data());
        } else { // size == 3
            success = MatrixOperations::inverse3x3<double>(matrix.data(), result.data());
        }
    } else {
        throw std::invalid_argument("Matrix inversion only supported for FP32 and FP64 elements");
    }
    
    if (!success) {
        throw std::runtime_error("Matrix is singular and cannot be inverted");
    }
    
    return result;
}

/**
 * @brief Multiply two matrices
 * 
 * @param matrixA First matrix data
 * @param matrixB Second matrix data
 * @param elementType Type of matrix elements
 * @param aRows Rows in first matrix
 * @param aCols Columns in first matrix
 * @param bRows Rows in second matrix
 * @param bCols Columns in second matrix
 * @return Vector of bytes representing the result matrix
 */
std::vector<uint8_t> matrixMultiply(const std::vector<uint8_t>& matrixA,
                                   const std::vector<uint8_t>& matrixB,
                                   const std::shared_ptr<Type>& elementType,
                                   size_t aRows, size_t aCols,
                                   size_t bRows, size_t bCols) {
    // Check compatibility
    if (aCols != bRows) {
        throw std::invalid_argument("Incompatible matrix dimensions for multiplication");
    }
    
    size_t elementSize = elementType->getSize();
    std::vector<uint8_t> result(aRows * bCols * elementSize);
    
    // Check matrix sizes
    if (matrixA.size() < aRows * aCols * elementSize) {
        throw std::out_of_range("Matrix A data size is smaller than expected");
    }
    
    if (matrixB.size() < bRows * bCols * elementSize) {
        throw std::out_of_range("Matrix B data size is smaller than expected");
    }
    
    bool success = false;
    
    // Multiply based on element type
    if (elementType->getOpCode() == Type::OpCode::INT32) {
        success = MatrixOperations::multiply<int32_t>(
            matrixA.data(), matrixB.data(), result.data(),
            aRows, aCols, bRows, bCols);
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        success = MatrixOperations::multiply<int64_t>(
            matrixA.data(), matrixB.data(), result.data(),
            aRows, aCols, bRows, bCols);
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        success = MatrixOperations::multiply<float>(
            matrixA.data(), matrixB.data(), result.data(),
            aRows, aCols, bRows, bCols);
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        success = MatrixOperations::multiply<double>(
            matrixA.data(), matrixB.data(), result.data(),
            aRows, aCols, bRows, bCols);
    } else {
        throw std::invalid_argument("Matrix multiplication only supported for INT32, INT64, FP32, and FP64 elements");
    }
    
    if (!success) {
        throw std::runtime_error("Matrix multiplication failed");
    }
    
    return result;
}

/**
 * @brief Multiply a matrix by a vector
 * 
 * @param matrix Matrix data
 * @param vector Vector data
 * @param elementType Type of matrix and vector elements
 * @param rows Matrix rows
 * @param cols Matrix columns (must equal vector length)
 * @return Vector of bytes representing the result vector
 */
std::vector<uint8_t> matrixVectorMultiply(const std::vector<uint8_t>& matrix,
                                         const std::vector<uint8_t>& vector,
                                         const std::shared_ptr<Type>& elementType,
                                         size_t rows, size_t cols) {
    size_t elementSize = elementType->getSize();
    
    // Check sizes
    if (vector.size() != cols * elementSize) {
        throw std::invalid_argument("Vector size does not match matrix columns");
    }
    
    if (matrix.size() < rows * cols * elementSize) {
        throw std::out_of_range("Matrix data size is smaller than expected");
    }
    
    // Create result vector
    std::vector<uint8_t> result(rows * elementSize);
    
    bool success = false;
    
    // Multiply based on element type
    if (elementType->getOpCode() == Type::OpCode::INT32) {
        success = MatrixOperations::multiplyMatrixVector<int32_t>(
            matrix.data(), vector.data(), result.data(), rows, cols);
    } else if (elementType->getOpCode() == Type::OpCode::INT64) {
        success = MatrixOperations::multiplyMatrixVector<int64_t>(
            matrix.data(), vector.data(), result.data(), rows, cols);
    } else if (elementType->getOpCode() == Type::OpCode::FP32) {
        success = MatrixOperations::multiplyMatrixVector<float>(
            matrix.data(), vector.data(), result.data(), rows, cols);
    } else if (elementType->getOpCode() == Type::OpCode::FP64) {
        success = MatrixOperations::multiplyMatrixVector<double>(
            matrix.data(), vector.data(), result.data(), rows, cols);
    } else {
        throw std::invalid_argument("Matrix-vector multiplication only supported for INT32, INT64, FP32, and FP64 elements");
    }
    
    if (!success) {
        throw std::runtime_error("Matrix-vector multiplication failed");
    }
    
    return result;
}

} // namespace coil
