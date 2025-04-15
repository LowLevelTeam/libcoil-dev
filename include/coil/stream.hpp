#pragma once

#include "coil/err.hpp"
#include <string>
#include <cstdint>
#include <algorithm>
#include <cstring>

namespace coil {

/**
* @brief Stream flags (bitfield)
*/
enum StreamFlags : uint32_t {
  Read  = (1 << 0),  ///< Stream is readable
  Write = (1 << 1),  ///< Stream is writable
  Eof   = (1 << 2)   ///< End of stream has been reached
};

/**
* @brief Stream base structure
* 
* This is the common interface for all streams
*/
struct Stream {
  // Position tracking for read/write operations
  StreamPosition readPosition;   // Current read position
  StreamPosition writePosition;  // Current write position
  uint32_t flags;                // Stream flags
  
  // Virtual function table - this approach allows C-like interfaces
  // while maintaining polymorphism without full OOP overhead
  struct VTable {
    bool (*eof)(const Stream* stream);
    void (*close)(Stream* stream);
    size_t (*read)(Stream* stream, void* buffer, size_t size);
    size_t (*write)(Stream* stream, const void* buffer, size_t size);
    void (*resetReadPosition)(Stream* stream);
    void (*resetWritePosition)(Stream* stream);
  };
  
  const VTable* vtable;  // Pointer to virtual function table
  
  /**
    * @brief Check if the end of stream has been reached
    * 
    * @return true if at end of stream
    */
  bool eof() const { return vtable->eof(this); }
  
  /**
    * @brief Close the stream
    */
  void close() { vtable->close(this); }
  
  /**
    * @brief Get stream flags
    * 
    * @return uint32_t Flags
    */
  uint32_t getFlags() const { return flags; }
  
  /**
    * @brief Get the current read position information
    * 
    * @return const StreamPosition& Read position
    */
  const StreamPosition& getReadPosition() const { return readPosition; }
  
  /**
    * @brief Get the current write position information
    * 
    * @return const StreamPosition& Write position
    */
  const StreamPosition& getWritePosition() const { return writePosition; }
  
  /**
    * @brief Get the current position information (for backward compatibility)
    * 
    * @return const StreamPosition& Current read position
    */
  const StreamPosition& getPosition() const { return readPosition; }
  
  /**
    * @brief Reset the read position to the beginning of the stream
    */
  void resetReadPosition() { vtable->resetReadPosition(this); }
  
  /**
    * @brief Reset the write position to the beginning of the stream
    */
  void resetWritePosition() { vtable->resetWritePosition(this); }
  
  /**
    * @brief Check if the stream is readable
    * 
    * @return true if readable
    */
  bool isReadable() const { return (flags & StreamFlags::Read) != 0; }
  
  /**
    * @brief Check if the stream is writable
    * 
    * @return true if writable
    */
  bool isWritable() const { return (flags & StreamFlags::Write) != 0; }
  
  /**
    * @brief Read data from the stream
    * 
    * @param buffer Buffer to read into
    * @param size Size to read
    * @return size_t Bytes read
    */
  size_t read(void* buffer, size_t size) { 
      return vtable->read(this, buffer, size); 
  }
  
  /**
    * @brief Write data to the stream
    * 
    * @param buffer Buffer to write from
    * @param size Size to write
    * @return size_t Bytes written
    */
  size_t write(const void* buffer, size_t size) { 
      return vtable->write(this, buffer, size); 
  }
  
  /**
    * @brief Read data of type T from the stream
    * 
    * @tparam T Type to read
    * @param value Reference to store the value
    * @return true if read successfully
    */
  template<typename T>
  bool readType(T* value) {
      return value ? (read(value, sizeof(T)) == sizeof(T)) : false;
  }
  
  /**
    * @brief Write data of type T to the stream
    * 
    * @tparam T Type to write
    * @param value Value to write
    * @return true if written successfully
    */
  template<typename T>
  bool writeType(const T& value) {
      return write(&value, sizeof(T)) == sizeof(T);
  }
  
