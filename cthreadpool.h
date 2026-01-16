#ifndef _C_THREADPOOL_H_
#define _C_THREADPOOL_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG

#include <stdarg.h>
#include <sys/syscall.h>
#include <time.h>

#define gettid() syscall(SYS_gettid)
#define _BLUE_  "\033[34m"
#define _RESET_ "\033[0m"

#endif

#define _TP_RUNNING_  0
#define _TP_SHUTDOWN_ 1

typedef struct tp_task {
    void (*function)(void *);
    void *arguments;
} tp_task_t;

typedef struct tp_pool {
    int status;
    
    size_t n_threads;
    size_t queue_size;
    size_t tasks_count;

    size_t head;
    size_t tail;
    struct tp_task *tasks;
    
    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_cond_t can_accept_task;
    pthread_cond_t got_new_task;
} tp_pool_t;

#ifdef DEBUG
void __dbg_printf(const char *fmt, ...) {
    va_list args;
    char *s;
    
    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);

    va_start(args, fmt);

    s = (char *)malloc(128 * sizeof(char));
    
    vsnprintf(s, 128, fmt, args);
    printf("%s[DEBUG %d:%d:%d] %s %s\n", _BLUE_, t.tm_hour, t.tm_min, t.tm_sec, _RESET_, s);

    free(s);
    va_end(args);
}

void __print_queue_status(struct tp_task *queue, size_t queue_size) {
    size_t i;
    printf("TASKS QUEUE\n");
    
    for (i = 0; i < queue_size; i++)
        printf("| idx: %ld function: %p | -> ", i, queue[i].function);

    printf("\n");
}
#endif

void __free_pool(struct tp_pool *pool) {
    pthread_mutex_destroy(&pool->mutex);

    pthread_cond_destroy(&pool->got_new_task);
    pthread_cond_destroy(&pool->can_accept_task);

    free(pool->threads);
    free(pool->tasks);
    free(pool);
}

void __initialize_pool(struct tp_pool *pool, size_t n_threads, size_t queue_size, size_t tasks_count) {
    pool->status = _TP_RUNNING_;
    pool->n_threads = n_threads;
    pool->queue_size = queue_size;

    pool->head = 0;
    pool->tail = 0;
    pool->tasks_count = tasks_count;
    
    pool->tasks = NULL;
    pool->threads = NULL;

    pthread_mutex_init(&pool->mutex, NULL);

    pthread_cond_init(&pool->got_new_task, NULL);
    pthread_cond_init(&pool->can_accept_task, NULL);
}

void *__worker(void *p) {
    struct tp_pool *pool = (struct tp_pool *)p;
    struct tp_task task;
    
    while (1) {

        pthread_mutex_lock(&pool->mutex);
        while (pool->tasks_count == 0 && pool->status == _TP_RUNNING_) {
#ifdef DEBUG
            __dbg_printf("Thread %d waiting for task to execute", gettid());
#endif
            pthread_cond_wait(&pool->got_new_task, &pool->mutex);
        }

        if (pool->status == _TP_SHUTDOWN_ && pool->tasks_count == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        task = pool->tasks[pool->head];
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->tasks_count--;
        
#ifdef DEBUG
        __dbg_printf("Executing task %p at index %ld with thread %d", task.function, pool->head, gettid());
#endif
        
        pthread_cond_signal(&pool->can_accept_task);
        pthread_mutex_unlock(&pool->mutex);

        task.function(task.arguments);
        
#ifdef DEBUG
        __dbg_printf("Executed task %p with thread %d", task.function, gettid());
#endif
    }

#ifdef DEBUG
    __dbg_printf("Thread %d terminated", gettid());
#endif
    
    pthread_exit(NULL);
}

void tp_destroy_pool(struct tp_pool *pool) {
    size_t i;
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);

    pool->status = _TP_SHUTDOWN_;

    pthread_cond_broadcast(&pool->got_new_task);
    pthread_cond_broadcast(&pool->can_accept_task);

#ifdef DEBUG
    __print_queue_status(pool->tasks, pool->queue_size);
#endif
    pthread_mutex_unlock(&pool->mutex);

    for (i = 0; i < pool->n_threads; i++)
        pthread_join(pool->threads[i], NULL);

    __free_pool(pool);
}

struct tp_pool *tp_new_pool(size_t n_threads, size_t queue_size) {
    struct tp_pool *pool = (struct tp_pool*)malloc(sizeof(*pool));
    size_t i;

    if (!pool) return NULL;

    __initialize_pool(pool, n_threads, queue_size, 0);

    pool->tasks = (struct tp_task *)calloc(queue_size, sizeof(struct tp_task));
    pool->threads = (pthread_t *)malloc(n_threads * sizeof(pthread_t));

    for (i = 0; i < n_threads; i++) {
        pthread_create(pool->threads + i, NULL, __worker, pool);
#ifdef DEBUG
        __dbg_printf("New thread created");
#endif
    }
    
#ifdef DEBUG
    __dbg_printf("Pool successfully created: queue_size=%d n_threads=%d", pool->queue_size, pool->n_threads);
#endif
    
    return pool;
}

struct tp_pool *tp_change_pool_size(struct tp_pool *pool, size_t n_threads, size_t queue_size) {
    struct tp_pool *new_pool = tp_new_pool(n_threads, queue_size);

    if (!new_pool)
        return pool;

    tp_destroy_pool(pool);
    return new_pool;
}

int tp_submit_task(struct tp_pool *pool, void (*function)(void *), void *arguments) {
    size_t idx = 0;
    if (!pool || !function)
        return -1;
    
    pthread_mutex_lock(&pool->mutex);
    
#ifdef DEBUG
    __dbg_printf("Submitting new task %p", function);
#endif
    
    while (pool->tasks_count >= pool->queue_size) {
        if (pool->status == _TP_SHUTDOWN_)
            break;
        pthread_cond_wait(&pool->can_accept_task, &pool->mutex);
    }

    pool->tasks[pool->tail] = (struct tp_task){function, arguments};

    pool->tail = (pool->tail + 1) % pool->queue_size;
    pool->tasks_count++;
    
    pthread_cond_signal(&pool->got_new_task);
    pthread_mutex_unlock(&pool->mutex);

#ifdef DEBUG
    __print_queue_status(pool->tasks, pool->queue_size);
    __dbg_printf("Submitted new task %p at index %ld", function, idx);
#endif
    return 0;
}

#endif
