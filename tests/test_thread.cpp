#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "coil/thread.hpp"
#include "coil/coil.hpp"
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

using namespace coil;
using namespace testing;
using namespace std::chrono_literals;

class ThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize COIL
        ASSERT_TRUE(initialize());
    }
    
    void TearDown() override {
        cleanup();
    }
};

TEST_F(ThreadTest, ThreadTaskBasic) {
    // Create a task that returns a value
    auto task = ThreadTask::create([]() -> void* {
        return reinterpret_cast<void*>(42);
    });
    
    // Execute the task
    task->execute();
    
    // Check that the task is complete
    EXPECT_TRUE(task->isCompleted());
    
    // Get the result
    void* result = task->getResult();
    EXPECT_EQ(reinterpret_cast<uintptr_t>(result), 42);
}

TEST_F(ThreadTest, ThreadTaskWait) {
    // Create a task that takes some time
    auto task = ThreadTask::create([]() -> void* {
        std::this_thread::sleep_for(100ms);
        return reinterpret_cast<void*>(42);
    });
    
    // Execute the task on a separate thread
    std::thread t([&]() {
        task->execute();
    });
    
    // Wait for the task to complete
    void* result = task->wait();
    EXPECT_EQ(reinterpret_cast<uintptr_t>(result), 42);
    
    // Join the thread
    t.join();
}

TEST_F(ThreadTest, ThreadPoolBasic) {
    // Create a thread pool
    auto pool = ThreadPool::create(2);
    ASSERT_NE(pool, nullptr);
    
    // Verify attributes
    EXPECT_EQ(pool->getNumThreads(), 2);
    EXPECT_EQ(pool->getNumPendingTasks(), 0);
}

TEST_F(ThreadTest, ThreadPoolSubmitTask) {
    // Create a thread pool
    auto pool = ThreadPool::create(2);
    ASSERT_NE(pool, nullptr);
    
    // Submit a task
    std::atomic<int> counter = 0;
    auto task = pool->submit([&]() -> void* {
        counter.fetch_add(1);
        return nullptr;
    });
    
    // Wait for the task to complete
    task->wait();
    
    // Verify the task executed
    EXPECT_EQ(counter, 1);
}

TEST_F(ThreadTest, ThreadPoolMultipleTasks) {
    // Create a thread pool
    auto pool = ThreadPool::create(4);
    ASSERT_NE(pool, nullptr);
    
    // Number of tasks
    const int numTasks = 100;
    
    // Submit multiple tasks
    std::atomic<int> counter = 0;
    std::vector<std::shared_ptr<ThreadTask>> tasks;
    
    for (int i = 0; i < numTasks; i++) {
        auto task = pool->submit([&, i]() -> void* {
            // Simulate work
            std::this_thread::sleep_for(10ms);
            counter.fetch_add(1);
            return reinterpret_cast<void*>(static_cast<uintptr_t>(i));
        });
        
        tasks.push_back(task);
    }
    
    // Wait for all tasks
    pool->waitAll();
    
    // Verify all tasks completed
    EXPECT_EQ(counter, numTasks);
    
    // Verify each task has the correct result
    for (int i = 0; i < numTasks; i++) {
        EXPECT_TRUE(tasks[i]->isCompleted());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(tasks[i]->getResult()), static_cast<uintptr_t>(i));
    }
}

TEST_F(ThreadTest, ThreadSpecificData) {
    // Initialize thread-specific data for the main thread
    MemoryArenaPtr arena = MemoryArena::create("test_thread", 1024 * 1024);
    auto logger = defaultLogger;
    auto errorMgr = defaultErrorManager;
    int userData = 42;
    
    ASSERT_TRUE(initializeThreadData(arena, logger, errorMgr, &userData));
    
    // Get thread data
    ThreadData* data = getThreadData();
    ASSERT_NE(data, nullptr);
    
    // Verify thread data
    EXPECT_EQ(data->arena, arena);
    EXPECT_EQ(data->logger, logger);
    EXPECT_EQ(data->errorMgr, errorMgr);
    EXPECT_EQ(data->userData, &userData);
    
    // Individual getters
    EXPECT_EQ(getThreadArena(), arena);
    EXPECT_EQ(getThreadLogger(), logger);
    EXPECT_EQ(getThreadErrorManager(), errorMgr);
    EXPECT_EQ(getThreadUserData(), &userData);
}

TEST_F(ThreadTest, ThreadSpecificDataFromThreads) {
    // Create a thread pool
    auto pool = ThreadPool::create(2);
    ASSERT_NE(pool, nullptr);
    
    // Submit a task that accesses thread-specific data
    auto task = pool->submit([]() -> void* {
        // Create arena for this thread
        auto arena = createThreadArena(1024 * 1024);
        if (!arena) return reinterpret_cast<void*>(1);
        
        // Verify thread arena
        if (getThreadArena() != arena) return reinterpret_cast<void*>(2);
        
        // Allocate from thread arena
        void* ptr = COIL_THREAD_ALLOC(1024);
        if (!ptr) return reinterpret_cast<void*>(3);
        
        // Set thread logger
        setThreadLogger(defaultLogger);
        if (getThreadLogger() != defaultLogger) return reinterpret_cast<void*>(4);
        
        // Set thread error manager
        setThreadErrorManager(defaultErrorManager);
        if (getThreadErrorManager() != defaultErrorManager) return reinterpret_cast<void*>(5);
        
        // Set thread user data
        int userData = 42;
        setThreadUserData(&userData);
        if (getThreadUserData() != &userData) return reinterpret_cast<void*>(6);
        
        return reinterpret_cast<void*>(0); // Success
    });
    
    // Wait for the task to complete
    void* result = task->wait();
    EXPECT_EQ(reinterpret_cast<uintptr_t>(result), 0) << "Thread data test failed with code: " 
                                                      << reinterpret_cast<uintptr_t>(result);
}

TEST_F(ThreadTest, ThreadPoolWithThreadLocalArenas) {
    // Create a thread pool with larger per-thread arenas
    auto pool = ThreadPool::create(2, 2 * 1024 * 1024);
    ASSERT_NE(pool, nullptr);
    
    // Submit tasks that use their thread-local arenas
    std::atomic<bool> success = true;
    std::vector<std::shared_ptr<ThreadTask>> tasks;
    
    for (int i = 0; i < 10; i++) {
        auto task = pool->submit([i]() -> void* {
            // Get thread arena
            auto arena = getThreadArena();
            if (!arena) return reinterpret_cast<void*>(-1);
            
            // Allocate from thread arena
            void* ptr = arena->allocate(1024 * 1024); // 1MB allocation
            if (!ptr) return reinterpret_cast<void*>(-2);
            
            // Do something with the memory
            memset(ptr, i, 1024 * 1024);
            
            // Verify first byte
            if (*static_cast<unsigned char*>(ptr) != static_cast<unsigned char>(i)) {
                return reinterpret_cast<void*>(-3);
            }
            
            return reinterpret_cast<void*>(0); // Success
        });
        
        tasks.push_back(task);
    }
    
    // Wait for all tasks
    pool->waitAll();
    
    // Verify all tasks succeeded
    for (auto& task : tasks) {
        EXPECT_TRUE(task->isCompleted());
        EXPECT_EQ(reinterpret_cast<uintptr_t>(task->getResult()), 0) 
            << "Task failed with code: " << reinterpret_cast<intptr_t>(task->getResult());
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}