  /**
    * @brief Read a line of text from the stream
    * 
    * @param maxSize Maximum size to read
    * @return std::string The line read
    */
  std::string readLine(size_t maxSize = 1024);
  
  /**
    * @brief Write a string to the stream
    * 
    * @param str String to write
    * @return size_t Bytes written
    */
  size_t writeString(const char* str) {
      if (!str) return 0;
      return write(str, strlen(str));
  }
  
  /**
    * @brief Helper to update position information after read/write
    * 
    * @param buffer Buffer that was read or written
    * @param size Size of data
    * @param isRead Whether this was a read (true) or write (false) operation
    */
  void updatePosition(const char* buffer, size_t size, bool isRead);
};

/**
* @brief File-based stream
*/
struct FileStream : public Stream {
  FILE* fp = nullptr;        // File pointer
  size_t readOffset = 0;     // Current read offset
  size_t writeOffset = 0;    // Current write offset
  const Context* ctx = nullptr;  // Library context
  
  /**
    * @brief Open a file stream
    * 
    * @param filename Filename
    * @param mode Mode ("r", "w", "a", "r+", "w+", "a+")
    * @param context Library context
    * @return FileStream The opened stream
    */
  static FileStream open(
      const char* filename,
      const char* mode,
      const Context* context);
  
  /**
    * @brief Close the file
    */
  static void closeFile(Stream* stream);
  
  /**
    * @brief Check for EOF
    */
  static bool fileEof(const Stream* stream);
  
  /**
    * @brief Read implementation
    */
  static size_t fileRead(Stream* stream, void* buffer, size_t size);
  
  /**
    * @brief Write implementation
    */
  static size_t fileWrite(Stream* stream, const void* buffer, size_t size);
  
  /**
    * @brief Reset read position
    */
  static void fileResetReadPos(Stream* stream);
  
  /**
    * @brief Reset write position
    */
  static void fileResetWritePos(Stream* stream);
  
  // Static VTable for file operations
  static const VTable FILE_VTABLE;
};

/**
* @brief Memory-based stream
*/
struct MemoryStream : public Stream {
  uint8_t* buffer = nullptr;  // Memory buffer (not owned by default)
  size_t size = 0;            // Buffer size
  size_t readOffset = 0;      // Current read offset
  size_t writeOffset = 0;     // Current write offset
  bool ownsBuffer = false;    // Whether the buffer is owned by this stream
  const Context* ctx = nullptr;    // Library context
  
  /**
    * @brief Create a memory stream
    * 
    * @param buffer Buffer (if null, a new buffer is allocated)
    * @param size Size
    * @param streamFlags Flags
    * @param context Library context
    * @return MemoryStream The created stream
    */
  static MemoryStream create(
      void* buffer,
      size_t size,
      uint32_t streamFlags,
      const Context* context);
  
  /**
    * @brief Close the memory stream
    */
  static void closeMemory(Stream* stream);
  
  /**
    * @brief Check for EOF
    */
  static bool memoryEof(const Stream* stream);
  
  /**
    * @brief Read implementation
    */
  static size_t memoryRead(Stream* stream, void* buffer, size_t size);
  
  /**
    * @brief Write implementation
    */
  static size_t memoryWrite(Stream* stream, const void* buffer, size_t size);
  
  /**
    * @brief Reset read position
    */
  static void memoryResetReadPos(Stream* stream);
  
  /**
    * @brief Reset write position
    */
  static void memoryResetWritePos(Stream* stream);
  
  /**
    * @brief Get the buffer
    * 
    * @return void* Buffer
    */
  void* getBuffer() const { return buffer; }
  
  /**
    * @brief Get the buffer size
    * 
    * @return size_t Size
    */
  size_t getSize() const { return size; }
  
  /**
    * @brief Get the current write offset
    * 
    * @return size_t Current write offset
    */
  size_t getWriteOffset() const { return writeOffset; }
  
  /**
    * @brief Get the current read offset
    * 
    * @return size_t Current read offset
    */
  size_t getReadOffset() const { return readOffset; }
  
  // Static VTable for memory operations
  static const VTable MEMORY_VTABLE;
};

} // namespace coil