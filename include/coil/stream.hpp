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

// Forward declarations
class StreamReader;
class StreamWriter;

/**
 * @brief Base Stream interface
 */
class Stream {
public:
    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    virtual bool eof() const = 0;
    
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
     * @brief Get the current read position information
     * 
     * @return StreamPosition 
     */
    virtual StreamPosition getReadPosition() const = 0;
    
    /**
     * @brief Get the current write position information
     * 
     * @return StreamPosition 
     */
    virtual StreamPosition getWritePosition() const = 0;
    
    /**
     * @brief Get the current position information (for backward compatibility)
     * 
     * @return StreamPosition Current read position
     */
    virtual StreamPosition getPosition() const { return getReadPosition(); }
    
    /**
     * @brief Reset the read position to the beginning of the stream
     */
    virtual void resetReadPosition() = 0;
    
    /**
     * @brief Reset the write position to the beginning of the stream
     */
    virtual void resetWritePosition() = 0;
    
    /**
     * @brief Check if the stream is readable
     * 
     * @return true if readable
     */
    inline bool isReadable() const { return (getFlags() & StreamFlags::Read) != 0; }
    
    /**
     * @brief Check if the stream is writable
     * 
     * @return true if writable
     */
    inline bool isWritable() const { return (getFlags() & StreamFlags::Write) != 0; }
    
    /**
     * @brief Destructor
     */
    virtual ~Stream() = default;

protected:
    friend class StreamReader;
    friend class StreamWriter;
    
    // These are protected and only accessible via the reader/writer classes
    virtual size_t readImpl(void* buffer, size_t size) = 0;
    virtual size_t writeImpl(const void* buffer, size_t size) = 0;
};

/**
 * @brief Stream reader for type-safe reading operations
 * 
 * This class provides a type-safe interface for reading from streams
 */
class StreamReader {
public:
    /**
     * @brief Construct a reader for a stream
     * 
     * @param stream The stream to read from
     */
    explicit StreamReader(Stream& stream) : stream_(stream) {}
    
    /**
     * @brief Read data from the stream
     * 
     * @param buffer Buffer to read into
     * @param size Size to read
     * @return size_t Bytes read
     */
    inline size_t read(void* buffer, size_t size) {
        return stream_.readImpl(buffer, size);
    }
    
    /**
     * @brief Read a value of type T from the stream
     * 
     * @tparam T Type to read
     * @param value Reference to store the read value
     * @return true if read successfully
     */
    template<typename T>
    inline bool read(T& value) {
        return read(&value, sizeof(T)) == sizeof(T);
    }
    
    /**
     * @brief Read a uint8 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readUint8(uint8_t* value) {
        return value ? (read(value, sizeof(uint8_t)) == sizeof(uint8_t)) : false;
    }
    
    /**
     * @brief Read an int8 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readInt8(int8_t* value) {
        return value ? (read(value, sizeof(int8_t)) == sizeof(int8_t)) : false;
    }
    
    /**
     * @brief Read a uint16 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readUint16(uint16_t* value) {
        return value ? (read(value, sizeof(uint16_t)) == sizeof(uint16_t)) : false;
    }
    
    /**
     * @brief Read an int16 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readInt16(int16_t* value) {
        return value ? (read(value, sizeof(int16_t)) == sizeof(int16_t)) : false;
    }
    
    /**
     * @brief Read a uint32 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readUint32(uint32_t* value) {
        return value ? (read(value, sizeof(uint32_t)) == sizeof(uint32_t)) : false;
    }
    
    /**
     * @brief Read an int32 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readInt32(int32_t* value) {
        return value ? (read(value, sizeof(int32_t)) == sizeof(int32_t)) : false;
    }
    
    /**
     * @brief Read a uint64 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readUint64(uint64_t* value) {
        return value ? (read(value, sizeof(uint64_t)) == sizeof(uint64_t)) : false;
    }
    
    /**
     * @brief Read an int64 value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readInt64(int64_t* value) {
        return value ? (read(value, sizeof(int64_t)) == sizeof(int64_t)) : false;
    }
    
    /**
     * @brief Read a float value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readFloat(float* value) {
        return value ? (read(value, sizeof(float)) == sizeof(float)) : false;
    }
    
    /**
     * @brief Read a double value
     * 
     * @param value Pointer to store the value
     * @return bool Success
     */
    inline bool readDouble(double* value) {
        return value ? (read(value, sizeof(double)) == sizeof(double)) : false;
    }
    
