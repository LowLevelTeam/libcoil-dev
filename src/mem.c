/* src/mem.c */
#include "coil/mem.h"
#include <stdlib.h>
#include <string.h>

/* Global memory arena */
coil_memory_arena_t *coil_global_arena = NULL;

/* Thread-specific arena getter */
coil_thread_arena_getter_fn coil_thread_arena_getter = NULL;

/* Default alignment for allocations */
#define COIL_DEFAULT_ALIGNMENT 8

/* Helper functions for memory alignment */
static size_t coil_align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void *coil_align_pointer(void *ptr, size_t alignment) {
    return (void *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
}

int coil_memory_arena_init(coil_memory_arena_t *arena, 
                          const char *name,
                          size_t size, 
                          bool thread_safe,
                          coil_logger_t *logger,
                          coil_error_manager_t *error_mgr) {
    if (!arena) return -1;
    
    /* Align size to ensure alignment of allocations */
    size = coil_align_size(size, COIL_DEFAULT_ALIGNMENT);
    
    arena->memory = malloc(size);
    if (!arena->memory) {
        if (error_mgr) {
            coil_stream_pos_t pos = {0};
            pos.file_name = "memory";
            coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                           "Failed to allocate memory for arena");
        }
        return -1;
    }
    
    if (name) {
        arena->name = strdup(name);
    } else {
        arena->name = strdup("unnamed");
    }
    
    if (!arena->name) {
        free(arena->memory);
        
        if (error_mgr) {
            coil_stream_pos_t pos = {0};
            pos.file_name = "memory";
            coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                           "Failed to allocate memory for arena name");
        }
        
        return -1;
    }
    
    arena->size = size;
    arena->used = 0;
    arena->thread_safe = thread_safe;
    arena->initialized = true;
    arena->logger = logger ? logger : coil_default_logger;
    arena->error_mgr = error_mgr ? error_mgr : coil_default_error_manager;
    arena->parent = NULL;
    arena->next = NULL;
    arena->children = NULL;
    
    /* Initialize statistics */
    memset(&arena->stats, 0, sizeof(coil_memory_stats_t));
    
    if (thread_safe) {
        if (pthread_mutex_init(&arena->lock, NULL) != 0) {
            free(arena->name);
            free(arena->memory);
            return -1;
        }
    }
    
    return 0;
}

coil_memory_arena_t *coil_memory_arena_create(const char *name,
                                             size_t size, 
                                             bool thread_safe,
                                             coil_logger_t *logger,
                                             coil_error_manager_t *error_mgr) {
    coil_memory_arena_t *arena = (coil_memory_arena_t *)malloc(sizeof(coil_memory_arena_t));
    if (!arena) {
        if (error_mgr) {
            coil_stream_pos_t pos = {0};
            pos.file_name = "memory";
            coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                           "Failed to allocate memory for arena structure");
        }
        return NULL;
    }
    
    if (coil_memory_arena_init(arena, name, size, thread_safe, logger, error_mgr) != 0) {
        free(arena);
        return NULL;
    }
    
    return arena;
}

coil_memory_arena_t *coil_memory_arena_create_child(coil_memory_arena_t *parent,
                                                   const char *name,
                                                   size_t size, 
                                                   bool thread_safe) {
    if (!parent || !parent->initialized) return NULL;
    
    coil_memory_arena_t *child = coil_memory_arena_create(
        name, size, thread_safe, parent->logger, parent->error_mgr);
        
    if (!child) return NULL;
    
    if (parent->thread_safe) {
        pthread_mutex_lock(&parent->lock);
    }
    
    /* Link child to parent */
    child->parent = parent;
    
    /* Add to children list */
    if (parent->children) {
        coil_memory_arena_t *last = parent->children;
        while (last->next) {
            last = last->next;
        }
        last->next = child;
    } else {
        parent->children = child;
    }
    
    if (parent->thread_safe) {
        pthread_mutex_unlock(&parent->lock);
    }
    
    return child;
}

void *coil_memory_arena_alloc(coil_memory_arena_t *arena, size_t size) {
    return coil_memory_arena_alloc_aligned(arena, size, COIL_DEFAULT_ALIGNMENT);
}

void *coil_memory_arena_alloc_aligned(coil_memory_arena_t *arena, size_t size, size_t alignment) {
    if (!arena || !arena->initialized || size == 0) return NULL;
    
    if (arena->thread_safe) {
        pthread_mutex_lock(&arena->lock);
    }
    
    /* Align the current position */
    size_t aligned_offset = coil_align_size(arena->used, alignment);
    size_t aligned_size = coil_align_size(size, alignment);
    
    /* Check if there's enough space */
    if (aligned_offset + aligned_size > arena->size) {
        if (arena->error_mgr) {
            coil_stream_pos_t pos = {0};
            pos.file_name = "memory";
            coil_error_error(arena->error_mgr, COIL_ERR_MEMORY, &pos, 
                           "Arena '%s' out of memory (requested %zu bytes, available %zu bytes)",
                           arena->name, aligned_size, arena->size - aligned_offset);
        }
        
        if (arena->thread_safe) {
            pthread_mutex_unlock(&arena->lock);
        }
        
        return NULL;
    }
    
    /* Allocate memory */
    void *ptr = (uint8_t *)arena->memory + aligned_offset;
    arena->used = aligned_offset + aligned_size;
    
    /* Update statistics */
    arena->stats.total_allocated += aligned_size;
    arena->stats.allocation_count++;
    arena->stats.current_usage += aligned_size;
    
    if (arena->stats.current_usage > arena->stats.peak_usage) {
        arena->stats.peak_usage = arena->stats.current_usage;
    }
    
    if (arena->thread_safe) {
        pthread_mutex_unlock(&arena->lock);
    }
    
    return ptr;
}

