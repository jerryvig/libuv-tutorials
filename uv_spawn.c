#define _DEFAULT_SOURCE

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <uv.h>

static uv_loop_t *loop;
static uv_process_t child_proc;
static uv_process_options_t options;

static void on_proc_exit(uv_process_t *proc, int64_t exit_status, int term_signal) {
    fprintf(stderr, "Process exited with status %" PRId64 ", signal %d\n", exit_status, term_signal);
    uv_close((uv_handle_t*)proc, NULL);
}

int main(void) {
    loop = uv_default_loop();

    char* args[3] = {
            "mkdir",
            "test-uv-spawn-directory",
            NULL
    };

    options.exit_cb = on_proc_exit;
    options.file = "mkdir";
    options.args = args;

    int r;
    if ((r = uv_spawn(loop, &child_proc, &options))) {
        fprintf(stderr, "Launched process with ID %d\n", child_proc.pid);
        return EXIT_FAILURE;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
