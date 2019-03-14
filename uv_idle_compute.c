/* uv_idle_compute.c - See libuv documentation (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <uv.h>

static uv_loop_t *loop;
static uv_fs_t stdin_watcher;
static uv_idle_t idler;
static char buffer[1024];

void crunch_away(uv_idle_t *handle) {
    // some heavy computation here.
    fprintf(stderr, "Computing PI....\n");

    // just to avoid overwhelming your terminal.
    uv_idle_stop(handle);
}

void on_type(uv_fs_t *req) {
    if (stdin_watcher.result > 0) {
        buffer[stdin_watcher.result] = '\0';
        printf("Typed %s\n", buffer);

        uv_buf_t buf = uv_buf_init(buffer, 1024);
        uv_fs_read(loop, &stdin_watcher, 0, &buf, 1, -1, on_type);
        uv_idle_start(&idler, crunch_away);
    } else if (stdin_watcher.result < 0) {
        fprintf(stderr, "error opening file %s\n", uv_strerror(req->result));
    }
}

int main(void) {
    loop = uv_default_loop();

    uv_idle_init(loop, &idler);
    uv_buf_t buf = uv_buf_init(buffer, 1024);
    uv_fs_read(loop, &stdin_watcher, 0, &buf, 1, -1, on_type);
    uv_idle_start(&idler, crunch_away);

    return uv_run(loop, UV_RUN_DEFAULT);
}