void *coil_memory_arena_calloc(coil_memory_arena_t *arena, size_t count, size_t size) {
    size_t total_size = count * size;
    void *ptr = coil_memory_arena_alloc(arena, total_size);
    
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

void *coil_memory_arena_clone(coil_memory_arena_t *arena, const void *ptr, size_t size) {
    if (!ptr) return NULL;
    
    void *new_ptr = coil_memory_arena_alloc(arena, size);
    
    if (new_ptr) {
        memcpy(new_ptr, ptr, size);
    }
    
    return new_ptr;
}

char *coil_memory_arena_strdup(coil_memory_arena_t *arena, const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char *new_str = (char *)coil_memory_arena_alloc(arena, len);
    
    if (new_str) {
        memcpy(new_str, str, len);
    }
    
    return new_str;
}

void coil_memory_arena_free(coil_memory_arena_t *arena, void *ptr) {
    /* Individual frees are not supported in arena allocators */
    /* This is a no-op function provided for compatibility */
    (void)arena;
    (void)ptr;
}

void coil_memory_arena_reset(coil_memory_arena_t *arena) {
    if (!arena || !arena->initialized) return;
    
    if (arena->thread_safe) {
        pthread_mutex_lock(&arena->lock);
    }
    
    /* Reset arena */
    arena->used = 0;
    
    /* Update statistics */
    arena->stats.total_freed += arena->stats.current_usage;
    arena->stats.free_count++;
    arena->stats.current_usage = 0;
    
    if (arena->thread_safe) {
        pthread_mutex_unlock(&arena->lock);
    }
}

coil_memory_stats_t coil_memory_arena_get_stats(coil_memory_arena_t *arena) {
    coil_memory_stats_t stats = {0};
    
    if (!arena || !arena->initialized) return stats;
    
    if (arena->thread_safe) {
        pthread_mutex_lock(&arena->lock);
    }
    
    stats = arena->stats;
    
    if (arena->thread_safe) {
        pthread_mutex_unlock(&arena->lock);
    }
    
    return stats;
}

void coil_memory_arena_log_stats(coil_memory_arena_t *arena) {
    if (!arena || !arena->initialized || !arena->logger) return;
    
    coil_memory_stats_t stats = coil_memory_arena_get_stats(arena);
    
    COIL_INFO(arena->logger, "Arena '%s' statistics:", arena->name);
    COIL_INFO(arena->logger, "  Total size:        %zu bytes", arena->size);
    COIL_INFO(arena->logger, "  Used size:         %zu bytes (%.2f%%)", 
             arena->used, (double)arena->used / arena->size * 100.0);
    COIL_INFO(arena->logger, "  Total allocated:   %zu bytes", stats.total_allocated);
    COIL_INFO(arena->logger, "  Total freed:       %zu bytes", stats.total_freed);
    COIL_INFO(arena->logger, "  Current usage:     %zu bytes", stats.current_usage);
    COIL_INFO(arena->logger, "  Peak usage:        %zu bytes", stats.peak_usage);
    COIL_INFO(arena->logger, "  Allocation count:  %zu", stats.allocation_count);
    COIL_INFO(arena->logger, "  Free count:        %zu", stats.free_count);
}

void coil_memory_arena_cleanup(coil_memory_arena_t *arena) {
    if (!arena || !arena->initialized) return;
    
    /* First, clean up all children */
    coil_memory_arena_t *child = arena->children;
    while (child) {
        coil_memory_arena_t *next = child->next;
        coil_memory_arena_destroy(child);
        child = next;
    }
    
    if (arena->thread_safe) {
        pthread_mutex_destroy(&arena->lock);
    }
    
    /* Log final statistics if logger available */
    if (arena->logger) {
        coil_memory_arena_log_stats(arena);
    }
    
    free(arena->memory);
    free(arena->name);
    
    arena->memory = NULL;
    arena->name = NULL;
    arena->initialized = false;
}

void coil_memory_arena_destroy(coil_memory_arena_t *arena) {
    if (!arena) return;
    
    /* Don't destroy if it's a child - parent will handle that */
    if (arena->parent) {
        return;
    }
    
    coil_memory_arena_cleanup(arena);
    free(arena);
}

void coil_memory_set_thread_arena_getter(coil_thread_arena_getter_fn getter) {
    coil_thread_arena_getter = getter;
}

coil_memory_arena_t *coil_memory_get_thread_arena(void) {
    if (coil_thread_arena_getter) {
        coil_memory_arena_t *arena = coil_thread_arena_getter();
        if (arena) {
            return arena;
        }
    }
    
    /* Fall back to global arena if no thread arena is available */
    return coil_global_arena;
}

void coil_memory_init(void) {
    if (!coil_default_logger) {
        coil_log_init();
    }
    
    if (!coil_default_error_manager) {
        coil_error_init();
    }
    
    if (!coil_global_arena) {
        /* Create a 64MB global arena */
        coil_global_arena = coil_memory_arena_create(
            "global", 64 * 1024 * 1024, true, 
            coil_default_logger, coil_default_error_manager);
    }
}

void coil_memory_cleanup(void) {
    if (coil_global_arena) {
        coil_memory_arena_destroy(coil_global_arena);
        coil_global_arena = NULL;
    }
}