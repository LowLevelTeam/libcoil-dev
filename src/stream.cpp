#include "coil/stream.hpp"
#include <cstring>
#include <cerrno>

namespace coil {

// Stream implementation

bool Stream::readUint8(uint8_t* value) {
    if (!value) return false;
    return read(value, sizeof(uint8_t)) == sizeof(uint8_t);
}

bool Stream::readInt8(int8_t* value) {
    if (!value) return false;
    return read(value, sizeof(int8_t)) == sizeof(int8_t);
}

bool Stream::readUint16(uint16_t* value) {
    if (!value) return false;
    return read(value, sizeof(uint16_t)) == sizeof(uint16_t);
}

bool Stream::readInt16(int16_t* value) {
    if (!value) return false;
    return read(value, sizeof(int16_t)) == sizeof(int16_t);
}

bool Stream::readUint32(uint32_t* value) {
    if (!value) return false;
    return read(value, sizeof(uint32_t)) == sizeof(uint32_t);
}

bool Stream::readInt32(int32_t* value) {
    if (!value) return false;
    return read(value, sizeof(int32_t)) == sizeof(int32_t);
}

bool Stream::readUint64(uint64_t* value) {
    if (!value) return false;
    return read(value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool Stream::readInt64(int64_t* value) {
    if (!value) return false;
    return read(value, sizeof(int64_t)) == sizeof(int64_t);
}

bool Stream::readFloat(float* value) {
    if (!value) return false;
    return read(value, sizeof(float)) == sizeof(float);
}

bool Stream::readDouble(double* value) {
    if (!value) return false;
    return read(value, sizeof(double)) == sizeof(double);
}

bool Stream::writeUint8(uint8_t value) {
    return write(&value, sizeof(uint8_t)) == sizeof(uint8_t);
}

bool Stream::writeInt8(int8_t value) {
    return write(&value, sizeof(int8_t)) == sizeof(int8_t);
}

bool Stream::writeUint16(uint16_t value) {
    return write(&value, sizeof(uint16_t)) == sizeof(uint16_t);
}

bool Stream::writeInt16(int16_t value) {
    return write(&value, sizeof(int16_t)) == sizeof(int16_t);
}

bool Stream::writeUint32(uint32_t value) {
    return write(&value, sizeof(uint32_t)) == sizeof(uint32_t);
}

bool Stream::writeInt32(int32_t value) {
    return write(&value, sizeof(int32_t)) == sizeof(int32_t);
}

bool Stream::writeUint64(uint64_t value) {
    return write(&value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool Stream::writeInt64(int64_t value) {
    return write(&value, sizeof(int64_t)) == sizeof(int64_t);
}

bool Stream::writeFloat(float value) {
    return write(&value, sizeof(float)) == sizeof(float);
}

bool Stream::writeDouble(double value) {
    return write(&value, sizeof(double)) == sizeof(double);
}

std::string Stream::readString(size_t maxSize) {
    std::vector<char> buffer(maxSize + 1, 0);
    size_t bytesRead = read(buffer.data(), maxSize);
    return std::string(buffer.data(), bytesRead);
}

size_t Stream::writeString(const std::string& str) {
    return write(str.c_str(), str.size());
}

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

// BaseStream implementation

BaseStream::BaseStream(
    const std::string& name,
    uint32_t flags,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : name_(name)
    , flags_(flags)
    , logger_(logger ? logger : defaultLogger)
    , errorMgr_(errorMgr ? errorMgr : defaultErrorManager) {
    
    position_.fileName = name;
    position_.line = 1;
    position_.column = 1;
    position_.offset = 0;
}

uint32_t BaseStream::getFlags() const {
    return flags_;
}

StreamPosition BaseStream::getPosition() const {
    std::lock_guard<std::mutex> lock(mutex_);
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
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : BaseStream(filename, flags, logger, errorMgr)
    , fp_(fp) {
}

std::shared_ptr<FileStream> FileStream::open(
    const std::string& filename,
    const std::string& mode,
    std::shared_ptr<ErrorManager> errorMgr,
    std::shared_ptr<Logger> logger) {
    
    if (filename.empty() || mode.empty()) {
        if (errorMgr) {
            StreamPosition pos;
            pos.fileName = "stream";
            errorMgr->addError(ErrorCode::Argument, pos, 
                         "Invalid filename or mode for file stream");
        }
        return nullptr;
    }
    
    FILE* fp = fopen(filename.c_str(), mode.c_str());
    if (!fp) {
        if (errorMgr) {
            StreamPosition pos;
            pos.fileName = filename;
            errorMgr->addError(ErrorCode::IO, pos, 
                         "Failed to open file: " + std::string(strerror(errno)));
        }
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
    
    return std::shared_ptr<FileStream>(
        new FileStream(filename, fp, flags, logger, errorMgr));
}

size_t FileStream::read(void* buffer, size_t size) {
    if (!buffer || size == 0 || !fp_ || !(flags_ & StreamFlags::Read)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t bytesRead = fread(buffer, 1, size, fp_);
    
    if (bytesRead < size && ferror(fp_)) {
        if (errorMgr_) {
            errorMgr_->addError(ErrorCode::IO, position_, 
                              "Error reading from file stream: " + 
                              std::string(strerror(ferror(fp_))));
        }
    }
    
    if (bytesRead > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesRead);
    }
    
    if (feof(fp_)) {
        flags_ |= StreamFlags::Eof;
    }
    
    return bytesRead;
}

size_t FileStream::write(const void* buffer, size_t size) {
    if (!buffer || size == 0 || !fp_ || !(flags_ & StreamFlags::Write)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t bytesWritten = fwrite(buffer, 1, size, fp_);
    
    if (bytesWritten < size) {
        if (errorMgr_) {
            errorMgr_->addError(ErrorCode::IO, position_, 
                              "Error writing to file stream: " + 
                              std::string(strerror(ferror(fp_))));
        }
    }
    
    if (bytesWritten > 0) {
        updatePosition(static_cast<const char*>(buffer), bytesWritten);
    }
    
    return bytesWritten;
}

bool FileStream::eof() {
    if (!fp_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    return (feof(fp_) != 0) || ((flags_ & StreamFlags::Eof) != 0);
}

void FileStream::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
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
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : BaseStream("memory", flags, logger, errorMgr)
    , buffer_(static_cast<uint8_t*>(buffer))
    , size_(size)
    , memory_position_(0)
    , ownsBuffer_(ownsBuffer) {
}

std::shared_ptr<MemoryStream> MemoryStream::create(
    void* buffer,
    size_t size,
    uint32_t flags,
    std::shared_ptr<ErrorManager> errorMgr,
    std::shared_ptr<Logger> logger) {
    
    bool ownsBuffer = false;
    
    if (!buffer && size > 0) {
        // Allocate our own buffer
        buffer = malloc(size);
        if (!buffer) {
            if (errorMgr) {
                StreamPosition pos;
                pos.fileName = "memory";
                errorMgr->addError(ErrorCode::Memory, pos, 
                               "Failed to allocate memory for memory stream");
            }
            return nullptr;
        }
        ownsBuffer = true;
    }
    
    return std::shared_ptr<MemoryStream>(
        new MemoryStream(buffer, size, ownsBuffer, flags, logger, errorMgr));
}

size_t MemoryStream::read(void* buffer, size_t size) {
    if (!buffer || size == 0 || !buffer_ || !(flags_ & StreamFlags::Read)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
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

size_t MemoryStream::write(const void* buffer, size_t size) {
    if (!buffer || size == 0 || !buffer_ || !(flags_ & StreamFlags::Write)) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
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

bool MemoryStream::eof() {
    if (!buffer_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    return memory_position_ >= size_ || ((flags_ & StreamFlags::Eof) != 0);
}

void MemoryStream::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (ownsBuffer_ && buffer_) {
        free(buffer_);
        buffer_ = nullptr;
        size_ = 0;
        memory_position_ = 0;
    }
}

void* MemoryStream::getBuffer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_;
}

size_t MemoryStream::getSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

MemoryStream::~MemoryStream() {
    close();
}

} // namespace coil