/**
* @file stream.cpp
* @brief Implementation of COIL stream classes
*/

#include "coil/stream.hpp"
#include "coil/err.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>

namespace coil {

//
// Stream base class implementation
//

Result Stream::seekRelative(SeekOrigin origin, i64 offset) {
  size_t base_pos = 0;
  
  // Get base position based on origin
  switch (origin) {
      case SeekOrigin::Begin:
          base_pos = 0;
          break;
      case SeekOrigin::Current:
          base_pos = tell();
          break;
      case SeekOrigin::End:
          base_pos = size();
          break;
  }
  
  // Calculate new position
  i64 new_pos = static_cast<i64>(base_pos) + offset;
  if (new_pos < 0) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Seek position would be negative: %lld", new_pos);
  }
  
  // Seek to the new position
  return seek(static_cast<size_t>(new_pos));
}

size_t Stream::size() const {
  // Default implementation - derived classes should override this
  return 0;
}

size_t Stream::readString(char* buffer, size_t maxSize) {
  if (!buffer || maxSize == 0) {
      return 0;
  }
  
  // Initialize first byte to ensure null-termination even on failure
  buffer[0] = '\0';
  
  size_t i = 0;
  char c;
  
  // Read until null terminator or buffer full
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

size_t Stream::writeString(const char* str) {
  if (!str) {
      return 0;
  }
  
  size_t len = strlen(str) + 1;  // Include null terminator
  return write(str, len);
}

//
// FileStream implementation
//

FileStream::FileStream(const char* filename, StreamMode mode) 
  : handle(nullptr), is_at_eof(false) {
  const char* mode_str = nullptr;
  
  // Convert StreamMode to FILE mode string
  switch (mode) {
      case StreamMode::Read:
          mode_str = "rb";
          break;
      case StreamMode::Write:
          mode_str = "wb";
          break;
      case StreamMode::ReadWrite:
          mode_str = "r+b";
          break;
      default:
          reportError(ErrorLevel::Error, "Invalid stream mode");
          return;
  }
  
  // Open the file
  FILE* file = fopen(filename, mode_str);
  if (!file) {
      reportError(ErrorLevel::Error, "Failed to open file '%s': %s", 
                 filename, strerror(errno));
      return;
  }
  
  handle = file;
}

FileStream::~FileStream() {
  close();
}

size_t FileStream::read(void* buffer, size_t size) {
  if (!handle) return 0;
  
  // Reset EOF flag
  is_at_eof = false;
  
  size_t result = fread(buffer, 1, size, static_cast<FILE*>(handle));
  
  // Update EOF status
  if (result < size) {
      is_at_eof = feof(static_cast<FILE*>(handle)) != 0;
  }
  
  return result;
}

size_t FileStream::write(const void* buffer, size_t size) {
  if (!handle) return 0;
  
  return fwrite(buffer, 1, size, static_cast<FILE*>(handle));
}

bool FileStream::eof() const {
  if (!handle) return true;
  
  return is_at_eof;
}

size_t FileStream::tell() const {
  if (!handle) return 0;
  
  return static_cast<size_t>(ftell(static_cast<FILE*>(handle)));
}

Result FileStream::seek(size_t position) {
  if (!handle) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Cannot seek in closed file");
  }
  
  // Reset EOF flag
  is_at_eof = false;
  
  if (fseek(static_cast<FILE*>(handle), static_cast<long>(position), SEEK_SET) != 0) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to seek to position %zu: %s", 
                      position, strerror(errno));
  }
  
  return Result::Success;
}

Result FileStream::seekRelative(SeekOrigin origin, i64 offset) {
  if (!handle) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Cannot seek in closed file");
  }
  
  // Reset EOF flag
  is_at_eof = false;
  
  int whence;
  switch (origin) {
      case SeekOrigin::Begin:
          whence = SEEK_SET;
          break;
      case SeekOrigin::Current:
          whence = SEEK_CUR;
          break;
      case SeekOrigin::End:
          whence = SEEK_END;
          break;
      default:
          return makeError(Result::InvalidArg, ErrorLevel::Error, 
                          "Invalid seek origin");
  }
  
  if (fseek(static_cast<FILE*>(handle), static_cast<long>(offset), whence) != 0) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to seek to offset %lld from origin %d: %s", 
                      static_cast<long long>(offset), whence, strerror(errno));
  }
  
  return Result::Success;
}

size_t FileStream::size() const {
  if (!handle) return 0;
  
  // Remember current position
  long current_pos = ftell(static_cast<FILE*>(handle));
  if (current_pos < 0) {
      return 0;
  }
  
  // Seek to end to get size
  if (fseek(static_cast<FILE*>(handle), 0, SEEK_END) != 0) {
      return 0;
  }
  
  // Get position at end
  long end_pos = ftell(static_cast<FILE*>(handle));
  if (end_pos < 0) {
      return 0;
  }
  
  // Restore original position
  if (fseek(static_cast<FILE*>(handle), current_pos, SEEK_SET) != 0) {
      // Failed to restore position
      return 0;
  }
  
  return static_cast<size_t>(end_pos);
}

void FileStream::close() {
  if (handle) {
      fclose(static_cast<FILE*>(handle));
      handle = nullptr;
      is_at_eof = false;
  }
}

bool FileStream::isOpen() const {
  return handle != nullptr;
}

//
// MemoryStream implementation
//

