/**
* @file stream.cpp
* @brief Implementation of COIL stream classes
*/

#include "coil/stream.hpp"
#include "coil/err.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace coil {

//
// FileStream implementation
//

FileStream::FileStream(const char* filename, StreamMode mode) : handle(nullptr) {
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
      reportError(ErrorLevel::Error, "Failed to open file '%s'", filename);
      return;
  }
  
  handle = file;
}

FileStream::~FileStream() {
  close();
}

size_t FileStream::read(void* buffer, size_t size) {
  if (!handle) return 0;
  
  return fread(buffer, 1, size, static_cast<FILE*>(handle));
}

size_t FileStream::write(const void* buffer, size_t size) {
  if (!handle) return 0;
  
  return fwrite(buffer, 1, size, static_cast<FILE*>(handle));
}

bool FileStream::eof() const {
  if (!handle) return true;
  
  return feof(static_cast<FILE*>(handle)) != 0;
}

size_t FileStream::tell() const {
  if (!handle) return 0;
  
  return ftell(static_cast<FILE*>(handle));
}

Result FileStream::seek(size_t position) {
  if (!handle) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Cannot seek in closed file");
  }
  
  if (fseek(static_cast<FILE*>(handle), static_cast<long>(position), SEEK_SET) != 0) {
      return makeError(Result::IoError, ErrorLevel::Error, 
                      "Failed to seek to position %zu", position);
  }
  
  return Result::Success;
}

void FileStream::close() {
  if (handle) {
      fclose(static_cast<FILE*>(handle));
      handle = nullptr;
  }
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
    mode(mode) {
  
  // If no buffer provided, allocate one
  if (!buffer && size > 0) {
      this->buffer = static_cast<u8*>(malloc(size));
      if (this->buffer) {
          owns_buffer = true;
          
          // Initialize the buffer if it's for writing
          if (mode != StreamMode::Read) {
              memset(this->buffer, 0, size);
          }
      } else {
          reportError(ErrorLevel::Error, 
                      "Failed to allocate memory for MemoryStream");
          capacity = 0;
      }
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
  if (!buffer || mode == StreamMode::Write || position >= data_size) {
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
  if (!buffer || mode == StreamMode::Read || position >= capacity) {
      return 0;
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
  return !buffer || position >= data_size;
}

size_t MemoryStream::tell() const {
  return position;
}

Result MemoryStream::seek(size_t new_position) {
  if (!buffer) {
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

void MemoryStream::close() {
  if (buffer && owns_buffer) {
      free(buffer);
      owns_buffer = false;
  }
  
  buffer = nullptr;
  capacity = 0;
  position = 0;
  data_size = 0;
}

} // namespace coil