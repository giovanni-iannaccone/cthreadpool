#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define NO_SHUTDOWN     0
#define END_TASKS       1

typedef struct {
    void (*function)(void *);
    void *arguments;
} threadpool_task;

typedef struct {
    int shutdown;
    int n_threads;
    int queue_size;
    int tasks_count;

    threadpool_task *tasks;
    int head;
    int tail;

    pthread_t *threads;
    pthread_mutex_t mutex;
    pthread_cond_t notify;
    pthread_cond_t can_accept_task;
} threadpool;

void destroy_threadpool(threadpool *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = END_TASKS;
    pthread_mutex_unlock(&pool->mutex);

    pthread_cond_broadcast(&pool->notify);

    for (int i = 0; i < pool->n_threads; i++)
        pthread_join(pool->threads[i], NULL);

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->notify);
    pthread_cond_destroy(&pool->can_accept_task);

    free(pool->threads);
    free(pool->tasks);
    free(pool);
}

static void *execute_tasks(void *void_pool) {
    threadpool *pool = (threadpool *)void_pool;

    while (1) {
        pthread_mutex_lock(&pool->mutex);

        while (pool->tasks_count == 0 && pool->shutdown == NO_SHUTDOWN)
            pthread_cond_wait(&pool->notify, &pool->mutex);

        if (pool->shutdown == END_TASKS && pool->tasks_count == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        threadpool_task task = pool->tasks[pool->head];
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->tasks_count--;

        pthread_cond_signal(&pool->can_accept_task);
        pthread_mutex_unlock(&pool->mutex);

        (*(task.function))(task.arguments);
    }

    pthread_exit(NULL);
}

threadpool *new_threadpool(int n_threads, int queue_size) {
    threadpool *pool = malloc(sizeof(threadpool));
    if (!pool) return NULL;

    pool->shutdown = NO_SHUTDOWN;
    pool->n_threads = n_threads;
    pool->queue_size = queue_size;
    pool->tasks_count = 0;
    pool->head = 0;
    pool->tail = 0;

    pool->tasks = malloc(queue_size * sizeof(threadpool_task));
    pool->threads = malloc(n_threads * sizeof(pthread_t));

    pthread_mutex_init(&pool->mutex, NULL);

    pthread_cond_init(&pool->notify, NULL);
    pthread_cond_init(&pool->can_accept_task, NULL);

    for (int i = 0; i < n_threads; i++)
        pthread_create(&pool->threads[i], NULL, execute_tasks, pool);

    return pool;
}

threadpool *submit_task(threadpool *pool, void (*function)(void *), void *arguments) {
    if (!pool || !function || pool->shutdown != NO_SHUTDOWN)
        return NULL;

    pthread_mutex_lock(&pool->mutex);

    while (pool->tasks_count >= pool->queue_size)
        pthread_cond_wait(&pool->can_accept_task, &pool->mutex);

    pool->tasks[pool->tail] = (threadpool_task){function, arguments};

    pool->tail = (pool->tail + 1) % pool->queue_size;
    pool->tasks_count++;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->mutex);

    return pool;
}

#ifdef __cplusplus
}
#endif
