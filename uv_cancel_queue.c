/* uv_cancel_queue.c - See libuv documentation (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#define FIB_UNTIL 16

static uv_loop_t *loop;
static uv_work_t work_units[FIB_UNTIL];

long fib_(long t) {
    if (t == 0 || t == 1) {
        return 1;
    } else {
        return fib_(t - 1) + fib_(t - 2);
    }
}

void after_fib(uv_work_t *work_unit, int status) {
    if (status == UV_ECANCELED) {
        fprintf(stderr, "Calculation of %d cancelled.\n", *(int*)work_unit->data);
    }
}

void fib(uv_work_t *work_unit) {
    int n = *((int*)work_unit->data);
    if (random() % 2) {
        sleep(1);
    } else {
        sleep(3);
    }
    long fib = fib_(n);
    fprintf(stderr, "%dth fibonacci is %lu\n", n, fib);
}

void signal_handler(uv_signal_t *req, int signum) {
    printf("===== IN THE SIGNAL HANDLER MOTHER FUCKER ======\n");
    printf("Signal received...\n");
    for (int i = 0; i < FIB_UNTIL; ++i) {
        uv_cancel((uv_req_t*)&work_units[i]);
    }
    uv_signal_stop(req);
}

int main(void) {
    loop = uv_default_loop();

    int data[FIB_UNTIL];
    for (int i = 0; i < FIB_UNTIL; ++i) {
        data[i] = i;
        work_units[i].data = (void*)&data[i];
        uv_queue_work(loop, &work_units[i], fib, after_fib);
    }

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, signal_handler, SIGINT);

    return uv_run(loop, UV_RUN_DEFAULT);
}
