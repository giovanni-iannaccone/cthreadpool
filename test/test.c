#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../cthreadpool.h"

#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define RESET   "\033[0m"

#define N_THREADS       4
#define QUEUE_SIZE      8
#define TEST_TASK_COUNT 16

#define TOTAL_TRY 10

pthread_mutex_t m;
int executed_times = 0;

void empty_task(void *nil) {
    int a = 5;
    a++;
}

double benchmark_measure_speed(int threads, int queue_size, int tasks) {
    tp_pool_t *pool = tp_new_pool(threads, queue_size);
    clock_t start, end;
    int i;

    start = clock();

    for (i = 0; i < tasks; i++)
        tp_submit_task(pool, empty_task, NULL);

    tp_destroy_pool(pool);
    end = clock();

    return (double)(end - start) / CLOCKS_PER_SEC;
}

void benchmark_throughput(int threads, int queue_size, int tasks) {
    double total_duration = 0;
    int i;
    
    for (i = 0; i < TOTAL_TRY; i++)
        total_duration += benchmark_measure_speed(threads, queue_size, tasks);

    printf("%sBenchmark: %d threads, %d queue, %d tasks => %.2fs %s\n", GREEN, threads, queue_size, tasks, total_duration / TOTAL_TRY, RESET);
}

void task_mark_execution(void *args) {
    pthread_mutex_lock(&m);
    executed_times++;
    pthread_mutex_unlock(&m);

    sleep(1);
}

int test_basic_task_execution() {
    tp_pool_t *pool = tp_new_pool(N_THREADS, QUEUE_SIZE);
    int i;
    
    for (i = 0; i < TEST_TASK_COUNT; i++)
        tp_submit_task(pool, task_mark_execution, NULL);
    
    tp_destroy_pool(pool);
    printf("%d\n", executed_times);
    return executed_times == TEST_TASK_COUNT;        
}

void run_benchmarks() {
    printf("Running benchmarks...\n");
    benchmark_throughput(4, 16, 65536);
}

void run_unit_tests() {
    printf("Running unit tests...\n");
    if (test_basic_task_execution())
        printf("%s[ Unit tests passed ]%s\n\n", GREEN, RESET);
    else
        printf("%s[ Unit tests failed ]%s\n\n", RED, RESET);
}

int main() {
    pthread_mutex_init(&m, NULL);
    run_unit_tests();
    pthread_mutex_destroy(&m);
    
    run_benchmarks();
    
    return 0;
}
