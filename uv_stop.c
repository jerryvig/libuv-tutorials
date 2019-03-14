/* uv_stop.c - See libuv documentation (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <uv.h>

static uv_loop_t *loop;
static int64_t counter;

void idle_callback(uv_idle_t *handle) {
    printf("Idle callback....\n");
    counter++;

    sleep(1);

    if (counter > 5) {
        uv_stop(loop);
        printf("uv_stop() called....\n");
    }
}

void prep_callback(uv_prepare_t *prep) {
    printf("In prep callback()....\n");
}

int main(void) {
    loop = uv_default_loop();

    uv_idle_t idler;
    uv_prepare_t prep;

    uv_idle_init(loop, &idler);
    uv_idle_start(&idler, idle_callback);

    uv_prepare_init(loop, &prep);
    uv_prepare_start(&prep, prep_callback);

    return uv_run(loop, UV_RUN_DEFAULT);
}
