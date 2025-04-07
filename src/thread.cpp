#include "coil/thread.hpp"
#include <cstring>
#include <thread>

namespace coil {

// Thread-specific data key
thread_local ThreadData* threadData = nullptr;

ThreadData* getThreadData() {
    return threadData;
}

bool initializeThreadData(
    MemoryArenaPtr arena,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr,
    void* userData) {
    
    // Create new thread data if it doesn't exist
    if (!threadData) {
        threadData = new ThreadData();
        if (!threadData) {
            return false;
        }
    }
    
    // Set the provided data
    if (arena) {
        threadData->arena = arena;
    }
    
    if (logger) {
        threadData->logger = logger;
    }
    
    if (errorMgr) {
        threadData->errorMgr = errorMgr;
    }
    
    threadData->userData = userData;
    
    return true;
}

bool initializeThreading() {
    // Nothing specific to initialize for C++ threading
    return true;
}

void cleanupThreading() {
    // Clean up thread-specific data for the main thread
    if (threadData) {
        delete threadData;
        threadData = nullptr;
    }
}

// ThreadTask implementation

void* ThreadTask::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    while (!completed_) {
        cond_.wait(lock);
    }
    
    return result_;
}

bool ThreadTask::isCompleted() const {
    return completed_;
}

void* ThreadTask::getResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return result_;
}

void ThreadTask::execute() {
    void* result = func_();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result_ = result;
        completed_ = true;
    }
    
    cond_.notify_all();
}

ThreadTask::~ThreadTask() {
    // No special cleanup needed
}

// ThreadPool implementation

ThreadPool::ThreadPool(
    size_t numThreads,
    size_t arenaSize,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : numThreads_(numThreads)
    , arenaSize_(arenaSize)
    , logger_(logger ? logger : defaultLogger)
    , errorMgr_(errorMgr ? errorMgr : defaultErrorManager)
    , running_(true)
    , activeThreads_(0) {
    
    // Determine number of threads if not specified
    if (numThreads_ == 0) {
        numThreads_ = std::thread::hardware_concurrency();
        if (numThreads_ == 0) {
            numThreads_ = 4; // Default if can't determine
        }
    }
    
    // Default arena size
    if (arenaSize_ == 0) {
        arenaSize_ = 1024 * 1024; // 1MB
    }
    
    // Create worker threads
    threads_.reserve(numThreads_);
    for (size_t i = 0; i < numThreads_; ++i) {
        threads_.emplace_back(&ThreadPool::workerFunc, this);
    }
}

std::shared_ptr<ThreadPool> ThreadPool::create(
    size_t numThreads,
    size_t arenaSizePerThread,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr) {
    
    return std::shared_ptr<ThreadPool>(new ThreadPool(
        numThreads, arenaSizePerThread, logger, errorMgr));
}

void ThreadPool::workerFunc() {
    // Create a thread-specific arena
    auto arena = MemoryArena::create(
        "thread", arenaSize_, false, logger_, errorMgr_);
    
    // Initialize thread-specific data
    initializeThreadData(arena, logger_, errorMgr_, nullptr);
    
    while (true) {
        std::shared_ptr<ThreadTask> task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait for a task or shutdown
            cond_.wait(lock, [this] {
                return !tasks_.empty() || !running_;
            });
            
            // Check if we should exit
            if (!running_ && tasks_.empty()) {
                break;
            }
            
            // Get the next task
            if (!tasks_.empty()) {
                task = tasks_.front();
                tasks_.pop();
                activeThreads_++;
            }
        }
        
        // Execute the task if we got one
        if (task) {
            task->execute();
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                activeThreads_--;
            }
            
            // Signal if all tasks have been completed
            if (activeThreads_ == 0 && tasks_.empty()) {
                waitCond_.notify_all();
            }
        }
    }
    
    // Clean up thread-specific data
    if (threadData) {
        delete threadData;
        threadData = nullptr;
    }
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    waitCond_.wait(lock, [this] {
        return activeThreads_ == 0 && tasks_.empty();
    });
}

size_t ThreadPool::getNumThreads() const {
    return numThreads_;
}

size_t ThreadPool::getNumPendingTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    
    cond_.notify_all();
    
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Thread utility functions

MemoryArenaPtr createThreadArena(size_t size) {
    // Create a new arena for the current thread
    auto arena = MemoryArena::create(
        "thread", size, false, defaultLogger, defaultErrorManager);
    
    if (arena) {
        // Set it as the current thread's arena
        setThreadArena(arena);
    }
    
    return arena;
}

// Implementation of getThreadArena() to avoid multiple definitions
MemoryArenaPtr getThreadArena() {
    if (threadData) {
        return threadData->arena;
    }
    
    if (threadArenaGetter) {
        MemoryArenaPtr arena = threadArenaGetter();
        if (arena) {
            return arena;
        }
    }
    
    // Fall back to global arena if no thread arena is available
    return globalArena;
}

void setThreadArena(MemoryArenaPtr arena) {
    if (!threadData) {
        initializeThreadData();
    }
    
    if (threadData) {
        threadData->arena = arena;
    }
}

std::shared_ptr<Logger> getThreadLogger() {
    if (threadData) {
        return threadData->logger;
    }
    
    return nullptr;
}

void setThreadLogger(std::shared_ptr<Logger> logger) {
    if (!threadData) {
        initializeThreadData();
    }
    
    if (threadData) {
        threadData->logger = logger;
    }
}

std::shared_ptr<ErrorManager> getThreadErrorManager() {
    if (threadData) {
        return threadData->errorMgr;
    }
    
    return nullptr;
}

void setThreadErrorManager(std::shared_ptr<ErrorManager> errorMgr) {
    if (!threadData) {
        initializeThreadData();
    }
    
    if (threadData) {
        threadData->errorMgr = errorMgr;
    }
}

void* getThreadUserData() {
    if (threadData) {
        return threadData->userData;
    }
    
    return nullptr;
}

void setThreadUserData(void* userData) {
    if (!threadData) {
        initializeThreadData();
    }
    
    if (threadData) {
        threadData->userData = userData;
    }
}

} // namespace coil