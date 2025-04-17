/**
 * @file stream.hpp
 * @brief Modern stream abstraction for I/O using standard C++ streams
 */

#pragma once
#include "coil/types.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <cstring>
#include <functional>

namespace coil {

/**
 * @brief Stream modes
 */
enum class StreamMode {
  Read,      // Read-only
  Write,     // Write-only
  ReadWrite  // Read and write
};

/**
 * @brief Stream seek origin
 */
enum class SeekOrigin {
  Begin,     // Beginning of stream
  Current,   // Current position
  End        // End of stream
};

/**
 * @brief Modern stream interface
 */
class Stream {
public:
  virtual ~Stream() = default;
  
  /**
   * @brief Read data from the stream
   * @return Number of bytes read
   */
  virtual size_t read(void* buffer, size_t size) = 0;
  
  /**
   * @brief Write data to the stream
   * @return Number of bytes written
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
   */
  virtual void seek(size_t position) = 0;
  
  /**
   * @brief Seek relative to origin
   */
  virtual void seekRelative(SeekOrigin origin, int64_t offset) = 0;
  
  /**
   * @brief Get the size of the stream, if available
   */
  virtual size_t size() const = 0;
  
  /**
   * @brief Close the stream
   */
  virtual void close() = 0;
  
  /**
   * @brief Check if stream is open
   */
  virtual bool isOpen() const = 0;
  
  /**
   * @brief Read a string (fixed size buffer)
   * @return Number of bytes read including null terminator
   */
  size_t readString(char* buffer, size_t maxSize) {
    if (!buffer || maxSize == 0) {
      return 0;
    }
    
    buffer[0] = '\0';
    size_t i = 0;
    char c;
    
    while (i < maxSize - 1) {
      if (read(&c, 1) != 1) {
        break;
      }
      
      buffer[i++] = c;
      
      // Stop at null terminator
      if (c == '\0') {
        return i;
      }
    }
    
    // Ensure null termination
    buffer[i] = '\0';
    return i;
  }
  
  /**
   * @brief Write a string (null-terminated)
   * @return Number of bytes written including null terminator
   */
  size_t writeString(const char* str) {
    if (!str) {
      return 0;
    }
    
    size_t len = strlen(str) + 1; // Include null terminator
    return write(str, len);
  }
  
  /**
   * @brief Read a typed value
   */
  template<typename T>
  T readv() {
    T value;
    if (read(&value, sizeof(T)) != sizeof(T)) {
      throw IOException("Failed to read value");
    }
    return value;
  }
  
  /**
   * @brief Write a typed value
   */
  template<typename T>
  void writev(const T& value) {
    if (write(&value, sizeof(T)) != sizeof(T)) {
      throw IOException("Failed to write value");
    }
  }
};

/**
 * @brief File-based stream using std::fstream
 */
class FileStream : public Stream {
public:
  /**
   * @brief Create a file stream
   */
  FileStream(const std::string& filename, StreamMode mode) 
    : m_isAtEof(false) {
    
    std::ios_base::openmode openMode = std::ios::binary;
    
    switch (mode) {
      case StreamMode::Read:
        openMode |= std::ios::in;
        break;
      case StreamMode::Write:
        openMode |= std::ios::out;
        break;
      case StreamMode::ReadWrite:
        openMode |= std::ios::in | std::ios::out;
        break;
    }
    
    m_file.open(filename, openMode);
    if (!m_file) {
      throw IOException("Failed to open file: " + filename);
    }
  }
  
  /**
   * @brief Destructor
   */
  ~FileStream() override {
    close();
  }
  
  size_t read(void* buffer, size_t size) override {
    if (!m_file.is_open()) {
      return 0;
    }
    
    m_isAtEof = false;
    m_file.read(static_cast<char*>(buffer), size);
    size_t bytesRead = static_cast<size_t>(m_file.gcount());
    
    if (bytesRead < size) {
      m_isAtEof = m_file.eof();
    }
    
    return bytesRead;
  }
  
  size_t write(const void* buffer, size_t size) override {
    if (!m_file.is_open()) {
      return 0;
    }
    
    auto pos = m_file.tellp();
    m_file.write(static_cast<const char*>(buffer), size);
    
    if (!m_file) {
      return 0;
    }
    
    return static_cast<size_t>(m_file.tellp() - pos);
  }
  
