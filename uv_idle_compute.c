/* uv_idle_compute.c - See libuv documentation (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

static uv_loop_t *loop;
static uv_idle_t idler;

static uv_fs_t stdin_watcher;
static uv_fs_t stdout_watcher;

static char stdin_buffer[1024];
static char stdout_buffer[1024];

static uv_buf_t stdin_buf;
static uv_buf_t stdout_buf;

void on_type(uv_fs_t *req);

void crunch_away(uv_idle_t *handle) {
    // some heavy computation here.
    fprintf(stdout, "Computing PI....\n");
    uv_idle_stop(handle);
}

void on_write(uv_fs_t *req) {
    fputs("in on_write()\n", stderr);
    if (stdout_watcher.result > 0) {
        /* Syncrhonous write to stderr. */
        fprintf(stderr, "wrote: \"%s\"\n", stdout_buffer);

    } else if (stdout_watcher.result < 0) {
        fprintf(stderr, "error opening stdout %s\n", uv_strerror(req->result));
    }
}

void on_type(uv_fs_t *req) {
    fprintf(stderr, "in on_type()\n");

    if (stdin_watcher.result > 0) {
        stdin_buffer[stdin_watcher.result] = '\0';
        memset(stdout_buffer, 0, sizeof(stdout_buffer));
        sprintf(stdout_buffer, "typed: \"%s\"\n", stdin_buffer);

        stdout_buf = uv_buf_init(stdout_buffer, strlen(stdout_buffer));
        uv_fs_write(loop, &stdout_watcher, STDOUT_FILENO, &stdout_buf, 1, -1, on_write);

        stdin_buf = uv_buf_init(stdin_buffer, 1024);
        uv_fs_read(loop, &stdin_watcher, STDIN_FILENO, &stdin_buf, 1, -1, on_type);
        uv_idle_start(&idler, crunch_away);
    } else if (stdin_watcher.result < 0) {
        fprintf(stderr, "error opening stdin %s\n", uv_strerror(req->result));
    }
}

int main(void) {
    loop = uv_default_loop();

    uv_idle_init(loop, &idler);
    stdin_buf = uv_buf_init(stdin_buffer, 1024);

    uv_fs_read(loop, &stdin_watcher, STDIN_FILENO, &stdin_buf, 1, -1, on_type);

    uv_idle_start(&idler, crunch_away);

    return uv_run(loop, UV_RUN_DEFAULT);
}
