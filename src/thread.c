/* src/thread.c */
#include "coil/thread.h"
#include <stdlib.h>
#include <string.h>

/* Thread-specific data key */
pthread_key_t coil_thread_key;

/* Thread pool */
struct coil_thread_pool {
    pthread_t *threads;                /* Array of threads */
    size_t num_threads;                /* Number of threads */
    
    coil_thread_task_t **tasks;        /* Array of tasks */
    size_t num_tasks;                  /* Number of tasks */
    size_t tasks_capacity;             /* Capacity of tasks array */
    size_t next_task;                  /* Next task to be executed */
    
    pthread_mutex_t lock;              /* Mutex for thread-safety */
    pthread_cond_t cond;               /* Condition variable for signaling */
    pthread_cond_t wait_cond;          /* Condition variable for waiting */
    
    bool running;                      /* Whether the pool is running */
    
    coil_logger_t *logger;             /* Logger */
    coil_error_manager_t *error_mgr;   /* Error manager */
    
    size_t arena_size;                 /* Size of per-thread arenas */
};

/* Thread task implementation */
coil_thread_task_t *coil_thread_task_create(coil_thread_func_t func, void *arg) {
    coil_thread_task_t *task = (coil_thread_task_t *)malloc(sizeof(coil_thread_task_t));
    if (!task) return NULL;
    
    task->func = func;
    task->arg = arg;
    task->result = NULL;
    task->completed = false;
    
    if (pthread_mutex_init(&task->lock, NULL) != 0) {
        free(task);
        return NULL;
    }
    
    if (pthread_cond_init(&task->cond, NULL) != 0) {
        pthread_mutex_destroy(&task->lock);
        free(task);
        return NULL;
    }
    
    return task;
}

void *coil_thread_task_wait(coil_thread_task_t *task) {
    if (!task) return NULL;
    
    pthread_mutex_lock(&task->lock);
    
    while (!task->completed) {
        pthread_cond_wait(&task->cond, &task->lock);
    }
    
    void *result = task->result;
    
    pthread_mutex_unlock(&task->lock);
    
    return result;
}

static void coil_thread_task_destroy(coil_thread_task_t *task) {
    if (!task) return;
    
    pthread_mutex_destroy(&task->lock);
    pthread_cond_destroy(&task->cond);
    free(task);
}

/* Thread-specific data implementation */
static void coil_thread_data_destroy(void *data) {
    if (!data) return;
    
    coil_thread_data_t *thread_data = (coil_thread_data_t *)data;
    
    /* Clean up any thread-specific resources */
    /* Note: we don't destroy the arena, logger, or error manager here
     * because they might be shared with other threads or the main thread */
    
    free(thread_data);
}

coil_thread_data_t *coil_thread_get_data(void) {
    return (coil_thread_data_t *)pthread_getspecific(coil_thread_key);
}

int coil_thread_init_data(coil_memory_arena_t *arena,
                         coil_logger_t *logger,
                         coil_error_manager_t *error_mgr,
                         void *user_data) {
    coil_thread_data_t *data = (coil_thread_data_t *)malloc(sizeof(coil_thread_data_t));
    if (!data) return -1;
    
    data->arena = arena;
    data->logger = logger;
    data->error_mgr = error_mgr;
    data->user_data = user_data;
    
    if (pthread_setspecific(coil_thread_key, data) != 0) {
        free(data);
        return -1;
    }
    
    return 0;
}

int coil_thread_init(void) {
    if (pthread_key_create(&coil_thread_key, coil_thread_data_destroy) != 0) {
        return -1;
    }
    
    return 0;
}

void coil_thread_cleanup(void) {
    pthread_key_delete(coil_thread_key);
}

/* Thread pool worker function */
static void *coil_thread_pool_worker(void *arg) {
    coil_thread_pool_t *pool = (coil_thread_pool_t *)arg;
    
    /* Create a thread-specific arena */
    coil_memory_arena_t *arena = coil_memory_arena_create(
        "thread", pool->arena_size, false, pool->logger, pool->error_mgr);
    
    /* Initialize thread-specific data */
    coil_thread_init_data(arena, pool->logger, pool->error_mgr, NULL);
    
    while (true) {
        pthread_mutex_lock(&pool->lock);
        
        /* Wait for a task or shutdown */
        while (pool->next_task >= pool->num_tasks && pool->running) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        
        /* Check if we should exit */
        if (!pool->running && pool->next_task >= pool->num_tasks) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        
        /* Get the next task */
        coil_thread_task_t *task = pool->tasks[pool->next_task++];
        
        /* Signal if all tasks have been assigned */
        if (pool->next_task >= pool->num_tasks) {
            pthread_cond_broadcast(&pool->wait_cond);
        }
        
        pthread_mutex_unlock(&pool->lock);
        
        /* Execute the task */
        void *result = task->func(task->arg);
        
        /* Mark the task as completed */
        pthread_mutex_lock(&task->lock);
        task->result = result;
        task->completed = true;
        pthread_cond_broadcast(&task->cond);
        pthread_mutex_unlock(&task->lock);
    }
    
    /* Clean up thread-specific data */
    coil_thread_data_t *data = coil_thread_get_data();
    if (data && data->arena) {
        coil_memory_arena_destroy(data->arena);
    }
    
    return NULL;
}