MemoryStream::MemoryStream(void* buffer, size_t size, StreamMode mode) 
  : buffer(static_cast<u8*>(buffer)), 
    capacity(size), 
    position(0),
    data_size(0),
    owns_buffer(false),
    is_open(false),
    mode(mode) {
  
  // If no buffer provided, allocate one
  if (!buffer && size > 0) {
      this->buffer = static_cast<u8*>(malloc(size));
      if (this->buffer) {
          owns_buffer = true;
          is_open = true;
          
          // Initialize the buffer if it's for writing
          if (mode != StreamMode::Read) {
              memset(this->buffer, 0, size);
          }
      } else {
          reportError(ErrorLevel::Error, 
                      "Failed to allocate memory for MemoryStream");
          capacity = 0;
      }
  } else if (buffer) {
      is_open = true;
  }
  
  // For existing buffers, assume they're filled with data
  if (buffer && mode != StreamMode::Write) {
      data_size = size;
  }
}

MemoryStream::~MemoryStream() {
  close();
}

size_t MemoryStream::read(void* dest_buffer, size_t size) {
  // Check if we can read
  if (!buffer || !is_open || mode == StreamMode::Write || position >= data_size) {
      return 0;
  }
  
  // Calculate how much we can read
  size_t available = data_size - position;
  size_t to_read = (size < available) ? size : available;
  
  // Copy the data
  if (to_read > 0) {
      memcpy(dest_buffer, buffer + position, to_read);
      position += to_read;
  }
  
  return to_read;
}

size_t MemoryStream::write(const void* src_buffer, size_t size) {
  // Check if we can write
  if (!buffer || !is_open || mode == StreamMode::Read || position >= capacity) {
      return 0;
  }
  
  // Attempt to resize if owned buffer and would exceed capacity
  if (owns_buffer && position + size > capacity) {
      size_t new_capacity = position + size;
      // Round up to the next power of 2
      new_capacity--;
      new_capacity |= new_capacity >> 1;
      new_capacity |= new_capacity >> 2;
      new_capacity |= new_capacity >> 4;
      new_capacity |= new_capacity >> 8;
      new_capacity |= new_capacity >> 16;
      new_capacity++;
      
      if (resize(new_capacity) != Result::Success) {
          // Fallback: write as much as we can
          size = capacity - position;
      }
  }
  
  // Calculate how much we can write
  size_t available = capacity - position;
  size_t to_write = (size < available) ? size : available;
  
  // Copy the data
  if (to_write > 0) {
      memcpy(buffer + position, src_buffer, to_write);
      position += to_write;
      
      // Update data size if position is beyond current data size
      if (position > data_size) {
          data_size = position;
      }
  }
  
  return to_write;
}

bool MemoryStream::eof() const {
  return !buffer || !is_open || position >= data_size;
}

size_t MemoryStream::tell() const {
  return position;
}

Result MemoryStream::seek(size_t new_position) {
  if (!buffer || !is_open) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Cannot seek in closed memory stream");
  }
  
  // Check if the position is valid
  if (new_position > capacity) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Seek position %zu is beyond capacity %zu", 
                      new_position, capacity);
  }
  
  position = new_position;
  
  // If writing and position is beyond current data size, update data size
  if (mode != StreamMode::Read && position > data_size) {
      data_size = position;
  }
  
  return Result::Success;
}

Result MemoryStream::seekRelative(SeekOrigin origin, i64 offset) {
  if (!buffer || !is_open) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Cannot seek in closed memory stream");
  }
  
  size_t base_pos;
  switch (origin) {
      case SeekOrigin::Begin:
          base_pos = 0;
          break;
      case SeekOrigin::Current:
          base_pos = position;
          break;
      case SeekOrigin::End:
          base_pos = data_size;
          break;
      default:
          return makeError(Result::InvalidArg, ErrorLevel::Error, 
                          "Invalid seek origin");
  }
  
  // Calculate new position
  i64 new_pos = static_cast<i64>(base_pos) + offset;
  if (new_pos < 0) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Seek position would be negative: %lld", new_pos);
  }
  
  // Seek to the new position
  return seek(static_cast<size_t>(new_pos));
}

size_t MemoryStream::size() const {
  return data_size;
}

void MemoryStream::close() {
  if (buffer && owns_buffer) {
      free(buffer);
      owns_buffer = false;
  }
  
  buffer = nullptr;
  capacity = 0;
  position = 0;
  data_size = 0;
  is_open = false;
}

bool MemoryStream::isOpen() const {
  return is_open;
}

Result MemoryStream::resize(size_t new_capacity) {
  if (!owns_buffer) {
      return makeError(Result::NotSupported, ErrorLevel::Error, 
                      "Cannot resize non-owned buffer");
  }
  
  if (new_capacity == 0) {
      return makeError(Result::InvalidArg, ErrorLevel::Error, 
                      "Cannot resize to zero capacity");
  }
  
  // Allocate new buffer
  u8* new_buffer = static_cast<u8*>(realloc(buffer, new_capacity));
  if (!new_buffer) {
      return makeError(Result::OutOfMemory, ErrorLevel::Error, 
                      "Failed to resize memory stream to %zu bytes", new_capacity);
  }
  
  // Update stream state
  buffer = new_buffer;
  
  // Initialize new memory
  if (new_capacity > capacity) {
      memset(buffer + capacity, 0, new_capacity - capacity);
  }
  
  capacity = new_capacity;
  
  // Adjust position and data_size if they exceed new capacity
  if (position > capacity) {
      position = capacity;
  }
  
  if (data_size > capacity) {
      data_size = capacity;
  }
  
  return Result::Success;
}

} // namespace coil