/* uv_timer.c - An example of using libuv timers (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <uv.h>

#define TIMEOUT_MS 4000
#define REPEAT_MS 2000

static uv_loop_t *loop;
static uv_timer_t timer_watcher;

void timer_callback() {
    printf("in the timer callback....\n");
}

int main(void) {
    loop = uv_default_loop();

    uv_timer_init(loop, &timer_watcher);
    uv_timer_start(&timer_watcher, timer_callback, TIMEOUT_MS, REPEAT_MS);

    return uv_run(loop, UV_RUN_DEFAULT);
}