coil_thread_pool_t *coil_thread_pool_create(size_t num_threads,
                                          size_t arena_size_per_thread,
                                          coil_logger_t *logger,
                                          coil_error_manager_t *error_mgr) {
    if (num_threads == 0) {
        /* Use the number of CPU cores as the default */
        num_threads = 4;  /* A reasonable default if we can't determine */
    }
    
    coil_thread_pool_t *pool = (coil_thread_pool_t *)malloc(sizeof(coil_thread_pool_t));
    if (!pool) return NULL;
    
    /* Initialize pool members */
    pool->num_threads = num_threads;
    pool->threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    
    if (!pool->threads) {
        free(pool);
        return NULL;
    }
    
    pool->tasks = NULL;
    pool->num_tasks = 0;
    pool->tasks_capacity = 0;
    pool->next_task = 0;
    pool->running = true;
    pool->logger = logger ? logger : coil_default_logger;
    pool->error_mgr = error_mgr ? error_mgr : coil_default_error_manager;
    pool->arena_size = arena_size_per_thread > 0 ? 
        arena_size_per_thread : 1024 * 1024;  /* 1MB default */
    
    /* Initialize synchronization */
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->cond, NULL) != 0) {
        pthread_mutex_destroy(&pool->lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->wait_cond, NULL) != 0) {
        pthread_cond_destroy(&pool->cond);
        pthread_mutex_destroy(&pool->lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    /* Create worker threads */
    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, coil_thread_pool_worker, pool) != 0) {
            /* Clean up and fail */
            pool->running = false;
            pthread_cond_broadcast(&pool->cond);
            
            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            
            pthread_cond_destroy(&pool->wait_cond);
            pthread_cond_destroy(&pool->cond);
            pthread_mutex_destroy(&pool->lock);
            free(pool->threads);
            free(pool);
            
            return NULL;
        }
    }
    
    return pool;
}

static int coil_thread_pool_ensure_capacity(coil_thread_pool_t *pool) {
    if (!pool) return -1;
    
    /* Initial capacity or double when needed */
    size_t new_capacity = pool->tasks_capacity == 0 ? 16 : pool->tasks_capacity * 2;
    
    coil_thread_task_t **new_tasks = (coil_thread_task_t **)realloc(
        pool->tasks, new_capacity * sizeof(coil_thread_task_t *));
    
    if (!new_tasks) return -1;
    
    pool->tasks = new_tasks;
    pool->tasks_capacity = new_capacity;
    
    return 0;
}

coil_thread_task_t *coil_thread_pool_submit(coil_thread_pool_t *pool,
                                          coil_thread_func_t func,
                                          void *arg) {
    if (!pool || !func) return NULL;
    
    /* Create the task */
    coil_thread_task_t *task = coil_thread_task_create(func, arg);
    if (!task) return NULL;
    
    pthread_mutex_lock(&pool->lock);
    
    /* Check if we need more capacity */
    if (pool->num_tasks >= pool->tasks_capacity) {
        if (coil_thread_pool_ensure_capacity(pool) != 0) {
            pthread_mutex_unlock(&pool->lock);
            coil_thread_task_destroy(task);
            return NULL;
        }
    }
    
    /* Add the task to the pool */
    pool->tasks[pool->num_tasks++] = task;
    
    /* Signal that a new task is available */
    pthread_cond_signal(&pool->cond);
    
    pthread_mutex_unlock(&pool->lock);
    
    return task;
}

