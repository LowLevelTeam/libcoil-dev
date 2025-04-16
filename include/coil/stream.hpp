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
* @brief Stream seek origin
*/
enum class SeekOrigin {
  Begin,     ///< Beginning of stream
  Current,   ///< Current position
  End        ///< End of stream
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
    * @brief Seek to absolute position in stream
    * @return Result of operation
    */
  virtual Result seek(size_t position) = 0;
  
  /**
    * @brief Seek relative to origin
    * @return Result of operation
    */
  virtual Result seekRelative(SeekOrigin origin, i64 offset);
  
  /**
    * @brief Get the size of the stream, if available
    * @return Size of stream or 0 if not available
    */
  virtual size_t size() const;
  
  /**
    * @brief Close the stream
    */
  virtual void close() = 0;
  
  /**
    * @brief Check if stream is open
    */
  virtual bool isOpen() const = 0;
  
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
  
  /**
    * @brief Read a string (fixed size buffer)
    * @return Number of bytes read including null terminator, or 0 on error
    */
  size_t readString(char* buffer, size_t maxSize);
  
  /**
    * @brief Write a string (null-terminated)
    * @return Number of bytes written including null terminator, or 0 on error
    */
  size_t writeString(const char* str);
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
  Result seekRelative(SeekOrigin origin, i64 offset) override;
  size_t size() const override;
  void close() override;
  bool isOpen() const override;
  
private:
  void* handle = nullptr;  ///< File handle (opaque)
  bool is_at_eof = false;  ///< EOF flag
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
  Result seekRelative(SeekOrigin origin, i64 offset) override;
  size_t size() const override;
  void close() override;
  bool isOpen() const override;
  
  /**
    * @brief Get the underlying buffer
    */
  const void* getBuffer() const { return buffer; }
  
  /**
    * @brief Get the current size of valid data
    */
  size_t getSize() const { return data_size; }
  
  /**
    * @brief Get buffer capacity
    */
  size_t getCapacity() const { return capacity; }
  
  /**
    * @brief Resize the buffer (only valid for owned buffers)
    */
  Result resize(size_t new_capacity);
  
private:
  u8* buffer = nullptr;    ///< Memory buffer
  size_t capacity = 0;     ///< Buffer capacity
  size_t position = 0;     ///< Current position
  size_t data_size = 0;    ///< Valid data size
  bool owns_buffer = false;///< Whether we own the buffer
  bool is_open = false;    ///< Whether stream is open
  StreamMode mode;         ///< Stream mode
};

} // namespace coil