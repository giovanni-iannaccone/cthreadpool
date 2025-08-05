#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../cthreadpool.h"

#define GREEN   "\033[32m"
#define RESET   "\033[0m"

#define N_THREADS 4
#define QUEUE_SIZE 8
#define TEST_TASK_COUNT 16

int executed_flags[TEST_TASK_COUNT];

void empty_task(void *arg) {

}

void benchmark_throughput(int threads, int queue_size, int tasks) {
    threadpool *pool = new_threadpool(threads, queue_size);
    clock_t start = clock();

    for (int i = 0; i < tasks; i++)
        submit_task(pool, empty_task, NULL);

    destroy_threadpool(pool);
    clock_t end = clock();

    double duration = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Benchmark: %d threads, %d queue, %d tasks => %.2fs\n", threads, queue_size, tasks, duration);
}

void task_mark_execution(void *arg) {
    int index = *(int *)arg;
    executed_flags[index] = 1;
    usleep(10000);
    free(arg);
}

void test_basic_task_execution() {
    threadpool *pool = new_threadpool(N_THREADS, QUEUE_SIZE);
    for (int i = 0; i < TEST_TASK_COUNT; i++) {
        int *index = malloc(sizeof(int));
        *index = i;
        executed_flags[i] = 0;
        submit_task(pool, task_mark_execution, index);
    }
    
    destroy_threadpool(pool);
    for (int i = 0; i < TEST_TASK_COUNT; i++)
        assert(executed_flags[i] == 1);
}

void run_benchmarks() {
    printf("Running benchmarks...\n");
    benchmark_throughput(4, 16, 65536);
}

void run_unit_tests() {
    printf("Running unit tests...\n");
    test_basic_task_execution();
    printf("%s[ Unit tests passed ]%s\n\n", GREEN, RESET);
}

int main() {
    run_unit_tests();
    run_benchmarks();

    return 0;
}
