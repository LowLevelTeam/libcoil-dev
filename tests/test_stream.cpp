#include <catch2/catch_all.hpp>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fstream>
#include "coil/stream.hpp"
#include "coil/log.hpp"
#include "coil/err.hpp"

// Helper class for temporary test files
struct TempFile {
    char filename[256];
    
    TempFile(const char* content = nullptr) {
        // Create a temporary filename
        snprintf(filename, sizeof(filename), "coil_test_%p.tmp", (void*)this);
        
        // Write content if provided - use binary mode for exact byte control
        if (content) {
            FILE* fp = fopen(filename, "wb");  // Use binary mode
            if (fp) {
                fwrite(content, 1, strlen(content), fp);  // Exact byte count
                fclose(fp);
            }
        }
    }
    
    ~TempFile() {
        // Remove the file
        std::remove(filename);
    }
    
    const char* getFilename() const {
        return filename;
    }
};

// CaptureBuffer for logging output
struct CaptureBuffer {
    static constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    FILE* fp;
    
    CaptureBuffer() {
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    ~CaptureBuffer() {
        if (fp) fclose(fp);
    }
    
    FILE* getFile() const {
        return fp;
    }
    
    const char* getBuffer() const {
        return buffer;
    }
    
    void clear() {
        if (fp) fclose(fp);
        memset(buffer, 0, BUFFER_SIZE);
        fp = fmemopen(buffer, BUFFER_SIZE, "w+");
    }
    
    bool contains(const char* str) const {
        return strstr(buffer, str) != nullptr;
    }
};

// Create context for testing
coil::Context createStreamTestContext() {
    static CaptureBuffer capture;
    static coil::Logger logger("TEST", capture.getFile(), coil::LogLevel::Info, false);
    static coil::ErrorManager errorMgr(&logger);
    return {&logger, &errorMgr};
}

TEST_CASE("FileStream basic operations", "[stream]") {
    auto ctx = createStreamTestContext();
    
    SECTION("Creating a file stream") {
        // Create a temporary file with precise content length
        const char* content = "Hello, world!\nThis is a test file.";
        TempFile tempFile(content);
        
        // Open the file for reading
        coil::FileStream stream = coil::FileStream::open(
            tempFile.getFilename(), "rb", &ctx);  // Note binary mode
        
        REQUIRE(stream.fp != nullptr);
        REQUIRE(stream.isReadable());
        REQUIRE_FALSE(stream.isWritable());
        REQUIRE_FALSE(stream.eof());
        
        // Read the contents
        char buffer[100] = {0};
        size_t bytesRead = stream.read(buffer, sizeof(buffer) - 1);
        
        // This should match the exact length of the content string
        size_t expectedBytes = strlen(content);
        REQUIRE(bytesRead == expectedBytes);
        REQUIRE(strcmp(buffer, content) == 0);
        
        // Now we should be at EOF
        REQUIRE(stream.eof());
        
        // Close
        stream.close();
    }
    
    SECTION("Reading line by line") {
        // Create a temporary file with multiple lines
        TempFile tempFile("Line 1\nLine 2\r\nLine 3\n");
        
        // Open the file
        coil::FileStream stream = coil::FileStream::open(
            tempFile.getFilename(), "r", &ctx);
        
        REQUIRE(stream.fp != nullptr);
        
        // Read lines
        std::string line1 = stream.readLine();
        std::string line2 = stream.readLine();
        std::string line3 = stream.readLine();
        std::string line4 = stream.readLine(); // to read into eof
        
        REQUIRE(line1 == "Line 1");
        REQUIRE(line2 == "Line 2");
        REQUIRE(line3 == "Line 3");
        REQUIRE(line4.empty());
        
        // Now we should be at EOF
        REQUIRE(stream.eof());
        
        // Close
        stream.close();
    }
    
    SECTION("Writing to a file") {
        // Create a temporary file
        TempFile tempFile;
        
        // Open the file for writing
        coil::FileStream stream = coil::FileStream::open(
            tempFile.getFilename(), "wb", &ctx);  // Binary mode
        
        REQUIRE(stream.fp != nullptr);
        REQUIRE_FALSE(stream.isReadable());
        REQUIRE(stream.isWritable());
        
        // Write some data
        const char* data1 = "Test data 1\n";
        const char* data2 = "Test data 2\n";
        REQUIRE(stream.write(data1, strlen(data1)) == strlen(data1));
        REQUIRE(stream.write(data2, strlen(data2)) == strlen(data2));
        
        // Close the stream
        stream.close();
        
        // Verify the file contents with binary read
        FILE* fp = fopen(tempFile.getFilename(), "rb");
        char buffer[100] = {0};
        size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
        fclose(fp);
        
        REQUIRE(bytesRead == strlen(data1) + strlen(data2));
        REQUIRE(strcmp(buffer, "Test data 1\nTest data 2\n") == 0);
    }
    
    SECTION("Stream position tracking") {
        // Create a temporary file
        TempFile tempFile("Line 1\nSecond line\nThird line");
        
        // Open the file
        coil::FileStream stream = coil::FileStream::open(
            tempFile.getFilename(), "r", &ctx);
        
        REQUIRE(stream.fp != nullptr);
        
        // Initial position
        auto pos = stream.getPosition();
        REQUIRE(strcmp(pos.fileName, tempFile.getFilename()) == 0);
        REQUIRE(pos.line == 1);
        REQUIRE(pos.column == 1);
        REQUIRE(pos.offset == 0);
        
        // Read the first line
        std::string line1 = stream.readLine();
        
        // Check position after reading first line
        pos = stream.getPosition();
        REQUIRE(pos.line == 2);  // Now on line 2
        REQUIRE(pos.column == 1);  // At the beginning of the line
        REQUIRE(pos.offset == 7);  // "Line 1\n" is 7 bytes
        
        // Read another line
        std::string line2 = stream.readLine();
        
        // Check position again
        pos = stream.getPosition();
        REQUIRE(pos.line == 3);  // Now on line 3
        REQUIRE(pos.column == 1);  // At the beginning of the line
        REQUIRE(pos.offset == 19);  // Previous 7 plus "Second line\n" (12 bytes)
        
        // Close
        stream.close();
    }
    
    SECTION("Read/write of basic types") {
        // Create a temporary file
        TempFile tempFile;
        
        // Open for writing
        coil::FileStream writeStream = coil::FileStream::open(
            tempFile.getFilename(), "wb", &ctx);
        
        REQUIRE(writeStream.fp != nullptr);
        
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
        
        REQUIRE(writeStream.writeType(u8));
        REQUIRE(writeStream.writeType(i8));
        REQUIRE(writeStream.writeType(u16));
        REQUIRE(writeStream.writeType(i16));
        REQUIRE(writeStream.writeType(u32));
        REQUIRE(writeStream.writeType(i32));
        REQUIRE(writeStream.writeType(u64));
        REQUIRE(writeStream.writeType(i64));
        REQUIRE(writeStream.writeType(f32));
        REQUIRE(writeStream.writeType(f64));
        
        // Close the write stream
        writeStream.close();
        
        // Open for reading
        coil::FileStream readStream = coil::FileStream::open(
            tempFile.getFilename(), "rb", &ctx);
        
        REQUIRE(readStream.fp != nullptr);
        
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
        
        REQUIRE(readStream.readType(&read_u8));
        REQUIRE(readStream.readType(&read_i8));
        REQUIRE(readStream.readType(&read_u16));
        REQUIRE(readStream.readType(&read_i16));
        REQUIRE(readStream.readType(&read_u32));
        REQUIRE(readStream.readType(&read_i32));
        REQUIRE(readStream.readType(&read_u64));
        REQUIRE(readStream.readType(&read_i64));
        REQUIRE(readStream.readType(&read_f32));
        REQUIRE(readStream.readType(&read_f64));
        
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
        
        // Close
        readStream.close();
    }
}

TEST_CASE("MemoryStream basic operations", "[stream]") {
    auto ctx = createStreamTestContext();
    
    SECTION("Creating a memory stream with existing buffer") {
        // Prepare a buffer with data
        const char* testData = "Memory stream test data";
        size_t dataSize = strlen(testData);
        
        // Allocate and copy
        char* buffer = new char[dataSize];
        memcpy(buffer, testData, dataSize);
        
        // Create memory stream - read-only
        coil::MemoryStream stream = coil::MemoryStream::create(
            buffer, dataSize, coil::StreamFlags::Read, &ctx);
        
        REQUIRE(stream.buffer != nullptr);
        REQUIRE(stream.isReadable());
        REQUIRE_FALSE(stream.isWritable());
        REQUIRE_FALSE(!stream.eof());
        
        // Write the contents
        size_t bytesWritten = stream.write(testData, dataSize);
        REQUIRE(bytesWritten == dataSize);
        REQUIRE(stream.getWriteOffset() == dataSize);
        REQUIRE(stream.getReadOffset() == 0);

        // Read the contents
        char readBuffer[100] = {0};
        size_t bytesRead = stream.read(readBuffer, sizeof(readBuffer) - 1);
        REQUIRE(bytesRead == dataSize);
        REQUIRE(strcmp(readBuffer, testData) == 0);
        
        // Now we should be at EOF
        REQUIRE(stream.eof());
        
        // Close
        stream.close();
        
        // The buffer is owned by us in this case, so we need to free it
        delete[] buffer;
    }
    
    SECTION("Creating a memory stream with internal buffer") {
        // Create memory stream with internal buffer allocation
        coil::MemoryStream stream = coil::MemoryStream::create(
            nullptr, 1024, coil::StreamFlags::Read | coil::StreamFlags::Write, &ctx);
        
        REQUIRE(stream.buffer != nullptr);
        REQUIRE(stream.isReadable());
        REQUIRE(stream.isWritable());
        REQUIRE_FALSE(stream.eof());
        
        // Get buffer info
        REQUIRE(stream.getBuffer() != nullptr);
        REQUIRE(stream.getSize() == 1024);
        
        // Write to the stream
        const char* testData = "Test data for memory stream";
        size_t dataSize = strlen(testData);
        REQUIRE(stream.writeString(testData) == dataSize);
        
        // Reset position and read back
        stream.resetReadPosition();
        
        char readBuffer[100] = {0};
        size_t bytesRead = stream.read(readBuffer, sizeof(readBuffer) - 1);
        
        // Should only read what was written
        REQUIRE(bytesRead == dataSize);
        REQUIRE(strcmp(readBuffer, testData) == 0);
        
        // Close and cleanup
        stream.close();
    }
    
    SECTION("Reading and writing specific data types") {
        // Create memory stream
        coil::MemoryStream stream = coil::MemoryStream::create(
            nullptr, 100, coil::StreamFlags::Read | coil::StreamFlags::Write, &ctx);
        
        REQUIRE(stream.buffer != nullptr);
        
        // Write types
        uint8_t u8 = 123;
        int16_t i16 = -12345;
        uint32_t u32 = 0xDEADBEEF;
        float f32 = 3.14159f;
        
        REQUIRE(stream.writeType(u8));
        REQUIRE(stream.writeType(i16));
        REQUIRE(stream.writeType(u32));
        REQUIRE(stream.writeType(f32));
        
        // Reset position for reading
        stream.resetReadPosition();
        
        // Read back
        uint8_t read_u8;
        int16_t read_i16;
        uint32_t read_u32;
        float read_f32;
        
        REQUIRE(stream.readType(&read_u8));
        REQUIRE(stream.readType(&read_i16));
        REQUIRE(stream.readType(&read_u32));
        REQUIRE(stream.readType(&read_f32));
        
        // Verify
        REQUIRE(read_u8 == u8);
        REQUIRE(read_i16 == i16);
        REQUIRE(read_u32 == u32);
        REQUIRE(read_f32 == Catch::Approx(f32));
        
        // Close
        stream.close();
    }
    
    SECTION("Stream bounds checking") {
        // Create a small memory stream
        coil::MemoryStream stream = coil::MemoryStream::create(
            nullptr, 10, coil::StreamFlags::Read | coil::StreamFlags::Write, &ctx);
        
        REQUIRE(stream.buffer != nullptr);
        
        // Write exactly 10 bytes
        const char* data = "0123456789";
        REQUIRE(stream.writeString(data) == 10);
        
        // Try to write more - should be truncated
        REQUIRE(stream.writeString("overflow") == 0);
        
        // Reset and try to read more than available
        stream.resetReadPosition();
        char buffer[20] = {0};
        REQUIRE(stream.read(buffer, 20) == 10);  // Only 10 bytes available
        REQUIRE(strcmp(buffer, "0123456789") == 0);
        
        // Close
        stream.close();
    }
    
    SECTION("EOF detection") {
        // Create a memory stream with 20 bytes capacity
        coil::MemoryStream stream = coil::MemoryStream::create(
            nullptr, 20, coil::StreamFlags::Read | coil::StreamFlags::Write, &ctx);
            
        // Write 10 bytes
        const char* data = "0123456789";
        REQUIRE(stream.writeString(data) == 10);
        
        // Reset read position
        stream.resetReadPosition();
        
        // Read 10 bytes
        char buffer[11] = {0};
        REQUIRE(stream.read(buffer, 10) == 10);
        
        // Should be at EOF
        REQUIRE(stream.eof());
        
        // Try to read more - should return 0
        REQUIRE(stream.read(buffer, 1) == 0);
        
        // Reset both positions
        stream.resetReadPosition();
        stream.resetWritePosition();
        
        // Should not be at EOF anymore
        REQUIRE_FALSE(stream.eof());
        
        // Close
        stream.close();
    }
}