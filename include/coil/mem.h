/* coil/mem.h */
#ifndef COIL_MEMORY_H
#define COIL_MEMORY_H

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "coil/log.h"
#include "coil/err.h"

/* Memory allocation statistics */
typedef struct {
    size_t total_allocated;    /* Total bytes allocated */
    size_t total_freed;        /* Total bytes freed */
    size_t current_usage;      /* Current bytes in use */
    size_t peak_usage;         /* Peak memory usage */
    size_t allocation_count;   /* Number of allocations */
    size_t free_count;         /* Number of frees */
} coil_memory_stats_t;

/* Forward declaration */
typedef struct coil_memory_arena coil_memory_arena_t;

/* Memory arena */
struct coil_memory_arena {
    char *name;                          /* Arena name for debugging */
    void *memory;                        /* Arena memory */
    size_t size;                         /* Total size of arena */
    size_t used;                         /* Used size of arena */
    bool thread_safe;                    /* Whether the arena is thread-safe */
    pthread_mutex_t lock;                /* Mutex for thread-safety */
    bool initialized;                    /* Whether the arena is initialized */
    coil_memory_stats_t stats;           /* Memory statistics */
    coil_logger_t *logger;               /* Logger */
    coil_error_manager_t *error_mgr;     /* Error manager */
    struct coil_memory_arena *parent;    /* Parent arena (or NULL if root) */
    struct coil_memory_arena *next;      /* Next arena in list (for hierarchical arenas) */
    struct coil_memory_arena *children;  /* Child arenas */
};

/* Global memory arena */
extern coil_memory_arena_t *coil_global_arena;

/* Thread-specific arena getter function type */
typedef coil_memory_arena_t *(*coil_thread_arena_getter_fn)(void);

/* Thread-specific arena getter */
extern coil_thread_arena_getter_fn coil_thread_arena_getter;

/* Initialize a memory arena */
int coil_memory_arena_init(coil_memory_arena_t *arena, 
                          const char *name,
                          size_t size, 
                          bool thread_safe,
                          coil_logger_t *logger,
                          coil_error_manager_t *error_mgr);

/* Create a new memory arena */
coil_memory_arena_t *coil_memory_arena_create(const char *name,
                                             size_t size, 
                                             bool thread_safe,
                                             coil_logger_t *logger,
                                             coil_error_manager_t *error_mgr);

/* Create a child arena */
coil_memory_arena_t *coil_memory_arena_create_child(coil_memory_arena_t *parent,
                                                   const char *name,
                                                   size_t size, 
                                                   bool thread_safe);

/* Allocate memory from an arena */
void *coil_memory_arena_alloc(coil_memory_arena_t *arena, size_t size);

/* Allocate aligned memory from an arena */
void *coil_memory_arena_alloc_aligned(coil_memory_arena_t *arena, size_t size, size_t alignment);

/* Allocate and zero memory from an arena */
void *coil_memory_arena_calloc(coil_memory_arena_t *arena, size_t count, size_t size);

/* Clone a memory block to an arena */
void *coil_memory_arena_clone(coil_memory_arena_t *arena, const void *ptr, size_t size);

/* Clone a string to an arena */
char *coil_memory_arena_strdup(coil_memory_arena_t *arena, const char *str);

/* Free memory in an arena (no-op for this implementation) */
void coil_memory_arena_free(coil_memory_arena_t *arena, void *ptr);

/* Reset an arena (clear all allocations) */
void coil_memory_arena_reset(coil_memory_arena_t *arena);

/* Get memory statistics for an arena */
coil_memory_stats_t coil_memory_arena_get_stats(coil_memory_arena_t *arena);

/* Log memory statistics for an arena */
void coil_memory_arena_log_stats(coil_memory_arena_t *arena);

/* Cleanup an arena */
void coil_memory_arena_cleanup(coil_memory_arena_t *arena);

/* Destroy an arena */
void coil_memory_arena_destroy(coil_memory_arena_t *arena);

/* Set the thread arena getter */
void coil_memory_set_thread_arena_getter(coil_thread_arena_getter_fn getter);

/* Get the current thread arena */
coil_memory_arena_t *coil_memory_get_thread_arena(void);

/* Initialize library memory management */
void coil_memory_init(void);

/* Cleanup library memory management */
void coil_memory_cleanup(void);

/* Convenience macros for current thread arena */
#define COIL_THREAD_ALLOC(size) \
    coil_memory_arena_alloc(coil_memory_get_thread_arena(), size)

#define COIL_THREAD_CALLOC(count, size) \
    coil_memory_arena_calloc(coil_memory_get_thread_arena(), count, size)

#define COIL_THREAD_STRDUP(str) \
    coil_memory_arena_strdup(coil_memory_get_thread_arena(), str)

#define COIL_THREAD_CLONE(ptr, size) \
    coil_memory_arena_clone(coil_memory_get_thread_arena(), ptr, size)

/* Convenience macros for global arena */
#define COIL_GLOBAL_ALLOC(size) \
    coil_memory_arena_alloc(coil_global_arena, size)

#define COIL_GLOBAL_CALLOC(count, size) \
    coil_memory_arena_calloc(coil_global_arena, count, size)

#define COIL_GLOBAL_STRDUP(str) \
    coil_memory_arena_strdup(coil_global_arena, str)

#define COIL_GLOBAL_CLONE(ptr, size) \
    coil_memory_arena_clone(coil_global_arena, ptr, size)

#endif /* COIL_MEMORY_H */