  bool eof() const override {
    return m_isAtEof || (m_file.is_open() && m_file.eof());
  }
  
  size_t tell() const override {
    if (!m_file.is_open()) {
      return 0;
    }
    
    return static_cast<size_t>(m_file.tellg());
  }
  
  void seek(size_t position) override {
    if (!m_file.is_open()) {
      throw IOException("Cannot seek in closed file");
    }
    
    m_isAtEof = false;
    m_file.seekg(position);
    m_file.seekp(position);
    
    if (!m_file) {
      throw IOException("Failed to seek to position " + std::to_string(position));
    }
  }
  
  void seekRelative(SeekOrigin origin, int64_t offset) override {
    if (!m_file.is_open()) {
      throw IOException("Cannot seek in closed file");
    }
    
    std::ios_base::seekdir dir;
    switch (origin) {
      case SeekOrigin::Begin:
        dir = std::ios_base::beg;
        break;
      case SeekOrigin::Current:
        dir = std::ios_base::cur;
        break;
      case SeekOrigin::End:
        dir = std::ios_base::end;
        break;
      default:
        throw InvalidArgException("Invalid seek origin");
    }
    
    m_isAtEof = false;
    m_file.seekg(offset, dir);
    m_file.seekp(offset, dir);
    
    if (!m_file) {
      throw IOException("Failed to seek to offset " + std::to_string(offset));
    }
  }
  
  size_t size() const override {
    if (!m_file.is_open()) {
      return 0;
    }
    
    auto currentPos = m_file.tellg();
    m_file.seekg(0, std::ios::end);
    auto size = m_file.tellg();
    m_file.seekg(currentPos);
    
    return static_cast<size_t>(size);
  }
  
  void close() override {
    if (m_file.is_open()) {
      m_file.close();
    }
    m_isAtEof = false;
  }
  
  bool isOpen() const override {
    return m_file.is_open();
  }
  
private:
  mutable std::fstream m_file;
  bool m_isAtEof;
};

/**
 * @brief Memory-based stream
 */
class MemoryStream : public Stream {
public:
  /**
   * @brief Create a memory stream with an optional external buffer
   */
  MemoryStream(void* buffer, size_t size, StreamMode mode)
    : m_buffer(nullptr), 
      m_capacity(size), 
      m_position(0), 
      m_dataSize(0), 
      m_ownsBuffer(false), 
      m_isOpen(false), 
      m_mode(mode) {
    
    // If no buffer provided, allocate one
    if (!buffer && size > 0) {
      m_buffer = std::make_unique<u8[]>(size);
      m_ownsBuffer = true;
      m_isOpen = true;
      
      // Initialize the buffer if it's for writing
      if (mode != StreamMode::Read) {
        std::memset(m_buffer.get(), 0, size);
      }
    } else if (buffer) {
      // Use external buffer - no ownership
      m_buffer = std::unique_ptr<u8[], std::function<void(u8*)>>(
        static_cast<u8*>(buffer), 
        [](u8*) {} // No-op deleter for external buffer
      );
      m_isOpen = true;
    }
    
    // For existing buffers, assume they're filled with data
    if (buffer && mode != StreamMode::Write) {
      m_dataSize = size;
    }
  }
  
  /**
   * @brief Create an empty memory stream
   */
  explicit MemoryStream(size_t initialCapacity = 1024) 
    : MemoryStream(nullptr, initialCapacity, StreamMode::ReadWrite) {}
  
  /**
   * @brief Create from vector data
   */
  explicit MemoryStream(const std::vector<u8>& data) 
    : m_capacity(data.size()), 
      m_position(0), 
      m_dataSize(data.size()), 
      m_ownsBuffer(true), 
      m_isOpen(true), 
      m_mode(StreamMode::ReadWrite) {
    
    m_buffer = std::make_unique<u8[]>(data.size());
    std::memcpy(m_buffer.get(), data.data(), data.size());
  }
  
  size_t read(void* dest_buffer, size_t size) override {
    if (!m_buffer || !m_isOpen || m_mode == StreamMode::Write || m_position >= m_dataSize) {
      return 0;
    }
    
    size_t available = m_dataSize - m_position;
    size_t to_read = std::min(size, available);
    
    if (to_read > 0) {
      std::memcpy(dest_buffer, m_buffer.get() + m_position, to_read);
      m_position += to_read;
    }
    
    return to_read;
  }
  
