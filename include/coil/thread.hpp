#pragma once

#include "coil/log.hpp"
#include "coil/err.hpp"
#include "coil/mem.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <condition_variable>
#include <atomic>

namespace coil {

/**
 * @brief Thread-specific data
 */
struct ThreadData {
    MemoryArenaPtr arena;                ///< Thread-specific memory arena
    std::shared_ptr<Logger> logger;     ///< Thread-specific logger
    std::shared_ptr<ErrorManager> errorMgr; ///< Thread-specific error manager
    void* userData = nullptr;           ///< User-defined thread-specific data
};

/**
 * @brief Get the current thread's data
 * 
 * @return ThreadData* 
 */
ThreadData* getThreadData();

/**
 * @brief Initialize thread-specific data
 * 
 * @param arena Memory arena
 * @param logger Logger
 * @param errorMgr Error manager
 * @param userData User data
 * @return true on success
 */
bool initializeThreadData(
    MemoryArenaPtr arena = nullptr,
    std::shared_ptr<Logger> logger = nullptr,
    std::shared_ptr<ErrorManager> errorMgr = nullptr,
    void* userData = nullptr);

/**
 * @brief Initialize the thread system
 * 
 * @return true on success
 */
bool initializeThreading();

/**
 * @brief Clean up the thread system
 */
void cleanupThreading();

/**
 * @brief Thread Task
 */
class ThreadTask {
public:
    /**
     * @brief Create a thread task
     * 
     * @param func Function to execute
     * @return std::shared_ptr<ThreadTask> 
     */
    template<typename Func>
    static std::shared_ptr<ThreadTask> create(Func&& func) {
        return std::shared_ptr<ThreadTask>(new ThreadTask(std::forward<Func>(func)));
    }
    
    /**
     * @brief Wait for the task to complete
     * 
     * @return void* Result
     */
    void* wait();
    
    /**
     * @brief Check if the task is completed
     * 
     * @return true if completed
     */
    bool isCompleted() const;
    
    /**
     * @brief Get the result
     * 
     * @return void* Result
     */
    void* getResult() const;
    
    /**
     * @brief Execute the task
     * 
     * @param result Result to set
     */
    void execute();
    
    /**
     * @brief Destructor
     */
    ~ThreadTask();
    
    // Delete copy and move constructors/assignments
    ThreadTask(const ThreadTask&) = delete;
    ThreadTask& operator=(const ThreadTask&) = delete;
    ThreadTask(ThreadTask&&) = delete;
    ThreadTask& operator=(ThreadTask&&) = delete;

private:
    template<typename Func>
    ThreadTask(Func&& func)
        : func_(std::forward<Func>(func))
        , completed_(false)
        , result_(nullptr) {
    }

    std::function<void*()> func_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> completed_;
    void* result_;
};

/**
 * @brief Thread Pool
 */
class ThreadPool {
public:
    /**
     * @brief Create a thread pool
     * 
     * @param numThreads Number of threads (0 = auto)
     * @param arenaSizePerThread Size of per-thread arenas
     * @param logger Logger
     * @param errorMgr Error manager
     * @return std::shared_ptr<ThreadPool> 
     */
    static std::shared_ptr<ThreadPool> create(
        size_t numThreads = 0,
        size_t arenaSizePerThread = 0,
        std::shared_ptr<Logger> logger = nullptr,
        std::shared_ptr<ErrorManager> errorMgr = nullptr);
    
    /**
     * @brief Submit a task to the pool
     * 
     * @param func Function to execute
     * @return std::shared_ptr<ThreadTask> 
     */
    template<typename Func>
    std::shared_ptr<ThreadTask> submit(Func&& func) {
        auto task = ThreadTask::create(std::forward<Func>(func));
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (!running_) {
                if (errorMgr_) {
                    StreamPosition pos;
                    pos.fileName = "thread";
                    errorMgr_->addError(ErrorCode::State, pos, 
                                      "Thread pool is not running");
                }
                return nullptr;
            }
            
            tasks_.push(task);
        }
        
        cond_.notify_one();
        return task;
    }
    
    /**
     * @brief Wait for all tasks to complete
     */
    void waitAll();
    
    /**
     * @brief Get the number of threads
     * 
     * @return size_t 
     */
    size_t getNumThreads() const;
    
    /**
     * @brief Get the number of pending tasks
     * 
     * @return size_t 
     */
    size_t getNumPendingTasks() const;
    
    /**
     * @brief Destructor
     */
    ~ThreadPool();
    
    // Delete copy and move constructors/assignments
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    ThreadPool(
        size_t numThreads,
        size_t arenaSize,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<ErrorManager> errorMgr);
    
    void workerFunc();
    
    size_t numThreads_;
    size_t arenaSize_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<ErrorManager> errorMgr_;
    
    std::vector<std::thread> threads_;
    std::queue<std::shared_ptr<ThreadTask>> tasks_;
    
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::condition_variable waitCond_;
    
    std::atomic<bool> running_;
    std::atomic<size_t> activeThreads_;
};

/**
 * @brief Create a thread-specific memory arena
 * 
 * @param size Size
 * @return MemoryArenaPtr 
 */
MemoryArenaPtr createThreadArena(size_t size);

/**
 * @brief Get the current thread's memory arena
 * 
 * @return MemoryArenaPtr 
 */
MemoryArenaPtr getThreadArena();

/**
 * @brief Set the current thread's memory arena
 * 
 * @param arena Arena
 */
void setThreadArena(MemoryArenaPtr arena);

/**
 * @brief Get the current thread's logger
 * 
 * @return std::shared_ptr<Logger> 
 */
std::shared_ptr<Logger> getThreadLogger();

/**
 * @brief Set the current thread's logger
 * 
 * @param logger Logger
 */
void setThreadLogger(std::shared_ptr<Logger> logger);

/**
 * @brief Get the current thread's error manager
 * 
 * @return std::shared_ptr<ErrorManager> 
 */
std::shared_ptr<ErrorManager> getThreadErrorManager();

/**
 * @brief Set the current thread's error manager
 * 
 * @param errorMgr Error manager
 */
void setThreadErrorManager(std::shared_ptr<ErrorManager> errorMgr);

/**
 * @brief Get the current thread's user data
 * 
 * @return void* 
 */
void* getThreadUserData();

/**
 * @brief Set the current thread's user data
 * 
 * @param userData User data
 */
void setThreadUserData(void* userData);

} // namespace coil