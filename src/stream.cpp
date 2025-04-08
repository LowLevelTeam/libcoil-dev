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
    
    // Initialize both positions
    readPosition_.fileName = name;
    readPosition_.line = 1;
    readPosition_.column = 1;
    readPosition_.offset = 0;
    
    writePosition_.fileName = name;
    writePosition_.line = 1;
    writePosition_.column = 1;
    writePosition_.offset = 0;
}

uint32_t BaseStream::getFlags() const {
    return flags_;
}

StreamPosition BaseStream::getReadPosition() const {
    return readPosition_;
}

StreamPosition BaseStream::getWritePosition() const {
    return writePosition_;
}

void BaseStream::updatePosition(const char* buffer, size_t size, PositionType type) {
    StreamPosition& position = (type == PositionType::Read) ? readPosition_ : writePosition_;
    
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
FileStream::FileStream(
    const std::string& filename,
    FILE* fp,
    uint32_t flags,
    const Context& ctx)
    : BaseStream(filename, flags, ctx)
    , fp_(fp)
    , readOffset_(0)
    , writeOffset_(0) {
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
    
    // Position the file pointer at the read offset
    if (fseek(fp_, readOffset_, SEEK_SET) != 0) {
        ctx_.errorManager.addError(ErrorCode::IO, readPosition_, 
                                "Error positioning file pointer for reading: " + 
                                std::string(strerror(errno)));
        return 0;
    }
    
    size_t bytesRead = fread(buffer, 1, size, fp_);
    
    if (bytesRead < size && ferror(fp_)) {
        ctx_.errorManager.addError(ErrorCode::IO, readPosition_, 
                                "Error reading from file stream: " + 
                                std::string(strerror(ferror(fp_))));
    }
    
    if (bytesRead > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesRead, PositionType::Read);
        readOffset_ += bytesRead;
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
    
    // Position the file pointer at the write offset
    if (fseek(fp_, writeOffset_, SEEK_SET) != 0) {
        ctx_.errorManager.addError(ErrorCode::IO, writePosition_, 
                                "Error positioning file pointer for writing: " + 
                                std::string(strerror(errno)));
        return 0;
    }
    
    size_t bytesWritten = fwrite(buffer, 1, size, fp_);
    
    if (bytesWritten < size) {
        ctx_.errorManager.addError(ErrorCode::IO, writePosition_, 
                                "Error writing to file stream: " + 
                                std::string(strerror(ferror(fp_))));
    }
    
    if (bytesWritten > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesWritten, PositionType::Write);
        writeOffset_ += bytesWritten;
    }
    
    return bytesWritten;
}

bool FileStream::eof() const {
    if (!fp_) {
        return true;
    }
    
    // Save current position
    long currentPos = ftell(fp_);
    
    // Seek to end of file
    fseek(fp_, 0, SEEK_END);
    long endPos = ftell(fp_);
    
    // Restore position
    fseek(fp_, currentPos, SEEK_SET);
    
    // EOF if read position is at or past the end of the file
    return readOffset_ >= static_cast<size_t>(endPos) || ((flags_ & StreamFlags::Eof) != 0);
}

void FileStream::resetReadPosition() {
    readOffset_ = 0;
    readPosition_.line = 1;
    readPosition_.column = 1;
    readPosition_.offset = 0;
}

void FileStream::resetWritePosition() {
    writeOffset_ = 0;
    writePosition_.line = 1;
    writePosition_.column = 1;
    writePosition_.offset = 0;
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
    , readOffset_(0)
    , writeOffset_(0)
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
    
    size_t available = size_ - readOffset_;
    size_t bytesToRead = std::min(size, available);
    
    if (bytesToRead == 0) {
        flags_ |= StreamFlags::Eof;
        return 0;
    }
    
    memcpy(buffer, buffer_ + readOffset_, bytesToRead);
    
    if (bytesToRead > 0) {
        updatePosition(reinterpret_cast<const char*>(buffer_ + readOffset_), bytesToRead, PositionType::Read);
        readOffset_ += bytesToRead;
    }
    
    if (readOffset_ >= size_) {
        flags_ |= StreamFlags::Eof;
    }
    
    return bytesToRead;
}

size_t MemoryStream::writeImpl(const void* buffer, size_t size) {
    if (!buffer || size == 0 || !buffer_ || !(flags_ & StreamFlags::Write)) {
        return 0;
    }
    
    size_t available = size_ - writeOffset_;
    size_t bytesToWrite = std::min(size, available);
    
    if (bytesToWrite == 0) {
        return 0;
    }
    
    memcpy(buffer_ + writeOffset_, buffer, bytesToWrite);
    
    if (bytesToWrite > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesToWrite, PositionType::Write);
        writeOffset_ += bytesToWrite;
    }
    
    return bytesToWrite;
}

bool MemoryStream::eof() const {
    if (!buffer_) {
        return true;
    }
    
    return readOffset_ >= size_ || ((flags_ & StreamFlags::Eof) != 0);
}

void MemoryStream::resetReadPosition() {
    readOffset_ = 0;
    readPosition_.line = 1;
    readPosition_.column = 1;
    readPosition_.offset = 0;
}

void MemoryStream::resetWritePosition() {
    writeOffset_ = 0;
    writePosition_.line = 1;
    writePosition_.column = 1;
    writePosition_.offset = 0;
}

void MemoryStream::close() {
    if (ownsBuffer_ && buffer_) {
        free(buffer_);
        buffer_ = nullptr;
        size_ = 0;
        readOffset_ = 0;
        writeOffset_ = 0;
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