#pragma once

#include "coil/log.hpp"
#include "coil/err.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <thread>

namespace coil {

/**
 * @brief Memory allocation statistics
 */
struct MemoryStats {
    size_t totalAllocated = 0;   ///< Total bytes allocated
    size_t totalFreed = 0;       ///< Total bytes freed
    size_t currentUsage = 0;     ///< Current bytes in use
    size_t peakUsage = 0;        ///< Peak memory usage
    size_t allocationCount = 0;  ///< Number of allocations
    size_t freeCount = 0;        ///< Number of frees
};

/**
 * @brief Forward declaration
 */
class MemoryArena;
using MemoryArenaPtr = std::shared_ptr<MemoryArena>;

/**
 * @brief Thread-specific arena getter function type
 */
using ThreadArenaGetter = std::function<MemoryArenaPtr()>;

/**
 * @brief Modern C++ Memory Arena class
 */
class MemoryArena : public std::enable_shared_from_this<MemoryArena> {
public:
    /**
     * @brief Create a memory arena
     * 
     * @param name Arena name for debugging
     * @param size Total size of arena
     * @param threadSafe Whether the arena is thread-safe
     * @param logger Logger
     * @param errorMgr Error manager
     * @return MemoryArenaPtr 
     */
    static MemoryArenaPtr create(
        const std::string& name,
        size_t size,
        bool threadSafe = true,
        std::shared_ptr<Logger> logger = nullptr,
        std::shared_ptr<ErrorManager> errorMgr = nullptr);
    
    /**
     * @brief Create a child arena
     * 
     * @param name Arena name for debugging
     * @param size Total size of arena
     * @param threadSafe Whether the arena is thread-safe
     * @return MemoryArenaPtr 
     */
    MemoryArenaPtr createChild(const std::string& name, size_t size, bool threadSafe = true);
    
    /**
     * @brief Allocate memory from the arena
     * 
     * @param size Size to allocate
     * @return void* Pointer to allocated memory, or nullptr if failed
     */
    void* allocate(size_t size);
    
    /**
     * @brief Allocate aligned memory from the arena
     * 
     * @param size Size to allocate
     * @param alignment Alignment requirement
     * @return void* Pointer to allocated memory, or nullptr if failed
     */
    void* allocateAligned(size_t size, size_t alignment);
    
    /**
     * @brief Allocate and zero memory from the arena
     * 
     * @param count Number of elements
     * @param size Size of each element
     * @return void* Pointer to allocated memory, or nullptr if failed
     */
    void* callocate(size_t count, size_t size);
    
    /**
     * @brief Allocate an object of type T
     * 
     * @tparam T Object type
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return T* Pointer to new object, or nullptr if failed
     */
    template<typename T, typename... Args>
    T* createObject(Args&&... args) {
        static_assert(std::is_constructible<T, Args...>::value, 
                    "Cannot construct object of type T with the given arguments");
        
        void* memory = allocate(sizeof(T));
        if (!memory) return nullptr;
        
        return new(memory) T(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Clone a memory block to the arena
     * 
     * @param ptr Pointer to memory to clone
     * @param size Size of memory to clone
     * @return void* Pointer to cloned memory, or nullptr if failed
     */
    void* cloneMemory(const void* ptr, size_t size);
    
    /**
     * @brief Clone a string to the arena
     * 
     * @param str String to clone
     * @return char* Pointer to cloned string, or nullptr if failed
     */
    char* cloneString(const char* str);
    
    /**
     * @brief Clone a std::string to the arena
     * 
     * @param str String to clone
     * @return char* Pointer to cloned string, or nullptr if failed
     */
    char* cloneString(const std::string& str);
    
    /**
     * @brief Reset the arena (clear all allocations)
     */
    void reset();
    
    /**
     * @brief Get memory statistics for the arena
     * 
     * @return MemoryStats 
     */
    MemoryStats getStats() const;
    
    /**
     * @brief Log memory statistics for the arena
     */
    void logStats() const;
    
    /**
     * @brief Get the arena name
     * 
     * @return const std::string& 
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief Get current arena size
     * 
     * @return size_t 
     */
    size_t getSize() const { return size_; }
    
    /**
     * @brief Get used bytes
     * 
     * @return size_t 
     */
    size_t getUsed() const { return used_; }
    
    /**
     * @brief Get available bytes
     * 
     * @return size_t 
     */
    size_t getAvailable() const { return size_ - used_; }
    
    /**
     * @brief Check if the arena is thread-safe
     * 
     * @return true if thread-safe
     */
    bool isThreadSafe() const { return threadSafe_; }
    
    /**
     * @brief Destructor
     */
    ~MemoryArena();
    
    // Delete copy constructors/assignments
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    MemoryArena(MemoryArena&&) = delete;
    MemoryArena& operator=(MemoryArena&&) = delete;

private:
    MemoryArena(
        const std::string& name,
        size_t size,
        bool threadSafe,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<ErrorManager> errorMgr);
    
    std::string name_;
    void* memory_ = nullptr;
    size_t size_ = 0;
    size_t used_ = 0;
    bool threadSafe_ = false;
    mutable std::mutex mutex_;
    MemoryStats stats_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<ErrorManager> errorMgr_;
    MemoryArenaPtr parent_;
    std::vector<MemoryArenaPtr> children_;
    
    static size_t alignSize(size_t size, size_t alignment);
    static void* alignPointer(void* ptr, size_t alignment);
};

// Global memory arena
extern MemoryArenaPtr globalArena;

// Thread-specific arena getter
extern ThreadArenaGetter threadArenaGetter;

// Initialize library memory management
void initializeMemory();

// Cleanup library memory management
void cleanupMemory();

// Set the thread arena getter
void setThreadArenaGetter(ThreadArenaGetter getter);

// Get the current thread arena
MemoryArenaPtr getThreadArena();

// Helper template for array allocation
template<typename T>
T* allocateArray(MemoryArenaPtr arena, size_t count) {
    return static_cast<T*>(arena->callocate(count, sizeof(T)));
}

// Convenience macros for current thread arena
#define COIL_THREAD_ALLOC(size) \
    coil::getThreadArena()->allocate(size)

#define COIL_THREAD_CALLOC(count, size) \
    coil::getThreadArena()->callocate(count, size)

#define COIL_THREAD_STRDUP(str) \
    coil::getThreadArena()->cloneString(str)

#define COIL_THREAD_CLONE(ptr, size) \
    coil::getThreadArena()->cloneMemory(ptr, size)

// Convenience macros for global arena
#define COIL_GLOBAL_ALLOC(size) \
    coil::globalArena->allocate(size)

#define COIL_GLOBAL_CALLOC(count, size) \
    coil::globalArena->callocate(count, size)

#define COIL_GLOBAL_STRDUP(str) \
    coil::globalArena->cloneString(str)

#define COIL_GLOBAL_CLONE(ptr, size) \
    coil::globalArena->cloneMemory(ptr, size)

} // namespace coil