  size_t write(const void* src_buffer, size_t size) override {
    if (!m_buffer || !m_isOpen || m_mode == StreamMode::Read) {
      return 0;
    }
    
    // Resize if needed and this is our buffer
    if (m_ownsBuffer && m_position + size > m_capacity) {
      resize(nextPowerOfTwo(m_position + size));
    }
    
    size_t available = m_capacity - m_position;
    size_t to_write = std::min(size, available);
    
    if (to_write > 0) {
      std::memcpy(m_buffer.get() + m_position, src_buffer, to_write);
      m_position += to_write;
      
      m_dataSize = std::max(m_dataSize, m_position);
    }
    
    return to_write;
  }
  
  bool eof() const override {
    return !m_buffer || !m_isOpen || m_position >= m_dataSize;
  }
  
  size_t tell() const override {
    return m_position;
  }
  
  void seek(size_t position) override {
    if (!m_buffer || !m_isOpen) {
      throw IOException("Cannot seek in closed memory stream");
    }
    
    if (position > m_capacity) {
      throw InvalidArgException("Seek position beyond capacity");
    }
    
    m_position = position;
    
    // If writing and position is beyond current data size, update data size
    if (m_mode != StreamMode::Read && m_position > m_dataSize) {
      m_dataSize = m_position;
    }
  }
  
  void seekRelative(SeekOrigin origin, int64_t offset) override {
    if (!m_buffer || !m_isOpen) {
      throw IOException("Cannot seek in closed memory stream");
    }
    
    size_t base_pos;
    switch (origin) {
      case SeekOrigin::Begin:
        base_pos = 0;
        break;
      case SeekOrigin::Current:
        base_pos = m_position;
        break;
      case SeekOrigin::End:
        base_pos = m_dataSize;
        break;
      default:
        throw InvalidArgException("Invalid seek origin");
    }
    
    // Calculate new position
    int64_t new_pos = static_cast<int64_t>(base_pos) + offset;
    if (new_pos < 0) {
      throw InvalidArgException("Seek position would be negative: " + std::to_string(new_pos));
    }
    
    seek(static_cast<size_t>(new_pos));
  }
  
  size_t size() const override {
    return m_dataSize;
  }
  
  void close() override {
    if (m_ownsBuffer) {
      m_buffer.reset();
    } else {
      m_buffer.reset(nullptr);
    }
    
    m_capacity = 0;
    m_position = 0;
    m_dataSize = 0;
    m_isOpen = false;
  }
  
  bool isOpen() const override {
    return m_isOpen;
  }
  
  /**
   * @brief Get data as vector
   */
  std::vector<u8> getData() const {
    if (!m_buffer || m_dataSize == 0) {
      return {};
    }
    
    return std::vector<u8>(m_buffer.get(), m_buffer.get() + m_dataSize);
  }
  
  /**
   * @brief Get pointer to data
   */
  const u8* getBufferPointer() const {
    return m_buffer.get();
  }
  
  /**
   * @brief Get capacity
   */
  size_t getCapacity() const {
    return m_capacity;
  }
  
  /**
   * @brief Resize the buffer
   */
  void resize(size_t newCapacity) {
    if (!m_ownsBuffer) {
      throw NotSupportedException("Cannot resize non-owned buffer");
    }
    
    if (newCapacity == 0) {
      throw InvalidArgException("Cannot resize to zero capacity");
    }
    
    if (newCapacity <= m_capacity) {
      return; // No need to resize down
    }
    
    // Allocate new buffer
    auto newBuffer = std::make_unique<u8[]>(newCapacity);
    
    // Copy existing data
    if (m_buffer && m_dataSize > 0) {
      std::memcpy(newBuffer.get(), m_buffer.get(), m_dataSize);
    }
    
    // Initialize new memory
    if (newCapacity > m_capacity) {
      std::memset(newBuffer.get() + m_capacity, 0, newCapacity - m_capacity);
    }
    
    // Update buffer
    m_buffer = std::move(newBuffer);
    m_capacity = newCapacity;
  }
  
private:
  std::unique_ptr<u8[], std::function<void(u8*)>> m_buffer;
  size_t m_capacity;
  size_t m_position;
  size_t m_dataSize;
  bool m_ownsBuffer;
  bool m_isOpen;
  StreamMode m_mode;
};

} // namespace coil