void coil_thread_pool_wait_all(coil_thread_pool_t *pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->lock);
    
    /* Wait until all tasks have been assigned */
    while (pool->next_task < pool->num_tasks) {
        pthread_cond_wait(&pool->wait_cond, &pool->lock);
    }
    
    /* Wait for all tasks to complete */
    for (size_t i = 0; i < pool->num_tasks; i++) {
        coil_thread_task_t *task = pool->tasks[i];
        
        pthread_mutex_unlock(&pool->lock);
        coil_thread_task_wait(task);
        pthread_mutex_lock(&pool->lock);
    }
    
    /* Reset the task queue */
    for (size_t i = 0; i < pool->num_tasks; i++) {
        coil_thread_task_destroy(pool->tasks[i]);
        pool->tasks[i] = NULL;
    }
    
    pool->num_tasks = 0;
    pool->next_task = 0;
    
    pthread_mutex_unlock(&pool->lock);
}

void coil_thread_pool_destroy(coil_thread_pool_t *pool) {
    if (!pool) return;
    
    /* Signal threads to exit */
    pthread_mutex_lock(&pool->lock);
    pool->running = false;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
    
    /* Wait for threads to exit */
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    /* Clean up any remaining tasks */
    for (size_t i = 0; i < pool->num_tasks; i++) {
        coil_thread_task_destroy(pool->tasks[i]);
    }
    
    /* Clean up resources */
    pthread_cond_destroy(&pool->wait_cond);
    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->lock);
    
    free(pool->tasks);
    free(pool->threads);
    free(pool);
}

pthread_mutex_t *coil_mutex_create(void) {
    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (!mutex) return NULL;
    
    if (pthread_mutex_init(mutex, NULL) != 0) {
        free(mutex);
        return NULL;
    }
    
    return mutex;
}

void coil_mutex_destroy(pthread_mutex_t *mutex) {
    if (!mutex) return;
    
    pthread_mutex_destroy(mutex);
    free(mutex);
}

pthread_cond_t *coil_cond_create(void) {
    pthread_cond_t *cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    if (!cond) return NULL;
    
    if (pthread_cond_init(cond, NULL) != 0) {
        free(cond);
        return NULL;
    }
    
    return cond;
}

void coil_cond_destroy(pthread_cond_t *cond) {
    if (!cond) return;
    
    pthread_cond_destroy(cond);
    free(cond);
}

static coil_memory_arena_t *coil_thread_arena_getter_impl(void) {
    coil_thread_data_t *data = coil_thread_get_data();
    return data ? data->arena : NULL;
}

coil_memory_arena_t *coil_thread_create_arena(size_t size) {
    /* Create a new arena for the current thread */
    coil_memory_arena_t *arena = coil_memory_arena_create(
        "thread", size, false, coil_default_logger, coil_default_error_manager);
    
    if (arena) {
        /* Set it as the current thread's arena */
        coil_thread_set_arena(arena);
        
        /* Set it as the thread arena getter */
        coil_memory_set_thread_arena_getter(coil_thread_arena_getter_impl);
    }
    
    return arena;
}

coil_memory_arena_t *coil_thread_get_arena(void) {
    coil_thread_data_t *data = coil_thread_get_data();
    return data ? data->arena : NULL;
}

void coil_thread_set_arena(coil_memory_arena_t *arena) {
    coil_thread_data_t *data = coil_thread_get_data();
    
    if (data) {
        data->arena = arena;
    } else {
        /* Create thread data if it doesn't exist */
        coil_thread_init_data(arena, NULL, NULL, NULL);
    }
}

coil_logger_t *coil_thread_get_logger(void) {
    coil_thread_data_t *data = coil_thread_get_data();
    return data ? data->logger : NULL;
}

void coil_thread_set_logger(coil_logger_t *logger) {
    coil_thread_data_t *data = coil_thread_get_data();
    
    if (data) {
        data->logger = logger;
    } else {
        /* Create thread data if it doesn't exist */
        coil_thread_init_data(NULL, logger, NULL, NULL);
    }
}

coil_error_manager_t *coil_thread_get_error_mgr(void) {
    coil_thread_data_t *data = coil_thread_get_data();
    return data ? data->error_mgr : NULL;
}

void coil_thread_set_error_mgr(coil_error_manager_t *error_mgr) {
    coil_thread_data_t *data = coil_thread_get_data();
    
    if (data) {
        data->error_mgr = error_mgr;
    } else {
        /* Create thread data if it doesn't exist */
        coil_thread_init_data(NULL, NULL, error_mgr, NULL);
    }
}

void *coil_thread_get_user_data(void) {
    coil_thread_data_t *data = coil_thread_get_data();
    return data ? data->user_data : NULL;
}

void coil_thread_set_user_data(void *user_data) {
    coil_thread_data_t *data = coil_thread_get_data();
    
    if (data) {
        data->user_data = user_data;
    } else {
        /* Create thread data if it doesn't exist */
        coil_thread_init_data(NULL, NULL, NULL, user_data);
    }
}