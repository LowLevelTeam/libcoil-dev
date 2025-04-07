#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/stream.hpp"
#include "coil/coil.hpp"
#include <string>
#include <memory>
#include <vector>
#include <cstdio>
#include <filesystem>

using namespace coil;
using namespace testing;

class StreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize COIL
        ASSERT_TRUE(initialize());
        
        // Create temporary directory for test files if it doesn't exist
        if (!std::filesystem::exists("test_tmp")) {
            std::filesystem::create_directory("test_tmp");
        }
    }
    
    void TearDown() override {
        cleanup();
    }
    
    // Helper to create a temporary file path
    std::string getTempFilePath(const std::string& name) {
        return "test_tmp/" + name;
    }
};

TEST_F(StreamTest, MemoryStreamReadWrite) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write data
    const char data[] = "Hello, World!";
    size_t written = stream->write(data, sizeof(data));
    EXPECT_EQ(written, sizeof(data));
    
    // Reset position
    stream->seek(0, SeekOrigin::Begin);
    
    // Read data
    char buffer[256] = {0};
    size_t read = stream->read(buffer, sizeof(buffer));
    EXPECT_EQ(read, sizeof(data));
    EXPECT_STREQ(buffer, data);
}

TEST_F(StreamTest, MemoryStreamExistingBuffer) {
    // Create a buffer
    char buffer[1024] = "Hello, World!";
    
    // Create a memory stream with the existing buffer
    auto stream = MemoryStream::create(buffer, sizeof(buffer), StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Read data
    char readBuffer[256] = {0};
    size_t read = stream->read(readBuffer, sizeof(readBuffer));
    EXPECT_GT(read, 0);
    EXPECT_STREQ(readBuffer, buffer);
    
    // Write data
    const char newData[] = "New data";
    stream->seek(0, SeekOrigin::Begin);
    size_t written = stream->write(newData, sizeof(newData));
    EXPECT_EQ(written, sizeof(newData));
    
    // Verify original buffer was modified
    EXPECT_STREQ(buffer, newData);
}

TEST_F(StreamTest, MemoryStreamSeek) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write data
    const char data[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    stream->write(data, sizeof(data));
    
    // Seek to beginning
    int64_t pos = stream->seek(0, SeekOrigin::Begin);
    EXPECT_EQ(pos, 0);
    
    // Read a few bytes
    char buffer[4] = {0};
    stream->read(buffer, 3);
    EXPECT_STREQ(buffer, "ABC");
    
    // Seek relative
    pos = stream->seek(2, SeekOrigin::Current);
    EXPECT_EQ(pos, 5);
    
    // Read from new position
    stream->read(buffer, 3);
    EXPECT_STREQ(buffer, "FGH");
    
    // Seek from end
    pos = stream->seek(-3, SeekOrigin::End);
    EXPECT_EQ(pos, sizeof(data) - 3);
    
    // Read from new position
    memset(buffer, 0, sizeof(buffer));
    stream->read(buffer, 2);
    EXPECT_STREQ(buffer, "XY");
}

TEST_F(StreamTest, MemoryStreamEOF) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 10, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write data
    const char data[] = "1234567890";
    stream->write(data, sizeof(data) - 1); // Don't include null terminator
    
    // Reset position
    stream->seek(0, SeekOrigin::Begin);
    
    // Read all data
    char buffer[20] = {0};
    size_t read = stream->read(buffer, sizeof(buffer));
    EXPECT_EQ(read, sizeof(data) - 1);
    
    // Check EOF
    EXPECT_TRUE(stream->eof());
    
    // Trying to read more should return 0
    read = stream->read(buffer, sizeof(buffer));
    EXPECT_EQ(read, 0);
}

TEST_F(StreamTest, FileStreamReadWrite) {
    // File path
    std::string filePath = getTempFilePath("test_file.txt");
    
    // Create a file stream for writing
    auto writeStream = FileStream::open(filePath, "w");
    ASSERT_NE(writeStream, nullptr);
    
    // Write data
    const char data[] = "Hello, File Stream!";
    size_t written = writeStream->write(data, sizeof(data) - 1); // Don't write null terminator
    EXPECT_EQ(written, sizeof(data) - 1);
    
    // Close the write stream
    writeStream->close();
    
    // Open the file for reading
    auto readStream = FileStream::open(filePath, "r");
    ASSERT_NE(readStream, nullptr);
    
    // Read data
    char buffer[256] = {0};
    size_t read = readStream->read(buffer, sizeof(buffer));
    EXPECT_EQ(read, sizeof(data) - 1);
    EXPECT_STREQ(buffer, data);
    
    // Close the read stream
    readStream->close();
    
    // Clean up
    std::filesystem::remove(filePath);
}

