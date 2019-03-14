#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <uv.h>

static char buf[32];
static uv_fs_t open_req;
static uv_fs_t read_req;
static uv_fs_t write_req;
static uv_buf_t iov;
static uv_loop_t *loop;

void on_read(uv_fs_t *req);

void on_write(uv_fs_t *req) {
    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(req->result));
    }
    else {
        uv_fs_read(loop, &read_req, open_req.result, &iov, 1, -1, on_read);
    }
}

void on_read(uv_fs_t *req) {
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    }
    else if (req->result == 0) {
        uv_fs_t close_req;
        uv_fs_close(loop, &close_req, open_req.result, NULL);
    }
    else if (req->result > 0) {
        iov.len = req->result;
        uv_fs_write(loop, &write_req, STDOUT_FILENO, &iov, 1, -1, on_write);
    }
}

void on_open(uv_fs_t *req) {
    assert(req == &open_req);
    if (req->result >= 0) {
        iov = uv_buf_init(buf, sizeof(buf));
        uv_fs_read(loop, &read_req, req->result, &iov, 1, -1, on_read);
    } else {
        fprintf(stderr, "error opening file %s\n", uv_strerror((int)req->result));
    }
}

int main(int argc, char **argv) {
    // We set the dominoes rolling in main().
    if (argc < 2) {
        fprintf(stderr, "No file given....\n");
        return EXIT_FAILURE;
    }

    loop = uv_default_loop();
    uv_fs_open(loop, &open_req, argv[1], O_RDONLY, 0, on_open);
    uv_run(loop, UV_RUN_DEFAULT);

    uv_fs_req_cleanup(&open_req);
    uv_fs_req_cleanup(&read_req);
    uv_fs_req_cleanup(&write_req);

    return EXIT_SUCCESS;
}
