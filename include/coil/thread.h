/* coil/thread.h */
#ifndef COIL_THREAD_H
#define COIL_THREAD_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "coil/log.h"
#include "coil/err.h"
#include "coil/mem.h"

/* Thread-specific data key */
extern pthread_key_t coil_thread_key;

/* Thread pool */
typedef struct coil_thread_pool coil_thread_pool_t;

/* Thread function */
typedef void *(*coil_thread_func_t)(void *arg);

/* Thread task */
typedef struct {
    coil_thread_func_t func;     /* Thread function */
    void *arg;                   /* Thread argument */
    void *result;                /* Thread result */
    bool completed;              /* Whether the task has completed */
    pthread_mutex_t lock;        /* Mutex for thread-safety */
    pthread_cond_t cond;         /* Condition variable for waiting */
} coil_thread_task_t;

/* Create a thread task */
coil_thread_task_t *coil_thread_task_create(coil_thread_func_t func, void *arg);

/* Wait for a thread task to complete */
void *coil_thread_task_wait(coil_thread_task_t *task);

/* Thread-specific data */
typedef struct {
    coil_memory_arena_t *arena;       /* Thread-specific memory arena */
    coil_logger_t *logger;            /* Thread-specific logger */
    coil_error_manager_t *error_mgr;  /* Thread-specific error manager */
    void *user_data;                  /* User-defined thread-specific data */
} coil_thread_data_t;

/* Get the current thread's data */
coil_thread_data_t *coil_thread_get_data(void);

/* Initialize thread-specific data */
int coil_thread_init_data(coil_memory_arena_t *arena,
                         coil_logger_t *logger,
                         coil_error_manager_t *error_mgr,
                         void *user_data);

/* Initialize the thread system */
int coil_thread_init(void);

/* Clean up the thread system */
void coil_thread_cleanup(void);

/* Create a thread pool */
coil_thread_pool_t *coil_thread_pool_create(size_t num_threads,
                                          size_t arena_size_per_thread,
                                          coil_logger_t *logger,
                                          coil_error_manager_t *error_mgr);

/* Submit a task to a thread pool */
coil_thread_task_t *coil_thread_pool_submit(coil_thread_pool_t *pool,
                                          coil_thread_func_t func,
                                          void *arg);

/* Wait for all tasks in a thread pool to complete */
void coil_thread_pool_wait_all(coil_thread_pool_t *pool);

/* Destroy a thread pool */
void coil_thread_pool_destroy(coil_thread_pool_t *pool);

/* Create a mutex */
pthread_mutex_t *coil_mutex_create(void);

/* Destroy a mutex */
void coil_mutex_destroy(pthread_mutex_t *mutex);

/* Create a condition variable */
pthread_cond_t *coil_cond_create(void);

/* Destroy a condition variable */
void coil_cond_destroy(pthread_cond_t *cond);

/* Create a thread-specific memory arena */
coil_memory_arena_t *coil_thread_create_arena(size_t size);

/* Get the current thread's memory arena */
coil_memory_arena_t *coil_thread_get_arena(void);

/* Set the current thread's memory arena */
void coil_thread_set_arena(coil_memory_arena_t *arena);

/* Get the current thread's logger */
coil_logger_t *coil_thread_get_logger(void);

/* Set the current thread's logger */
void coil_thread_set_logger(coil_logger_t *logger);

/* Get the current thread's error manager */
coil_error_manager_t *coil_thread_get_error_mgr(void);

/* Set the current thread's error manager */
void coil_thread_set_error_mgr(coil_error_manager_t *error_mgr);

/* Get the current thread's user data */
void *coil_thread_get_user_data(void);

/* Set the current thread's user data */
void coil_thread_set_user_data(void *user_data);

#endif /* COIL_THREAD_H */