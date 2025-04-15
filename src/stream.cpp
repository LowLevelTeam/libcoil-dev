#include "coil/stream.hpp"
#include <cstring>
#include <cerrno>
#include <algorithm>

namespace coil {

// Initialize VTable static structures for file and memory streams
const Stream::VTable FileStream::FILE_VTABLE = {
  &FileStream::fileEof,
  &FileStream::closeFile,
  &FileStream::fileRead,
  &FileStream::fileWrite,
  &FileStream::fileResetReadPos,
  &FileStream::fileResetWritePos
};

const Stream::VTable MemoryStream::MEMORY_VTABLE = {
  &MemoryStream::memoryEof,
  &MemoryStream::closeMemory,
  &MemoryStream::memoryRead,
  &MemoryStream::memoryWrite,
  &MemoryStream::memoryResetReadPos,
  &MemoryStream::memoryResetWritePos
};

// Stream implementation for readLine and position updating
std::string Stream::readLine(size_t maxSize) {
  std::string line;
  line.reserve(128); // Initial capacity
  
  char ch;
  size_t count = 0;
  
  while (count < maxSize) {
      if (read(&ch, 1) != 1) {
          break;
      }
      
      count++;
      
      if (ch == '\n') {
          break;
      } else if (ch != '\r') { // Skip CR
          line.push_back(ch);
      }
  }
  
  return line;
}

void Stream::updatePosition(const char* buffer, size_t size, bool isRead) {
  StreamPosition& position = isRead ? readPosition : writePosition;
  
  for (size_t i = 0; i < size; i++) {
      if (buffer[i] == '\n') {
          position.line++;
          position.column = 1;
      } else {
          position.column++;
      }
  }
  
  position.offset += size;
}

// FileStream implementation
FileStream FileStream::open(
  const char* filename,
  const char* mode,
  const Context* context) {
  
  FileStream stream;
  
  if (!filename || !mode || !context) {
      if (context && context->errorManager) {
          StreamPosition pos;
          context->errorManager->addError(ErrorCode::Argument, pos, 
                                    "Invalid arguments for file stream");
      }
      return stream;
  }
  
  FILE* fp = fopen(filename, mode);
  if (!fp) {
      StreamPosition pos;
      strncpy(pos.fileName, filename, sizeof(pos.fileName) - 1);
      pos.fileName[sizeof(pos.fileName) - 1] = '\0';
      
      if (context && context->errorManager) {
          context->errorManager->addError(ErrorCode::IO, pos, 
                                    strerror(errno));
      }
      return stream;
  }
  
  uint32_t flags = 0;
  
  if (strchr(mode, 'r') != nullptr) {
      flags |= StreamFlags::Read;
  }
  
  if (strchr(mode, 'w') != nullptr || strchr(mode, 'a') != nullptr || 
      strchr(mode, '+') != nullptr) {
      flags |= StreamFlags::Write;
  }
  
  // Initialize the stream
  stream.fp = fp;
  stream.flags = flags;
  stream.readOffset = 0;
  stream.writeOffset = 0;
  stream.ctx = context;
  stream.vtable = &FILE_VTABLE;
  
  // Set up file position tracking
  strncpy(stream.readPosition.fileName, filename, sizeof(stream.readPosition.fileName) - 1);
  stream.readPosition.fileName[sizeof(stream.readPosition.fileName) - 1] = '\0';
  stream.readPosition.line = 1;
  stream.readPosition.column = 1;
  stream.readPosition.offset = 0;
  
  strncpy(stream.writePosition.fileName, filename, sizeof(stream.writePosition.fileName) - 1);
  stream.writePosition.fileName[sizeof(stream.writePosition.fileName) - 1] = '\0';
  stream.writePosition.line = 1;
  stream.writePosition.column = 1;
  stream.writePosition.offset = 0;
  
  return stream;
}

void FileStream::closeFile(Stream* stream) {
  if (!stream) return;
  
  FileStream* fs = static_cast<FileStream*>(stream);
  if (fs->fp) {
      fclose(fs->fp);
      fs->fp = nullptr;
  }
}

bool FileStream::fileEof(const Stream* stream) {
  if (!stream) return true;
  
  const FileStream* fs = static_cast<const FileStream*>(stream);
  if (!fs->fp) {
      return true;
  }
  
  // Save current position
  long currentPos = ftell(fs->fp);
  
  // Seek to end of file
  fseek(fs->fp, 0, SEEK_END);
  long endPos = ftell(fs->fp);
  
  // Restore position
  fseek(fs->fp, currentPos, SEEK_SET);
  
  // EOF if read position is at or past the end of the file
  return fs->readOffset >= static_cast<size_t>(endPos) || ((fs->flags & StreamFlags::Eof) != 0);
}

size_t FileStream::fileRead(Stream* stream, void* buffer, size_t size) {
  if (!stream || !buffer || size == 0) return 0;
  
  FileStream* fs = static_cast<FileStream*>(stream);
  if (!fs->fp || !(fs->flags & StreamFlags::Read)) {
      return 0;
  }
  
  // Position the file pointer at the read offset
  if (fseek(fs->fp, fs->readOffset, SEEK_SET) != 0) {
      if (fs->ctx && fs->ctx->errorManager) {
          fs->ctx->errorManager->addError(ErrorCode::IO, fs->readPosition, 
                                      strerror(errno));
      }
      return 0;
  }
  
  size_t bytesRead = fread(buffer, 1, size, fs->fp);
  
  if (bytesRead < size && ferror(fs->fp)) {
      if (fs->ctx && fs->ctx->errorManager) {
          fs->ctx->errorManager->addError(ErrorCode::IO, fs->readPosition, 
                                      strerror(errno));
      }
  }
  
  if (bytesRead > 0) {
      fs->updatePosition(static_cast<const char*>(buffer), bytesRead, true);
      fs->readOffset += bytesRead;
  }
  
  if (feof(fs->fp)) {
      fs->flags |= StreamFlags::Eof;
  }
  
  return bytesRead;
}

size_t FileStream::fileWrite(Stream* stream, const void* buffer, size_t size) {
  if (!stream || !buffer || size == 0) return 0;
  
  FileStream* fs = static_cast<FileStream*>(stream);
  if (!fs->fp || !(fs->flags & StreamFlags::Write)) {
      return 0;
  }
  
  // Position the file pointer at the write offset
  if (fseek(fs->fp, fs->writeOffset, SEEK_SET) != 0) {
      if (fs->ctx && fs->ctx->errorManager) {
          fs->ctx->errorManager->addError(ErrorCode::IO, fs->writePosition, 
                                      strerror(errno));
      }
      return 0;
  }
  
  size_t bytesWritten = fwrite(buffer, 1, size, fs->fp);
  
  if (bytesWritten < size) {
      if (fs->ctx && fs->ctx->errorManager) {
          fs->ctx->errorManager->addError(ErrorCode::IO, fs->writePosition, 
                                      strerror(errno));
      }
  }
  
  if (bytesWritten > 0) {
      fs->updatePosition(static_cast<const char*>(buffer), bytesWritten, false);
      fs->writeOffset += bytesWritten;
  }
  
  return bytesWritten;
}

void FileStream::fileResetReadPos(Stream* stream) {
  if (!stream) return;
  
  FileStream* fs = static_cast<FileStream*>(stream);
  fs->readOffset = 0;
  fs->readPosition.line = 1;
  fs->readPosition.column = 1;
  fs->readPosition.offset = 0;
}

void FileStream::fileResetWritePos(Stream* stream) {
  if (!stream) return;
  
  FileStream* fs = static_cast<FileStream*>(stream);
  fs->writeOffset = 0;
  fs->writePosition.line = 1;
  fs->writePosition.column = 1;
  fs->writePosition.offset = 0;
}

// MemoryStream implementation
MemoryStream MemoryStream::create(
  void* buffer,
  size_t size,
  uint32_t streamFlags,
  const Context* context) {
  
  MemoryStream stream;
  stream.vtable = &MEMORY_VTABLE;
  stream.flags = streamFlags;
  stream.size = size;
  stream.ctx = context;
  stream.readOffset = 0;
  stream.writeOffset = 0;
  
  // Set up position tracking
  strcpy(stream.readPosition.fileName, "memory");
  stream.readPosition.line = 1;
  stream.readPosition.column = 1;
  stream.readPosition.offset = 0;
  
  strcpy(stream.writePosition.fileName, "memory");
  stream.writePosition.line = 1;
  stream.writePosition.column = 1;
  stream.writePosition.offset = 0;
  
  // Handle buffer creation if needed
  if (!buffer && size > 0) {
      // Allocate our own buffer
      buffer = malloc(size);
      if (!buffer) {
          StreamPosition pos;
          strcpy(pos.fileName, "memory");
          
          if (context && context->errorManager) {
              context->errorManager->addError(ErrorCode::Memory, pos, 
                                        "Failed to allocate memory for memory stream");
          }
          return stream;
      }
      stream.ownsBuffer = true;
  } else {
      stream.ownsBuffer = false;
  }
  
  stream.buffer = static_cast<uint8_t*>(buffer);
  return stream;
}

void MemoryStream::closeMemory(Stream* stream) {
  if (!stream) return;
  
  MemoryStream* ms = static_cast<MemoryStream*>(stream);
  if (ms->ownsBuffer && ms->buffer) {
      free(ms->buffer);
      ms->buffer = nullptr;
      ms->size = 0;
      ms->readOffset = 0;
      ms->writeOffset = 0;
  }
}

bool MemoryStream::memoryEof(const Stream* stream) {
  if (!stream) return true;
  
  const MemoryStream* ms = static_cast<const MemoryStream*>(stream);
  if (!ms->buffer) {
      return true;
  }
  
  return ms->readOffset >= ms->size || ((ms->flags & StreamFlags::Eof) != 0);
}

size_t MemoryStream::memoryRead(Stream* stream, void* buffer, size_t size) {
  if (!stream || !buffer || size == 0) return 0;
  
  MemoryStream* ms = static_cast<MemoryStream*>(stream);
  if (!ms->buffer || !(ms->flags & StreamFlags::Read)) {
      return 0;
  }
  
  size_t available = ms->size - ms->readOffset;
  size_t bytesToRead = std::min(size, available);
  
  if (bytesToRead == 0) {
      ms->flags |= StreamFlags::Eof;
      return 0;
  }
  
  memcpy(buffer, ms->buffer + ms->readOffset, bytesToRead);
  
  if (bytesToRead > 0) {
      ms->updatePosition(reinterpret_cast<const char*>(ms->buffer + ms->readOffset), bytesToRead, true);
      ms->readOffset += bytesToRead;
  }
  
  if (ms->readOffset >= ms->size) {
      ms->flags |= StreamFlags::Eof;
  }
  
  return bytesToRead;
}

size_t MemoryStream::memoryWrite(Stream* stream, const void* buffer, size_t size) {
  if (!stream || !buffer || size == 0) return 0;
  
  MemoryStream* ms = static_cast<MemoryStream*>(stream);
  if (!ms->buffer || !(ms->flags & StreamFlags::Write)) {
      return 0;
  }
  
  size_t available = ms->size - ms->writeOffset;
  size_t bytesToWrite = std::min(size, available);
  
  if (bytesToWrite == 0) {
      return 0;
  }
  
  memcpy(ms->buffer + ms->writeOffset, buffer, bytesToWrite);
  
  if (bytesToWrite > 0) {
      ms->updatePosition(static_cast<const char*>(buffer), bytesToWrite, false);
      ms->writeOffset += bytesToWrite;
  }
  
  return bytesToWrite;
}

void MemoryStream::memoryResetReadPos(Stream* stream) {
  if (!stream) return;
  
  MemoryStream* ms = static_cast<MemoryStream*>(stream);
  ms->readOffset = 0;
  ms->readPosition.line = 1;
  ms->readPosition.column = 1;
  ms->readPosition.offset = 0;
}

void MemoryStream::memoryResetWritePos(Stream* stream) {
  if (!stream) return;
  
  MemoryStream* ms = static_cast<MemoryStream*>(stream);
  ms->writeOffset = 0;
  ms->writePosition.line = 1;
  ms->writePosition.column = 1;
  ms->writePosition.offset = 0;
}

} // namespace coil