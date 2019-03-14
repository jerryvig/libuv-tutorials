/* uv_work_queue.c - Work queue example with libuv (https://libuv.org).
 *
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#define FIB_UNTIL 16

static uv_loop_t *loop;

long fib_(long t) {
    if (t == 0 || t == 1) {
        return 1;
    }
    return fib_(t - 1) + fib_(t - 2);
}

void after_fib(uv_work_t *work_unit, int status) {
    if (status == 0) {
        int n = *((int *) work_unit->data);
        fprintf(stderr, "Done computing %dth fibonacci number.\n", n);
    }
}

void fib(uv_work_t *work_unit) {
    int n = *((int*)work_unit->data);

    if (random() % 2) {
        sleep(1);
    } else {
        sleep(2);
    }
    long fib = fib_(n);
    fprintf(stderr, "%dth fibonacci is %lu\n", n, fib);
}

int main(void) {
    loop = uv_default_loop();

    int data[FIB_UNTIL];
    uv_work_t work_units[FIB_UNTIL];

    for (int i = 0; i < FIB_UNTIL; ++i) {
        data[i] = i;
        work_units[i].data = (void*)&data[i];
        uv_queue_work(loop, &work_units[i], fib, after_fib);
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
