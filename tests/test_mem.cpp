#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/mem.hpp"
#include "coil/coil.hpp"
#include <string>
#include <memory>
#include <vector>

using namespace coil;
using namespace testing;

class MemoryArenaTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize COIL
        ASSERT_TRUE(initialize());
        
        // Create a test arena
        arena = MemoryArena::create("test", 1024 * 1024, true);
        ASSERT_TRUE(arena != nullptr);
    }
    
    void TearDown() override {
        arena.reset();
        cleanup();
    }
    
    MemoryArenaPtr arena;
};

TEST_F(MemoryArenaTest, BasicAllocation) {
    // Allocate some memory
    void* ptr = arena->allocate(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Verify stats
    MemoryStats stats = arena->getStats();
    EXPECT_EQ(stats.allocationCount, 1);
    EXPECT_GE(stats.totalAllocated, 1024);
    EXPECT_GE(stats.currentUsage, 1024);
}

TEST_F(MemoryArenaTest, MultipleAllocations) {
    // Allocate multiple blocks
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; i++) {
        void* ptr = arena->allocate(1024);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Verify stats
    MemoryStats stats = arena->getStats();
    EXPECT_EQ(stats.allocationCount, 10);
    EXPECT_GE(stats.totalAllocated, 10 * 1024);
    EXPECT_GE(stats.currentUsage, 10 * 1024);
}

TEST_F(MemoryArenaTest, AlignedAllocation) {
    // Allocate aligned memory
    void* ptr = arena->allocateAligned(1024, 128);
    ASSERT_NE(ptr, nullptr);
    
    // Verify alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(addr % 128, 0);
}

TEST_F(MemoryArenaTest, CallocZeroesMemory) {
    // Allocate with calloc
    int* numbers = static_cast<int*>(arena->callocate(10, sizeof(int)));
    ASSERT_NE(numbers, nullptr);
    
    // Verify that memory is zeroed
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(numbers[i], 0);
    }
}

TEST_F(MemoryArenaTest, CloneMemory) {
    // Source data
    const char data[] = "Hello, World!";
    size_t size = sizeof(data);
    
    // Clone memory
    char* clone = static_cast<char*>(arena->cloneMemory(data, size));
    ASSERT_NE(clone, nullptr);
    
    // Verify cloned data
    EXPECT_EQ(std::string(clone), std::string(data));
}

TEST_F(MemoryArenaTest, CloneString) {
    // Source string
    const char* str = "Hello, World!";
    
    // Clone string
    char* clone = arena->cloneString(str);
    ASSERT_NE(clone, nullptr);
    
    // Verify cloned string
    EXPECT_STREQ(clone, str);
    
    // Clone std::string
    std::string stdStr = "Hello, C++!";
    char* stdClone = arena->cloneString(stdStr);
    ASSERT_NE(stdClone, nullptr);
    
    // Verify cloned std::string
    EXPECT_STREQ(stdClone, stdStr.c_str());
}

TEST_F(MemoryArenaTest, Reset) {
    // Allocate some memory
    for (int i = 0; i < 10; i++) {
        void* ptr = arena->allocate(1024);
        ASSERT_NE(ptr, nullptr);
    }
    
    // Get stats before reset
    MemoryStats before = arena->getStats();
    
    // Reset arena
    arena->reset();
    
    // Get stats after reset
    MemoryStats after = arena->getStats();
    
    // Verify reset
    EXPECT_EQ(after.currentUsage, 0);
    EXPECT_EQ(after.freeCount, before.freeCount + 1);
    EXPECT_EQ(after.totalFreed, before.totalFreed + before.currentUsage);
    
    // Verify we can allocate again
    void* ptr = arena->allocate(1024);
    ASSERT_NE(ptr, nullptr);
}

TEST_F(MemoryArenaTest, ChildArena) {
    // Create a child arena
    auto child = arena->createChild("child", 1024 * 64);
    ASSERT_NE(child, nullptr);
    
    // Allocate from child arena
    void* ptr = child->allocate(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Verify child stats
    MemoryStats childStats = child->getStats();
    EXPECT_EQ(childStats.allocationCount, 1);
    EXPECT_GE(childStats.totalAllocated, 1024);
    
    // Verify parent is unaffected
    MemoryStats parentStats = arena->getStats();
    EXPECT_EQ(parentStats.allocationCount, 0);
}

TEST_F(MemoryArenaTest, MemoryExhaustion) {
    // Create a small arena
    auto smallArena = MemoryArena::create("small", 1024);
    ASSERT_NE(smallArena, nullptr);
    
    // Allocate until exhaustion
    void* ptr1 = smallArena->allocate(512);
    ASSERT_NE(ptr1, nullptr);
    
    void* ptr2 = smallArena->allocate(512);
    ASSERT_NE(ptr2, nullptr);
    
    // This should fail as we're out of memory
    void* ptr3 = smallArena->allocate(512);
    EXPECT_EQ(ptr3, nullptr);
}

TEST_F(MemoryArenaTest, CreateObject) {
    // Test structure
    struct TestStruct {
        int a;
        float b;
        std::string c;
        
        TestStruct(int a, float b, const std::string& c)
            : a(a), b(b), c(c) {}
    };
    
    // Create an object
    TestStruct* obj = arena->createObject<TestStruct>(42, 3.14f, "test");
    ASSERT_NE(obj, nullptr);
    
    // Verify object state
    EXPECT_EQ(obj->a, 42);
    EXPECT_FLOAT_EQ(obj->b, 3.14f);
    EXPECT_EQ(obj->c, "test");
}

TEST_F(MemoryArenaTest, ThreadSpecificArena) {
    // Create a thread-specific arena
    MemoryArenaPtr threadArena = createThreadArena(1024 * 1024);
    ASSERT_NE(threadArena, nullptr);
    
    // Get the thread arena
    MemoryArenaPtr retrieved = getThreadArena();
    EXPECT_EQ(retrieved, threadArena);
    
    // Allocate from the thread arena
    void* ptr = COIL_THREAD_ALLOC(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Verify stats
    MemoryStats stats = threadArena->getStats();
    EXPECT_EQ(stats.allocationCount, 1);
    EXPECT_GE(stats.totalAllocated, 1024);
}

TEST_F(MemoryArenaTest, GlobalArena) {
    // Verify global arena exists
    ASSERT_NE(globalArena, nullptr);
    
    // Allocate from global arena
    void* ptr = COIL_GLOBAL_ALLOC(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Verify stats
    MemoryStats stats = globalArena->getStats();
    EXPECT_GE(stats.allocationCount, 1);
    EXPECT_GE(stats.totalAllocated, 1024);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}