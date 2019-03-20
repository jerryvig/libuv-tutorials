/* uv_thread_pool.c - Thread pool example in C with libuv (https://libuv.org).
 * The source code for the libuv threadpool is at https://github.com/libuv/libuv/blob/v1.x/src/threadpool.c
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <uv.h>

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define THREAD_POOL_SIZE 4

static uv_loop_t *loop;

void after_work(uv_work_t *work_unit, int status) {
    if (!status) {
        free(work_unit);
    }
}

void do_work(uv_work_t *work_unit) {
    sleep(1);
    int value = *((int*)work_unit->data);
    int sq = value * value;
    printf("%d\n", sq);
}

int main(void) {
    putenv("UV_THREADPOOL_SIZE=" STRINGIFY(THREAD_POOL_SIZE));
    printf("UV_THREADPOOL_SIZE = %s\n", getenv("UV_THREADPOOL_SIZE"));

    loop = uv_default_loop();

    int proc_items[] = { 1, 8, 17, 12, 13, 14, 19, 22, 31, 56, 43, 77 };

    for (int i=0; i<12; ++i) {
        uv_work_t *work_unit = (uv_work_t*)malloc(sizeof(uv_work_t));
        work_unit->data = (void*)&proc_items[i];
        uv_queue_work(loop, work_unit, do_work, after_work);
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
