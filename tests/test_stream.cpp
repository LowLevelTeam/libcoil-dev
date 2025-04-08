#include <catch2/catch_all.hpp>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <fstream>
#include "coil/stream.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"

// Helper class for temporary test files
class TempFile {
public:
    TempFile(const std::string& content = "") {
        // Create a temporary filename
        filename = "coil_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + ".tmp";
        
        // Write content if provided
        if (!content.empty()) {
            std::ofstream file(filename);
            file << content;
            file.close();
        }
    }
    
    ~TempFile() {
        // Remove the file
        std::remove(filename.c_str());
    }
    
    const std::string& getFilename() const {
        return filename;
    }
    
private:
    std::string filename;
};

// CaptureBuffer for logging output
class CaptureBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 4096;
    
    CaptureBuffer() {
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    ~CaptureBuffer() {
        if (fp) fclose(fp);
    }
    
    FILE* getFile() const { return fp; }
    
    const char* getBuffer() const { return buffer; }
    
    void clear() {
        fclose(fp);
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    bool contains(const std::string& str) const {
        return strstr(buffer, str.c_str()) != nullptr;
    }
    
private:
    char buffer[BUFFER_SIZE];
    FILE* fp;
};

// Create context for testing
coil::Context createTestContext() {
    static CaptureBuffer capture;
    static coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
    static coil::ErrorManager errorMgr(logger);
    return {logger, errorMgr};
}

TEST_CASE("FileStream basic operations", "[stream]") {
    auto ctx = createTestContext();
    
    SECTION("Creating a file stream") {
        // Create a temporary file
        TempFile tempFile("Hello, world!\nThis is a test file.");
        
        // Open the file for reading
        coil::FileStream* stream = coil::FileStream::create(
            tempFile.getFilename(), "r", ctx);
        
        REQUIRE(stream != nullptr);
        REQUIRE(stream->isReadable());
        REQUIRE_FALSE(stream->isWritable());
        REQUIRE_FALSE(stream->eof());
        
        // Get a reader
        coil::StreamReader reader = stream->reader();
        
        // Read the contents
        std::string content = reader.readString(100);
        REQUIRE(content == "Hello, world!\nThis is a test file.");
        
        // Now we should be at EOF
        REQUIRE(stream->eof());
        
        // Close and cleanup
        stream->close();
        delete stream;
    }
    
    SECTION("Reading line by line") {
        // Create a temporary file with multiple lines
        TempFile tempFile("Line 1\nLine 2\r\nLine 3\n");
        
        // Open the file
        coil::FileStream* stream = coil::FileStream::create(
            tempFile.getFilename(), "r", ctx);
        
        REQUIRE(stream != nullptr);
        
        // Get a reader
        coil::StreamReader reader = stream->reader();
        
        // Read lines
        std::string line1 = reader.readLine();
        std::string line2 = reader.readLine();
        std::string line3 = reader.readLine();
        
        REQUIRE(line1 == "Line 1");
        REQUIRE(line2 == "Line 2");
        REQUIRE(line3 == "Line 3");
        
        // Now we should be at EOF
        REQUIRE(stream->eof());
        
        // Close and cleanup
        stream->close();
        delete stream;
    }
    
    SECTION("Writing to a file") {
        // Create a temporary file
        TempFile tempFile;
        
        // Open the file for writing
        coil::FileStream* stream = coil::FileStream::create(
            tempFile.getFilename(), "w", ctx);
        
        REQUIRE(stream != nullptr);
        REQUIRE_FALSE(stream->isReadable());
        REQUIRE(stream->isWritable());
        
        // Get a writer
        coil::StreamWriter writer = stream->writer();
        
        // Write some data
        REQUIRE(writer.writeString("Test data 1\n") == 12);
        REQUIRE(writer.writeString("Test data 2\n") == 12);
        
        // Close the stream
        stream->close();
        delete stream;
        
        // Verify the file contents
        std::ifstream file(tempFile.getFilename());
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        REQUIRE(content == "Test data 1\nTest data 2\n");
    }
    
    SECTION("Stream position tracking") {
        // Create a temporary file
        TempFile tempFile("Line 1\nSecond line\nThird line");
        
        // Open the file
        coil::FileStream* stream = coil::FileStream::create(
            tempFile.getFilename(), "r", ctx);
        
        REQUIRE(stream != nullptr);
        
        // Initial position
        auto pos = stream->getPosition();
        REQUIRE(pos.fileName == tempFile.getFilename());
        REQUIRE(pos.line == 1);
        REQUIRE(pos.column == 1);
        REQUIRE(pos.offset == 0);
        
        // Read the first line
        coil::StreamReader reader = stream->reader();
        std::string line1 = reader.readLine();
        
        // Check position after reading first line
        pos = stream->getPosition();
        REQUIRE(pos.line == 2);  // Now on line 2
        REQUIRE(pos.column == 1);  // At the beginning of the line
        REQUIRE(pos.offset == 7);  // "Line 1\n" is 7 bytes
        
        // Read another line
        std::string line2 = reader.readLine();
        
        // Check position again
        pos = stream->getPosition();
        REQUIRE(pos.line == 3);  // Now on line 3
        REQUIRE(pos.column == 1);  // At the beginning of the line
        REQUIRE(pos.offset == 19);  // Previous 7 plus "Second line\n" (12 bytes)
        
        // Close and cleanup
        stream->close();
        delete stream;
    }
    
    SECTION("Read/write of basic types") {
        // Create a temporary file
        TempFile tempFile;
        
        // Open for writing
        coil::FileStream* writeStream = coil::FileStream::create(
            tempFile.getFilename(), "wb", ctx);
        
        REQUIRE(writeStream != nullptr);
        
        coil::StreamWriter writer = writeStream->writer();
        
        // Write various types
        uint8_t u8 = 42;
        int8_t i8 = -42;
        uint16_t u16 = 12345;
        int16_t i16 = -12345;
        uint32_t u32 = 1234567890;
        int32_t i32 = -1234567890;
        uint64_t u64 = 1234567890123456789ULL;
        int64_t i64 = -1234567890123456789LL;
        float f32 = 3.14159f;
        double f64 = 2.71828182845904;
        
        REQUIRE(writer.writeUint8(u8));
        REQUIRE(writer.writeInt8(i8));
        REQUIRE(writer.writeUint16(u16));
        REQUIRE(writer.writeInt16(i16));
        REQUIRE(writer.writeUint32(u32));
        REQUIRE(writer.writeInt32(i32));
        REQUIRE(writer.writeUint64(u64));
        REQUIRE(writer.writeInt64(i64));
        REQUIRE(writer.writeFloat(f32));
        REQUIRE(writer.writeDouble(f64));
        
        // Close the write stream
        writeStream->close();
        delete writeStream;
        
        // Open for reading
        coil::FileStream* readStream = coil::FileStream::create(
            tempFile.getFilename(), "rb", ctx);
        
        REQUIRE(readStream != nullptr);
        
        coil::StreamReader reader = readStream->reader();
        
        // Read back the values
        uint8_t read_u8;
        int8_t read_i8;
        uint16_t read_u16;
        int16_t read_i16;
        uint32_t read_u32;
        int32_t read_i32;
        uint64_t read_u64;
        int64_t read_i64;
        float read_f32;
        double read_f64;
        
        REQUIRE(reader.readUint8(&read_u8));
        REQUIRE(reader.readInt8(&read_i8));
        REQUIRE(reader.readUint16(&read_u16));
        REQUIRE(reader.readInt16(&read_i16));
        REQUIRE(reader.readUint32(&read_u32));
        REQUIRE(reader.readInt32(&read_i32));
        REQUIRE(reader.readUint64(&read_u64));
        REQUIRE(reader.readInt64(&read_i64));
        REQUIRE(reader.readFloat(&read_f32));
        REQUIRE(reader.readDouble(&read_f64));
        
        // Verify the values
        REQUIRE(read_u8 == u8);
        REQUIRE(read_i8 == i8);
        REQUIRE(read_u16 == u16);
        REQUIRE(read_i16 == i16);
        REQUIRE(read_u32 == u32);
        REQUIRE(read_i32 == i32);
        REQUIRE(read_u64 == u64);
        REQUIRE(read_i64 == i64);
        REQUIRE(read_f32 == Catch::Approx(f32));
        REQUIRE(read_f64 == Catch::Approx(f64));
        
        // Close and cleanup
        readStream->close();
        delete readStream;
    }
}

TEST_CASE("MemoryStream basic operations", "[stream]") {
    auto ctx = createTestContext();
    
    SECTION("Creating a memory stream with existing buffer") {
        // Prepare a buffer with data
        const char* testData = "Memory stream test data";
        size_t dataSize = strlen(testData);
        
        // Allocate and copy
        char* buffer = new char[dataSize];
        memcpy(buffer, testData, dataSize);
        
        // Create memory stream - read-only
        coil::MemoryStream* stream = coil::MemoryStream::create(
            buffer, dataSize, coil::StreamFlags::Read, ctx);
        
        REQUIRE(stream != nullptr);
        REQUIRE(stream->isReadable());
        REQUIRE_FALSE(stream->isWritable());
        REQUIRE_FALSE(stream->eof());
        
        // Get a reader
        coil::StreamReader reader = stream->reader();
        
        // Read the contents
        std::string content = reader.readString(100);
        REQUIRE(content == testData);
        
        // Now we should be at EOF
        REQUIRE(stream->eof());
        
        // Close and cleanup
        stream->close();
        delete stream;
        
        // The buffer is owned by us in this case, so we need to free it
        delete[] buffer;
    }
    
    SECTION("Creating a memory stream with internal buffer") {
        // Create memory stream with internal buffer allocation
        coil::MemoryStream* stream = coil::MemoryStream::create(
            nullptr, 1024, coil::StreamFlags::Read | coil::StreamFlags::Write, ctx);
        
        REQUIRE(stream != nullptr);
        REQUIRE(stream->isReadable());
        REQUIRE(stream->isWritable());
        REQUIRE_FALSE(stream->eof());
        
        // Get buffer info
        REQUIRE(stream->getBuffer() != nullptr);
        REQUIRE(stream->getSize() == 1024);
        
        // Write to the stream
        coil::StreamWriter writer = stream->writer();
        const char* testData = "Test data for memory stream";
        size_t dataSize = strlen(testData);
        REQUIRE(writer.writeString(testData) == dataSize);
        
        // Reset position and read back
        // In a real implementation, we would need a method to reset the position,
        // but for this test we'll create a new stream with the same buffer
        void* buffer = stream->getBuffer();
        coil::MemoryStream* readStream = coil::MemoryStream::create(
            buffer, dataSize, coil::StreamFlags::Read, ctx);
        
        coil::StreamReader reader = readStream->reader();
        std::string content = reader.readString(100);
        REQUIRE(content == testData);
        
        // Close and cleanup both streams
        readStream->close();
        delete readStream;
        
        stream->close();
        delete stream;
    }
    
    SECTION("Reading and writing specific data types") {
        // Create memory stream
        coil::MemoryStream* stream = coil::MemoryStream::create(
            nullptr, 100, coil::StreamFlags::Read | coil::StreamFlags::Write, ctx);
        
        REQUIRE(stream != nullptr);
        
        coil::StreamWriter writer = stream->writer();
        
        // Write types
        REQUIRE(writer.writeUint8(123));
        REQUIRE(writer.writeInt16(-12345));
        REQUIRE(writer.writeUint32(0xDEADBEEF));
        REQUIRE(writer.writeFloat(3.14159f));
        
        // Get the buffer and create a new stream for reading
        void* buffer = stream->getBuffer();
        size_t size = stream->getSize();
        
        coil::MemoryStream* readStream = coil::MemoryStream::create(
            buffer, size, coil::StreamFlags::Read, ctx);
        
        coil::StreamReader reader = readStream->reader();
        
        // Read back
        uint8_t u8;
        int16_t i16;
        uint32_t u32;
        float f32;
        
        REQUIRE(reader.readUint8(&u8));
        REQUIRE(reader.readInt16(&i16));
        REQUIRE(reader.readUint32(&u32));
        REQUIRE(reader.readFloat(&f32));
        
        // Verify
        REQUIRE(u8 == 123);
        REQUIRE(i16 == -12345);
        REQUIRE(u32 == 0xDEADBEEF);
        REQUIRE(f32 == Catch::Approx(3.14159f));
        
        // Cleanup
        readStream->close();
        delete readStream;
        
        stream->close();
        delete stream;
    }
    
    SECTION("Stream bounds checking") {
        // Create a small memory stream
        coil::MemoryStream* stream = coil::MemoryStream::create(
            nullptr, 10, coil::StreamFlags::Read | coil::StreamFlags::Write, ctx);
        
        REQUIRE(stream != nullptr);
        
        coil::StreamWriter writer = stream->writer();
        
        // Write exactly 10 bytes
        const char* data = "0123456789";
        REQUIRE(writer.writeString(data) == 10);
        
        // Try to write more - should be truncated
        REQUIRE(writer.writeString("overflow") == 0);
        
        // Create a reader and try to read more than available
        coil::StreamReader reader = stream->reader();
        char buffer[20] = {0};
        REQUIRE(reader.read(buffer, 20) == 10);  // Only 10 bytes available
        REQUIRE(std::string(buffer) == "0123456789");
        
        // Cleanup
        stream->close();
        delete stream;
    }
}