#include "coil/mem.hpp"
#include <cstring>
#include <algorithm>

namespace coil {

// Global memory arena
MemoryArenaPtr globalArena;

// Thread-specific arena getter
ThreadArenaGetter threadArenaGetter;

// Default alignment for allocations
const size_t DEFAULT_ALIGNMENT = 8;

// MemoryArena implementation

MemoryArena::MemoryArena(
    const std::string& name,
    size_t size,
    bool threadSafe,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr)
    : name_(name)
    , size_(size)
    , used_(0)
    , threadSafe_(threadSafe)
    , logger_(logger ? logger : defaultLogger)
    , errorMgr_(errorMgr ? errorMgr : defaultErrorManager) {
    
    // Align size to ensure alignment of allocations
    size_ = alignSize(size_, DEFAULT_ALIGNMENT);
    
    // Allocate memory
    memory_ = malloc(size_);
    if (!memory_ && size_ > 0) {
        if (errorMgr_) {
            StreamPosition pos;
            pos.fileName = "memory";
            errorMgr_->addError(ErrorCode::Memory, pos, 
                             "Failed to allocate memory for arena");
        }
    }
}

MemoryArenaPtr MemoryArena::create(
    const std::string& name,
    size_t size,
    bool threadSafe,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<ErrorManager> errorMgr) {
    
    return std::shared_ptr<MemoryArena>(new MemoryArena(
        name, size, threadSafe, logger, errorMgr));
}

MemoryArenaPtr MemoryArena::createChild(
    const std::string& name,
    size_t size,
    bool threadSafe) {
    
    auto child = create(name, size, threadSafe, logger_, errorMgr_);
    
    if (child) {
        if (threadSafe_) {
            std::lock_guard<std::mutex> lock(mutex_);
        }

        // Link child to parent
        child->parent_ = shared_from_this();
        
        // Add to children list
        children_.push_back(child);
    }
    
    return child;
}

void* MemoryArena::allocate(size_t size) {
    return allocateAligned(size, DEFAULT_ALIGNMENT);
}

void* MemoryArena::allocateAligned(size_t size, size_t alignment) {
    if (size == 0 || !memory_) return nullptr;
    
    if (threadSafe_) std::lock_guard<std::mutex> lock(mutex_);
    
    // Align the current position
    size_t alignedOffset = alignSize(used_, alignment);
    size_t alignedSize = alignSize(size, alignment);
    
    // Check if there's enough space
    if (alignedOffset + alignedSize > size_) {
        if (errorMgr_) {
            StreamPosition pos;
            pos.fileName = "memory";
            errorMgr_->addError(ErrorCode::Memory, pos, 
                             "Arena '" + name_ + "' out of memory (requested " + 
                             std::to_string(alignedSize) + " bytes, available " + 
                             std::to_string(size_ - alignedOffset) + " bytes)");
        }
        
        return nullptr;
    }
    
    // Allocate memory
    void* ptr = static_cast<uint8_t*>(memory_) + alignedOffset;
    used_ = alignedOffset + alignedSize;
    
    // Update statistics
    stats_.totalAllocated += alignedSize;
    stats_.allocationCount++;
    stats_.currentUsage += alignedSize;
    
    if (stats_.currentUsage > stats_.peakUsage) {
        stats_.peakUsage = stats_.currentUsage;
    }
    
    return ptr;
}

void* MemoryArena::callocate(size_t count, size_t size) {
    size_t totalSize = count * size;
    void* ptr = allocate(totalSize);
    
    if (ptr) {
        memset(ptr, 0, totalSize);
    }
    
    return ptr;
}

void* MemoryArena::cloneMemory(const void* ptr, size_t size) {
    if (!ptr) return nullptr;
    
    void* newPtr = allocate(size);
    
    if (newPtr) {
        memcpy(newPtr, ptr, size);
    }
    
    return newPtr;
}

char* MemoryArena::cloneString(const char* str) {
    if (!str) return nullptr;
    
    size_t len = strlen(str) + 1;
    char* newStr = static_cast<char*>(allocate(len));
    
    if (newStr) {
        memcpy(newStr, str, len);
    }
    
    return newStr;
}

char* MemoryArena::cloneString(const std::string& str) {
    size_t len = str.length() + 1;
    char* newStr = static_cast<char*>(allocate(len));
    
    if (newStr) {
        memcpy(newStr, str.c_str(), len);
    }
    
    return newStr;
}

void MemoryArena::reset() {
    if (threadSafe_) std::lock_guard<std::mutex> lock(mutex_);
    
    // Reset arena
    used_ = 0;
    
    // Update statistics
    stats_.totalFreed += stats_.currentUsage;
    stats_.freeCount++;
    stats_.currentUsage = 0;
}

MemoryStats MemoryArena::getStats() const {
    if (threadSafe_) std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void MemoryArena::logStats() const {
    MemoryStats stats = getStats();
    
    COIL_INFO(logger_, "Arena '%s' statistics:", name_.c_str());
    COIL_INFO(logger_, "  Total size:        %zu bytes", size_);
    COIL_INFO(logger_, "  Used size:         %zu bytes (%.2f%%)", 
             used_, (double)used_ / size_ * 100.0);
    COIL_INFO(logger_, "  Total allocated:   %zu bytes", stats.totalAllocated);
    COIL_INFO(logger_, "  Total freed:       %zu bytes", stats.totalFreed);
    COIL_INFO(logger_, "  Current usage:     %zu bytes", stats.currentUsage);
    COIL_INFO(logger_, "  Peak usage:        %zu bytes", stats.peakUsage);
    COIL_INFO(logger_, "  Allocation count:  %zu", stats.allocationCount);
    COIL_INFO(logger_, "  Free count:        %zu", stats.freeCount);
}

MemoryArena::~MemoryArena() {
    // First, clean up all children
    children_.clear();
    
    // Log final statistics if logger available
    if (logger_) {
        logStats();
    }
    
    // Free memory
    if (memory_) {
        free(memory_);
        memory_ = nullptr;
    }
}

size_t MemoryArena::alignSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void* MemoryArena::alignPointer(void* ptr, size_t alignment) {
    return reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(ptr) + alignment - 1) & ~(alignment - 1));
}

void setThreadArenaGetter(ThreadArenaGetter getter) {
    threadArenaGetter = getter;
}

// Moved the implementation to thread.cpp to avoid multiple definitions
// MemoryArenaPtr getThreadArena() {
//     if (threadArenaGetter) {
//         MemoryArenaPtr arena = threadArenaGetter();
//         if (arena) {
//             return arena;
//         }
//     }
//     
//     // Fall back to global arena if no thread arena is available
//     return globalArena;
// }

void initializeMemory() {
    if (!defaultLogger) {
        initializeLogging();
    }
    
    if (!defaultErrorManager) {
        initializeErrorHandling();
    }
    
    if (!globalArena) {
        // Create a 64MB global arena
        globalArena = MemoryArena::create(
            "global", 64 * 1024 * 1024, true, 
            defaultLogger, defaultErrorManager);
    }
}

void cleanupMemory() {
    globalArena.reset();
}

} // namespace coil