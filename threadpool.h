#include <pthread.h>
#include <stdlib.h>

#define NO_SHUTDOWN     0
#define DONT_END_TASKS  1
#define END_TASKS       2

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
    int index;

    pthread_cond_t  notify;
    pthread_mutex_t mutex;

    pthread_t *threads;
} threadpool;

void destroy_threadpool(threadpool *pool) {
    if (pool == NULL)
        return;
    
    pthread_cond_broadcast(&pool->notify);

    pthread_cond_destroy(&pool->notify);
    pthread_mutex_destroy(&pool->mutex);

    for(int i = 0; i < pool->n_threads; i++)
        pthread_join(pool->threads[i], NULL);

    free(pool->threads);
    free(pool->tasks);
}

static void *execute_tasks(void *void_pool) {
    threadpool *pool = (threadpool *)void_pool;
    threadpool_task task;

    while (true) {
        pthread_mutex_lock(&pool->mutex);

        while((pool->tasks_count == 0) && (!pool->shutdown))
            pthread_cond_wait(&pool->notify, &pool->mutex);

        if ((pool->shutdown == DONT_END_TASKS) ||
            (pool->shutdown == END_TASKS && pool->tasks_count == 0))
            break;

        task.function = pool->tasks[pool->index].function;
        task.arguments = pool->tasks[pool->index].arguments;
        
        pool->index = (pool->index + 1) % pool->queue_size;
        pool->tasks_count--;

        pthread_mutex_unlock(&pool->mutex);
        
        (*task.function)(task.arguments);
    }

    pthread_exit(NULL);
    return NULL;
}

threadpool *inc_tasksqueue_size(threadpool *pool, int inc) {

    pthread_mutex_lock(&pool->mutex);
    pool->tasks = (threadpool_task *)realloc(pool->tasks, (pool->queue_size + inc) * sizeof(threadpool_task));
    pthread_mutex_unlock(&pool->mutex);

    return pool;
}

threadpool *inc_threadpool_size(threadpool *pool, int inc) {

    pthread_mutex_lock(&pool->mutex);
    pool->threads = (pthread_t *)realloc(pool->threads, (pool->n_threads + inc) * sizeof(pthread_t));
    pthread_mutex_unlock(&pool->mutex);

    return pool;
}

threadpool *new_threadpool(int n_threads, int queue_size = 0) {
    
    if (n_threads <= 0)
        return NULL;

    if (queue_size <= 0)
        queue_size = n_threads;
    
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));

    pool->shutdown      = NO_SHUTDOWN;

    pool->n_threads     = n_threads;
    pool->queue_size    = queue_size;
    pool->tasks_count   = 0;

    pool->tasks = (threadpool_task *)malloc(queue_size * sizeof(threadpool_task));
    pool->index = 0;

    pool->threads   = (pthread_t *)malloc(n_threads * sizeof(n_threads));
    for (int i = 0; i < n_threads; i++) 
        pthread_create(pool->threads + i, NULL, 
            execute_tasks, (void*)pool);

    pthread_cond_init(&pool->notify, NULL);
    pthread_mutex_init(&pool->mutex, NULL);

    return pool;
}

threadpool *submit_task(threadpool *pool, void (*function)(void *), void *arguments) {
    pthread_mutex_lock(&pool->mutex);

    pool->tasks[pool->tasks_count] = threadpool_task{function, arguments};
    pool->tasks_count++;

    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_signal(&pool->notify);
}

threadpool *submit_task(threadpool *pool, threadpool_task task) {

    pthread_mutex_lock(&pool->mutex);

    pool->tasks[
        (pool->tasks_count + pool->index) % pool->queue_size
    ] = task;
    
    pool->tasks_count++;

    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_signal(&pool->notify);

    return pool;
}