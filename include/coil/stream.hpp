/**
* @file stream.hpp
* @brief Stream abstraction for I/O
*/

#pragma once
#include "coil/types.hpp"
#include <cstddef>

namespace coil {

/**
* @brief Stream modes
*/
enum class StreamMode {
  Read,      ///< Read-only
  Write,     ///< Write-only
  ReadWrite  ///< Read and write
};

/**
* @brief Base stream interface
*/
class Stream {
public:
  /**
    * @brief Virtual destructor
    */
  virtual ~Stream() = default;
  
  /**
    * @brief Read data from the stream
    * @return Number of bytes read, or 0 on EOF/error
    */
  virtual size_t read(void* buffer, size_t size) = 0;
  
  /**
    * @brief Write data to the stream
    * @return Number of bytes written, or 0 on error
    */
  virtual size_t write(const void* buffer, size_t size) = 0;
  
  /**
    * @brief Check if end of stream reached
    */
  virtual bool eof() const = 0;
  
  /**
    * @brief Get current position in stream
    */
  virtual size_t tell() const = 0;
  
  /**
    * @brief Seek to position in stream
    * @return Result of operation
    */
  virtual Result seek(size_t position) = 0;
  
  /**
    * @brief Close the stream
    */
  virtual void close() = 0;
  
  /**
    * @brief Read a single typed value
    */
  template<typename T>
  Result readValue(T& value) {
      if (read(&value, sizeof(T)) == sizeof(T)) {
          return Result::Success;
      }
      return Result::IoError;
  }
  
  /**
    * @brief Write a single typed value
    */
  template<typename T>
  Result writeValue(const T& value) {
      if (write(&value, sizeof(T)) == sizeof(T)) {
          return Result::Success;
      }
      return Result::IoError;
  }
};

/**
* @brief File-based stream
*/
class FileStream : public Stream {
public:
  /**
    * @brief Create a file stream
    */
  FileStream(const char* filename, StreamMode mode);
  
  /**
    * @brief Destructor
    */
  ~FileStream();
  
  // Stream implementation
  size_t read(void* buffer, size_t size) override;
  size_t write(const void* buffer, size_t size) override;
  bool eof() const override;
  size_t tell() const override;
  Result seek(size_t position) override;
  void close() override;
  
private:
  void* handle = nullptr;  ///< File handle (opaque)
};

/**
* @brief Memory-based stream
* 
* Provides a stream interface over a memory buffer.
* If not provided, the buffer is allocated and owned by the stream.
*/
class MemoryStream : public Stream {
public:
  /**
    * @brief Create a memory stream
    * If buffer is nullptr, one will be allocated of 'size' bytes
    */
  MemoryStream(void* buffer, size_t size, StreamMode mode);
  
  /**
    * @brief Destructor
    */
  ~MemoryStream();
  
  // Stream implementation
  size_t read(void* buffer, size_t size) override;
  size_t write(const void* buffer, size_t size) override;
  bool eof() const override;
  size_t tell() const override;
  Result seek(size_t position) override;
  void close() override;
  
  /**
    * @brief Get the underlying buffer
    */
  const void* getBuffer() const { return buffer; }
  
  /**
    * @brief Get the current size of valid data
    */
  size_t getSize() const { return data_size; }
  
private:
  u8* buffer = nullptr;    ///< Memory buffer
  size_t capacity = 0;     ///< Buffer capacity
  size_t position = 0;     ///< Current position
  size_t data_size = 0;    ///< Valid data size
  bool owns_buffer = false;///< Whether we own the buffer
  StreamMode mode;         ///< Stream mode
};

} // namespace coil