    /**
     * @brief Read a string
     * 
     * @param maxSize Maximum size to read
     * @return std::string String that was read
     */
    inline std::string readString(size_t maxSize = 1024) {
        char buffer[1024];
        size_t chunkSize = std::min(maxSize, sizeof(buffer));
        
        size_t bytesRead = read(buffer, chunkSize);
        return std::string(buffer, bytesRead);
    }
    
    /**
     * @brief Read a line
     * 
     * @param maxSize Maximum size to read
     * @return std::string Line that was read
     */
    std::string readLine(size_t maxSize = 1024);
    
    /**
     * @brief Reset the read position to the beginning of the stream
     */
    inline void resetPosition() {
        stream_.resetReadPosition();
    }
    
private:
    Stream& stream_;
};

/**
 * @brief Stream writer for type-safe writing operations
 * 
 * This class provides a type-safe interface for writing to streams
 */
class StreamWriter {
public:
    /**
     * @brief Construct a writer for a stream
     * 
     * @param stream The stream to write to
     */
    explicit StreamWriter(Stream& stream) : stream_(stream) {}
    
    /**
     * @brief Write data to the stream
     * 
     * @param buffer Buffer to write from
     * @param size Size to write
     * @return size_t Bytes written
     */
    inline size_t write(const void* buffer, size_t size) {
        return stream_.writeImpl(buffer, size);
    }
    
    /**
     * @brief Write a value of type T to the stream
     * 
     * @tparam T Type to write
     * @param value Value to write
     * @return true if written successfully
     */
    template<typename T>
    inline bool write(const T& value) {
        return write(&value, sizeof(T)) == sizeof(T);
    }
    
    /**
     * @brief Write a uint8 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeUint8(uint8_t value) {
        return write(&value, sizeof(uint8_t)) == sizeof(uint8_t);
    }
    
    /**
     * @brief Write an int8 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeInt8(int8_t value) {
        return write(&value, sizeof(int8_t)) == sizeof(int8_t);
    }
    
    /**
     * @brief Write a uint16 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeUint16(uint16_t value) {
        return write(&value, sizeof(uint16_t)) == sizeof(uint16_t);
    }
    
    /**
     * @brief Write an int16 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeInt16(int16_t value) {
        return write(&value, sizeof(int16_t)) == sizeof(int16_t);
    }
    
    /**
     * @brief Write a uint32 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeUint32(uint32_t value) {
        return write(&value, sizeof(uint32_t)) == sizeof(uint32_t);
    }
    
    /**
     * @brief Write an int32 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeInt32(int32_t value) {
        return write(&value, sizeof(int32_t)) == sizeof(int32_t);
    }
    
    /**
     * @brief Write a uint64 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeUint64(uint64_t value) {
        return write(&value, sizeof(uint64_t)) == sizeof(uint64_t);
    }
    
    /**
     * @brief Write an int64 value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeInt64(int64_t value) {
        return write(&value, sizeof(int64_t)) == sizeof(int64_t);
    }
    
    /**
     * @brief Write a float value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeFloat(float value) {
        return write(&value, sizeof(float)) == sizeof(float);
    }
    
    /**
     * @brief Write a double value
     * 
     * @param value Value to write
     * @return bool Success
     */
    inline bool writeDouble(double value) {
        return write(&value, sizeof(double)) == sizeof(double);
    }
    
    /**
     * @brief Write a string
     * 
     * @param str String to write
     * @return size_t Bytes written
     */
    inline size_t writeString(const std::string& str) {
        return write(str.c_str(), str.size());
    }
    
    /**
     * @brief Reset the write position to the beginning of the stream
     */
    inline void resetPosition() {
        stream_.resetWritePosition();
    }
    
private:
    Stream& stream_;
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
     * @param ctx Library context
     */
    BaseStream(const std::string& name, uint32_t flags, const Context& ctx);
    
