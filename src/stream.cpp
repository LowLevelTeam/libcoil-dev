#include "coil/stream.hpp"
#include <cstring>
#include <cerrno>
#include <algorithm>

namespace coil {

// StreamReader implementation for readLine
std::string StreamReader::readLine(size_t maxSize) {
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

// BaseStream implementation
BaseStream::BaseStream(const std::string& name, uint32_t flags, const Context& ctx)
    : name_(name)
    , flags_(flags)
    , ctx_(ctx) {
    
    position_.fileName = name;
    position_.line = 1;
    position_.column = 1;
    position_.offset = 0;
}

uint32_t BaseStream::getFlags() const {
    return flags_;
}

StreamPosition BaseStream::getPosition() const {
    return position_;
}

void BaseStream::updatePosition(const char* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (buffer[i] == '\n') {
            position_.line++;
            position_.column = 1;
        } else {
            position_.column++;
        }
    }
    
    position_.offset += size;
}

// FileStream implementation
FileStream::FileStream(
    const std::string& filename,
    FILE* fp,
    uint32_t flags,
    const Context& ctx)
    : BaseStream(filename, flags, ctx)
    , fp_(fp) {
}

FileStream* FileStream::create(
    const std::string& filename,
    const std::string& mode,
    const Context& ctx) {
    
    if (filename.empty() || mode.empty()) {
        ctx.errorManager.addError(ErrorCode::Argument, StreamPosition(), 
                               "Invalid filename or mode for file stream");
        return nullptr;
    }
    
    FILE* fp = fopen(filename.c_str(), mode.c_str());
    if (!fp) {
        StreamPosition pos;
        pos.fileName = filename;
        ctx.errorManager.addError(ErrorCode::IO, pos, 
                               "Failed to open file: " + std::string(strerror(errno)));
        return nullptr;
    }
    
    uint32_t flags = 0;
    
    if (mode.find('r') != std::string::npos) {
        flags |= StreamFlags::Read;
    }
    
    if (mode.find('w') != std::string::npos || mode.find('a') != std::string::npos || 
        mode.find('+') != std::string::npos) {
        flags |= StreamFlags::Write;
    }
    
    return new FileStream(filename, fp, flags, ctx);
}

size_t FileStream::readImpl(void* buffer, size_t size) {
    if (!buffer || size == 0 || !fp_ || !(flags_ & StreamFlags::Read)) {
        return 0;
    }
    
    size_t bytesRead = fread(buffer, 1, size, fp_);
    
    if (bytesRead < size && ferror(fp_)) {
        ctx_.errorManager.addError(ErrorCode::IO, position_, 
                                "Error reading from file stream: " + 
                                std::string(strerror(ferror(fp_))));
    }
    
    if (bytesRead > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesRead);
    }
    
    if (feof(fp_)) {
        flags_ |= StreamFlags::Eof;
    }
    
    return bytesRead;
}

size_t FileStream::writeImpl(const void* buffer, size_t size) {
    if (!buffer || size == 0 || !fp_ || !(flags_ & StreamFlags::Write)) {
        return 0;
    }
    
    size_t bytesWritten = fwrite(buffer, 1, size, fp_);
    
    if (bytesWritten < size) {
        ctx_.errorManager.addError(ErrorCode::IO, position_, 
                                "Error writing to file stream: " + 
                                std::string(strerror(ferror(fp_))));
    }
    
    if (bytesWritten > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesWritten);
    }
    
    return bytesWritten;
}

bool FileStream::eof() const {
    if (!fp_) {
        return true;
    }
    
    return (feof(fp_) != 0) || ((flags_ & StreamFlags::Eof) != 0);
}

void FileStream::close() {
    if (fp_) {
        fclose(fp_);
        fp_ = nullptr;
    }
}

FileStream::~FileStream() {
    close();
}

// MemoryStream implementation
MemoryStream::MemoryStream(
    void* buffer,
    size_t size,
    bool ownsBuffer,
    uint32_t flags,
    const Context& ctx)
    : BaseStream("memory", flags, ctx)
    , buffer_(static_cast<uint8_t*>(buffer))
    , size_(size)
    , memory_position_(0)
    , ownsBuffer_(ownsBuffer) {
}

MemoryStream* MemoryStream::create(
    void* buffer,
    size_t size,
    uint32_t flags,
    const Context& ctx) {
    
    bool ownsBuffer = false;
    
    if (!buffer && size > 0) {
        // Allocate our own buffer
        buffer = malloc(size);
        if (!buffer) {
            StreamPosition pos;
            pos.fileName = "memory";
            ctx.errorManager.addError(ErrorCode::Memory, pos, 
                                    "Failed to allocate memory for memory stream");
            return nullptr;
        }
        ownsBuffer = true;
    }
    
    return new MemoryStream(buffer, size, ownsBuffer, flags, ctx);
}

size_t MemoryStream::readImpl(void* buffer, size_t size) {
    if (!buffer || size == 0 || !buffer_ || !(flags_ & StreamFlags::Read)) {
        return 0;
    }
    
    size_t available = size_ - memory_position_;
    size_t bytesToRead = std::min(size, available);
    
    if (bytesToRead == 0) {
        flags_ |= StreamFlags::Eof;
        return 0;
    }
    
    memcpy(buffer, buffer_ + memory_position_, bytesToRead);
    
    if (bytesToRead > 0) {
        updatePosition(reinterpret_cast<const char*>(buffer_ + memory_position_), bytesToRead);
    }
    
    memory_position_ += bytesToRead;
    
    if (memory_position_ >= size_) {
        flags_ |= StreamFlags::Eof;
    }
    
    return bytesToRead;
}

size_t MemoryStream::writeImpl(const void* buffer, size_t size) {
    if (!buffer || size == 0 || !buffer_ || !(flags_ & StreamFlags::Write)) {
        return 0;
    }
    
    size_t available = size_ - memory_position_;
    size_t bytesToWrite = std::min(size, available);
    
    if (bytesToWrite == 0) {
        return 0;
    }
    
    memcpy(buffer_ + memory_position_, buffer, bytesToWrite);
    
    if (bytesToWrite > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesToWrite);
    }
    
    memory_position_ += bytesToWrite;
    
    return bytesToWrite;
}

bool MemoryStream::eof() const {
    if (!buffer_) {
        return true;
    }
    
    return memory_position_ >= size_ || ((flags_ & StreamFlags::Eof) != 0);
}

void MemoryStream::close() {
    if (ownsBuffer_ && buffer_) {
        free(buffer_);
        buffer_ = nullptr;
        size_ = 0;
        memory_position_ = 0;
    }
}

void* MemoryStream::getBuffer() const {
    return buffer_;
}

size_t MemoryStream::getSize() const {
    return size_;
}

MemoryStream::~MemoryStream() {
    close();
}

} // namespace coil