TEST_F(StreamTest, FileStreamSeekTell) {
    // File path
    std::string filePath = getTempFilePath("test_seek.txt");
    
    // Create a file stream
    auto stream = FileStream::open(filePath, "w+");
    ASSERT_NE(stream, nullptr);
    
    // Write data
    const char data[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    stream->write(data, sizeof(data) - 1);
    
    // Seek to beginning
    int64_t pos = stream->seek(0, SeekOrigin::Begin);
    EXPECT_EQ(pos, 0);
    EXPECT_EQ(stream->tell(), 0);
    
    // Read a few bytes
    char buffer[4] = {0};
    stream->read(buffer, 3);
    EXPECT_STREQ(buffer, "ABC");
    EXPECT_EQ(stream->tell(), 3);
    
    // Seek relative
    pos = stream->seek(2, SeekOrigin::Current);
    EXPECT_EQ(pos, 5);
    EXPECT_EQ(stream->tell(), 5);
    
    // Read from new position
    memset(buffer, 0, sizeof(buffer));
    stream->read(buffer, 3);
    EXPECT_STREQ(buffer, "FGH");
    
    // Seek from end
    pos = stream->seek(-3, SeekOrigin::End);
    EXPECT_EQ(pos, sizeof(data) - 1 - 3);
    
    // Read from new position
    memset(buffer, 0, sizeof(buffer));
    stream->read(buffer, 2);
    EXPECT_STREQ(buffer, "XY");
    
    // Close the stream
    stream->close();
    
    // Clean up
    std::filesystem::remove(filePath);
}

TEST_F(StreamTest, PrimitiveReadWrite) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write primitives
    uint8_t u8 = 42;
    int8_t i8 = -42;
    uint16_t u16 = 1000;
    int16_t i16 = -1000;
    uint32_t u32 = 1000000;
    int32_t i32 = -1000000;
    uint64_t u64 = 1000000000000ULL;
    int64_t i64 = -1000000000000LL;
    float f = 3.14159f;
    double d = 2.718281828459045;
    
    EXPECT_TRUE(stream->writeUint8(u8));
    EXPECT_TRUE(stream->writeInt8(i8));
    EXPECT_TRUE(stream->writeUint16(u16));
    EXPECT_TRUE(stream->writeInt16(i16));
    EXPECT_TRUE(stream->writeUint32(u32));
    EXPECT_TRUE(stream->writeInt32(i32));
    EXPECT_TRUE(stream->writeUint64(u64));
    EXPECT_TRUE(stream->writeInt64(i64));
    EXPECT_TRUE(stream->writeFloat(f));
    EXPECT_TRUE(stream->writeDouble(d));
    
    // Reset position
    stream->seek(0, SeekOrigin::Begin);
    
    // Read primitives
    uint8_t ru8;
    int8_t ri8;
    uint16_t ru16;
    int16_t ri16;
    uint32_t ru32;
    int32_t ri32;
    uint64_t ru64;
    int64_t ri64;
    float rf;
    double rd;
    
    EXPECT_TRUE(stream->readUint8(&ru8));
    EXPECT_TRUE(stream->readInt8(&ri8));
    EXPECT_TRUE(stream->readUint16(&ru16));
    EXPECT_TRUE(stream->readInt16(&ri16));
    EXPECT_TRUE(stream->readUint32(&ru32));
    EXPECT_TRUE(stream->readInt32(&ri32));
    EXPECT_TRUE(stream->readUint64(&ru64));
    EXPECT_TRUE(stream->readInt64(&ri64));
    EXPECT_TRUE(stream->readFloat(&rf));
    EXPECT_TRUE(stream->readDouble(&rd));
    
    // Verify values
    EXPECT_EQ(ru8, u8);
    EXPECT_EQ(ri8, i8);
    EXPECT_EQ(ru16, u16);
    EXPECT_EQ(ri16, i16);
    EXPECT_EQ(ru32, u32);
    EXPECT_EQ(ri32, i32);
    EXPECT_EQ(ru64, u64);
    EXPECT_EQ(ri64, i64);
    EXPECT_FLOAT_EQ(rf, f);
    EXPECT_DOUBLE_EQ(rd, d);
}

TEST_F(StreamTest, ReadWriteString) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write string
    std::string test = "Hello, String Functions!";
    size_t written = stream->writeString(test);
    EXPECT_EQ(written, test.size());
    
    // Reset position
    stream->seek(0, SeekOrigin::Begin);
    
    // Read string
    std::string read = stream->readString(100);
    EXPECT_EQ(read, test);
}

TEST_F(StreamTest, ReadLine) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Write multi-line string
    std::string test = "Line 1\nLine 2\nLine 3\n";
    stream->writeString(test);
    
    // Reset position
    stream->seek(0, SeekOrigin::Begin);
    
    // Read lines
    std::string line1 = stream->readLine();
    std::string line2 = stream->readLine();
    std::string line3 = stream->readLine();
    std::string line4 = stream->readLine(); // Should be empty
    
    // Verify lines
    EXPECT_EQ(line1, "Line 1");
    EXPECT_EQ(line2, "Line 2");
    EXPECT_EQ(line3, "Line 3");
    EXPECT_TRUE(line4.empty());
}

TEST_F(StreamTest, StreamPosition) {
    // Create a memory stream
    auto stream = MemoryStream::create(nullptr, 1024, StreamFlags::Read | StreamFlags::Write);
    ASSERT_NE(stream, nullptr);
    
    // Get initial position
    StreamPosition pos = stream->getPosition();
    EXPECT_EQ(pos.line, 1);
    EXPECT_EQ(pos.column, 1);
    EXPECT_EQ(pos.offset, 0);
    
    // Write some data without newlines
    stream->writeString("Hello");
    
    // Get position after write
    pos = stream->getPosition();
    EXPECT_EQ(pos.line, 1);
    EXPECT_EQ(pos.column, 6); // 1 + 5 chars
    EXPECT_EQ(pos.offset, 5);
    
    // Write newline
    stream->writeString("\n");
    
    // Get position after newline
    pos = stream->getPosition();
    EXPECT_EQ(pos.line, 2);
    EXPECT_EQ(pos.column, 1);
    EXPECT_EQ(pos.offset, 6);
    
    // Write more data
    stream->writeString("World");
    
    // Get final position
    pos = stream->getPosition();
    EXPECT_EQ(pos.line, 2);
    EXPECT_EQ(pos.column, 6); // 1 + 5 chars
    EXPECT_EQ(pos.offset, 11);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}