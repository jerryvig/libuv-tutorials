#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define BUFFER_SIZE 256

static uv_loop_t *loop;
static uv_fs_t stdin_watcher;
static uv_fs_t stdout_watcher;
static char stdin_buffer[BUFFER_SIZE];
static char *prompt = "Enter input data here: ";

static void on_stdin_read(uv_fs_t *req);

static void on_sigint(uv_signal_t *sig, int signum) {
    uv_signal_stop(sig);
    uv_close((uv_handle_t*)sig, NULL);
    uv_fs_req_cleanup(&stdin_watcher);
    uv_fs_req_cleanup(&stdout_watcher);
    uv_kill(getpid(), SIGTERM);
}

static void init_watchers() {
    memset(stdin_buffer, 0, BUFFER_SIZE);
    uv_buf_t stdin_buf = uv_buf_init(stdin_buffer, BUFFER_SIZE);
    uv_buf_t prompt_buf = uv_buf_init(prompt, strlen(prompt));

    uv_fs_write(loop, &stdout_watcher, STDOUT_FILENO, &prompt_buf, 1, -1, NULL);
    uv_fs_read(loop, &stdin_watcher, STDIN_FILENO, &stdin_buf, 1, -1, on_stdin_read);
}

static void on_stdout_write() {
    init_watchers();
}

static void on_stdin_read(uv_fs_t *req) {
    if (stdin_watcher.result > 0) {
        char echo_string[BUFFER_SIZE] = {'\0'};
        size_t stdin_len = strlen(stdin_buffer);
        stdin_buffer[stdin_len - 1] = '\0';
        sprintf(echo_string, "You entered \"%s\"\n", stdin_buffer);

        uv_buf_t echo_buf = uv_buf_init(echo_string, strlen(echo_string));
        uv_fs_write(loop, &stdout_watcher, STDOUT_FILENO, &echo_buf, 1, -1, on_stdout_write);
    } else if (stdin_watcher.result < 0) {
        fprintf(stderr, "error opening stdin.\n");
    }
}

int main(void) {
    loop = uv_default_loop();

    uv_signal_t sigint_watcher;
    uv_signal_init(loop, &sigint_watcher);
    uv_signal_start(&sigint_watcher, on_sigint, SIGINT);

    init_watchers();
    return uv_run(loop, UV_RUN_DEFAULT);
}
