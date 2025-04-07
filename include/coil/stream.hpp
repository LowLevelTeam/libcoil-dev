#pragma once

#include "coil/log.hpp"
#include "coil/err.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <optional>

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
 * @brief Stream interface
 */
class Stream {
public:
    /**
     * @brief Read from the stream
     * 
     * @param buffer Buffer to read into
     * @param size Size to read
     * @return size_t Bytes read
     */
    virtual size_t read(void* buffer, size_t size) = 0;
    
    /**
     * @brief Write to the stream
     * 
     * @param buffer Buffer to write from
     * @param size Size to write
     * @return size_t Bytes written
     */
    virtual size_t write(const void* buffer, size_t size) = 0;
    
    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    virtual bool eof() = 0;
    
    /**
     * @brief Close the stream
     */
    virtual void close() = 0;
    
    /**
     * @brief Get stream flags
     * 
     * @return uint32_t Flags
     */
    virtual uint32_t getFlags() const = 0;
    
    /**
     * @brief Check if the stream is readable
     * 
     * @return true if readable
     */
    bool isReadable() const { return (getFlags() & StreamFlags::Read) != 0; }
    
    /**
     * @brief Check if the stream is writable
     * 
     * @return true if writable
     */
    bool isWritable() const { return (getFlags() & StreamFlags::Write) != 0; }

    
    /**
     * @brief Get the current position information
     * 
     * @return StreamPosition 
     */
    virtual StreamPosition getPosition() const = 0;
    
    /**
     * @brief Read primitive types
     */
    bool readUint8(uint8_t* value);
    bool readInt8(int8_t* value);
    bool readUint16(uint16_t* value);
    bool readInt16(int16_t* value);
    bool readUint32(uint32_t* value);
    bool readInt32(int32_t* value);
    bool readUint64(uint64_t* value);
    bool readInt64(int64_t* value);
    bool readFloat(float* value);
    bool readDouble(double* value);
    
    /**
     * @brief Write primitive types
     */
    bool writeUint8(uint8_t value);
    bool writeInt8(int8_t value);
    bool writeUint16(uint16_t value);
    bool writeInt16(int16_t value);
    bool writeUint32(uint32_t value);
    bool writeInt32(int32_t value);
    bool writeUint64(uint64_t value);
    bool writeInt64(int64_t value);
    bool writeFloat(float value);
    bool writeDouble(double value);
    
    /**
     * @brief Read a string
     * 
     * @param maxSize Maximum size to read
     * @return std::string String that was read
     */
    std::string readString(size_t maxSize = 1024);
    
    /**
     * @brief Write a string
     * 
     * @param str String to write
     * @return size_t Bytes written
     */
    size_t writeString(const std::string& str);
    
    /**
     * @brief Read a line
     * 
     * @param maxSize Maximum size to read
     * @return std::string Line that was read
     */
    std::string readLine(size_t maxSize = 1024);
    
    /**
     * @brief Destructor
     */
    virtual ~Stream() = default;
};

/**
 * @brief Base Stream implementation
 */
class BaseStream : public Stream {
public:
    /**
     * @brief Constructor
     * 
     * @param name Stream name
     * @param flags Stream flags
     * @param logger Logger
     * @param errorMgr Error manager
     */
    BaseStream(
        const std::string& name,
        uint32_t flags,
        std::shared_ptr<Logger> logger = nullptr,
        std::shared_ptr<ErrorManager> errorMgr = nullptr);
    
    /**
     * @brief Get stream flags
     * 
     * @return uint32_t Flags
     */
    uint32_t getFlags() const override;
    
    /**
     * @brief Get the current position information
     * 
     * @return StreamPosition 
     */
    StreamPosition getPosition() const override;
    
    /**
     * @brief Destructor
     */
    ~BaseStream() override = default;
    
protected:
    std::string name_;
    uint32_t flags_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<ErrorManager> errorMgr_;
    mutable std::mutex mutex_;
    StreamPosition position_;
    
    void updatePosition(const char* buffer, size_t size);
};

/**
 * @brief File Stream
 */
class FileStream : public BaseStream {
public:
    /**
     * @brief Create a file stream
     * 
     * @param filename Filename
     * @param mode Mode ("r", "w", "a", "r+", "w+", "a+")
     * @param errorMgr Error manager
     * @param logger Logger
     * @return std::shared_ptr<FileStream> 
     */
    static std::shared_ptr<FileStream> open(
        const std::string& filename,
        const std::string& mode,
        std::shared_ptr<ErrorManager> errorMgr = nullptr,
        std::shared_ptr<Logger> logger = nullptr);
    
    /**
     * @brief Read from the stream
     * 
     * @param buffer Buffer to read into
     * @param size Size to read
     * @return size_t Bytes read
     */
    size_t read(void* buffer, size_t size) override;
    
    /**
     * @brief Write to the stream
     * 
     * @param buffer Buffer to write from
     * @param size Size to write
     * @return size_t Bytes written
     */
    size_t write(const void* buffer, size_t size) override;
    
    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    bool eof() override;
    
    /**
     * @brief Close the stream
     */
    void close() override;
    
    /**
     * @brief Destructor
     */
    ~FileStream() override;
    
private:
    FileStream(
        const std::string& filename,
        FILE* fp,
        uint32_t flags,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<ErrorManager> errorMgr);
    
    FILE* fp_ = nullptr;
};

/**
 * @brief Memory Stream
 */
class MemoryStream : public BaseStream {
public:
    /**
     * @brief Create a memory stream
     * 
     * @param buffer Buffer (if null, a new buffer is allocated)
     * @param size Size
     * @param flags Flags
     * @param errorMgr Error manager
     * @param logger Logger
     * @return std::shared_ptr<MemoryStream> 
     */
    static std::shared_ptr<MemoryStream> create(
        void* buffer,
        size_t size,
        uint32_t flags,
        std::shared_ptr<ErrorManager> errorMgr = nullptr,
        std::shared_ptr<Logger> logger = nullptr);
    
    /**
     * @brief Read from the stream
     * 
     * @param buffer Buffer to read into
     * @param size Size to read
     * @return size_t Bytes read
     */
    size_t read(void* buffer, size_t size) override;
    
    /**
     * @brief Write to the stream
     * 
     * @param buffer Buffer to write from
     * @param size Size to write
     * @return size_t Bytes written
     */
    size_t write(const void* buffer, size_t size) override;

    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    bool eof() override;
    
    /**
     * @brief Close the stream
     */
    void close() override;
    
    /**
     * @brief Get the buffer
     * 
     * @return void* Buffer
     */
    void* getBuffer() const;
    
    /**
     * @brief Get the buffer size
     * 
     * @return size_t Size
     */
    size_t getSize() const;
    
    /**
     * @brief Destructor
     */
    ~MemoryStream() override;
    
private:
    MemoryStream(
        void* buffer,
        size_t size,
        bool ownsBuffer,
        uint32_t flags,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<ErrorManager> errorMgr);
    
    uint8_t* buffer_ = nullptr;
    size_t size_ = 0;
    size_t memory_position_ = 0;
    bool ownsBuffer_ = false;
};

} // namespace coil