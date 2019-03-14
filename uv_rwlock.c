/* uv_rwlock.c - Read/write locks with libuv (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

static uv_barrier_t blocker;
static uv_rwlock_t numlock;
static int shared_num;

void reader(void *args) {
    int num = *((int*)args);
    for (int i = 0; i < 20; ++i) {
        uv_rwlock_rdlock(&numlock);
        printf("Reader %d acquired read lock...\n", num);
        printf("Reader %d: shared num = %d...\n", num, shared_num);
        uv_rwlock_rdunlock(&numlock);
        printf("Reader %d released read lock...\n", num);
    }
    uv_barrier_wait(&blocker);
}

void writer(void *args) {
    int num = *((int*)args);
    for (int i = 0; i < 20; ++i) {
        uv_rwlock_wrlock(&numlock);
        printf("Writer %d acquired lock\n", num);
        shared_num += 16;
        printf("writer %d incremented shared_num...\n", num);
        uv_rwlock_wrunlock(&numlock);
        printf("writer %d released lock...\n", num);
    }
    uv_barrier_wait(&blocker);
}

int main(void) {
    uv_barrier_init(&blocker, 4);

    shared_num = 0;

    uv_rwlock_init(&numlock);

    uv_thread_t threads[3];

    int thread_nums[] = {1, 2, 1};

    uv_thread_create(&threads[0], reader, &thread_nums[0]);
    uv_thread_create(&threads[1], reader, &thread_nums[1]);

    uv_thread_create(&threads[0], writer, &thread_nums[0]);

    uv_barrier_wait(&blocker);
    uv_barrier_destroy(&blocker);

    return EXIT_SUCCESS;
}