    /**
     * @brief Get stream flags
     * 
     * @return uint32_t Flags
     */
    uint32_t getFlags() const override;
    
    /**
     * @brief Get the current read position information
     * 
     * @return StreamPosition 
     */
    StreamPosition getReadPosition() const override;
    
    /**
     * @brief Get the current write position information
     * 
     * @return StreamPosition 
     */
    StreamPosition getWritePosition() const override;
    
    /**
     * @brief Destructor
     */
    ~BaseStream() override = default;
    
protected:
    std::string name_;
    uint32_t flags_;
    const Context& ctx_;
    StreamPosition readPosition_;
    StreamPosition writePosition_;
    
    // Position type enumeration
    enum class PositionType { Read, Write };
    
    // Update the specified position
    void updatePosition(const char* buffer, size_t size, PositionType type);
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
     * @param ctx Library context
     * @return FileStream* Newly created stream (caller takes ownership) or nullptr on error
     */
    static FileStream* create(
        const std::string& filename,
        const std::string& mode,
        const Context& ctx);
    
    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    bool eof() const override;
    
    /**
     * @brief Close the stream
     */
    void close() override;
    
    /**
     * @brief Reset the read position to the beginning of the stream
     */
    void resetReadPosition() override;
    
    /**
     * @brief Reset the write position to the beginning of the stream
     */
    void resetWritePosition() override;
    
    /**
     * @brief Destructor - automatically closes the file if still open
     */
    ~FileStream() override;
    
    /**
     * @brief Get a reader for this stream
     * 
     * @return StreamReader 
     */
    inline StreamReader reader() { return StreamReader(*this); }
    
    /**
     * @brief Get a writer for this stream
     * 
     * @return StreamWriter 
     */
    inline StreamWriter writer() { return StreamWriter(*this); }
    
protected:
    size_t readImpl(void* buffer, size_t size) override;
    size_t writeImpl(const void* buffer, size_t size) override;
    
private:
    FileStream(
        const std::string& filename,
        FILE* fp,
        uint32_t flags,
        const Context& ctx);
    
    FILE* fp_ = nullptr;
    size_t readOffset_ = 0;    // Current read offset
    size_t writeOffset_ = 0;   // Current write offset
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
     * @param ctx Library context
     * @return MemoryStream* Newly created stream (caller takes ownership) or nullptr on error
     */
    static MemoryStream* create(
        void* buffer,
        size_t size,
        uint32_t flags,
        const Context& ctx);
    
    /**
     * @brief Check if the end of stream has been reached
     * 
     * @return true if at end of stream
     */
    bool eof() const override;
    
    /**
     * @brief Close the stream
     */
    void close() override;
    
    /**
     * @brief Reset the read position to the beginning of the stream
     */
    void resetReadPosition() override;
    
    /**
     * @brief Reset the write position to the beginning of the stream
     */
    void resetWritePosition() override;
    
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
     * @brief Get the current write offset
     * 
     * @return size_t Current write offset
     */
    size_t getWriteOffset() const { return writeOffset_; }
    
    /**
     * @brief Get the current read offset
     * 
     * @return size_t Current read offset
     */
    size_t getReadOffset() const { return readOffset_; }
    
    /**
     * @brief Destructor - automatically closes if needed
     */
    ~MemoryStream() override;
    
    /**
     * @brief Get a reader for this stream
     * 
     * @return StreamReader 
     */
    inline StreamReader reader() { return StreamReader(*this); }
    
    /**
     * @brief Get a writer for this stream
     * 
     * @return StreamWriter 
     */
    inline StreamWriter writer() { return StreamWriter(*this); }
    
protected:
    size_t readImpl(void* buffer, size_t size) override;
    size_t writeImpl(const void* buffer, size_t size) override;
    
private:
    MemoryStream(
        void* buffer,
        size_t size,
        bool ownsBuffer,
        uint32_t flags,
        const Context& ctx);
    
    uint8_t* buffer_ = nullptr;
    size_t size_ = 0;
    size_t readOffset_ = 0;     // Current read position
    size_t writeOffset_ = 0;    // Current write position
    bool ownsBuffer_ = false;
};

} // namespace coil