/* uvtee.c - An implementation of the unix tee command with libuv. https://libuv.org/
 *
 * compile with: gcc uvtee.c -o uvtee -luv -Wall -Wextra -pedantic -std=c11
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

static uv_loop_t *loop;
static uv_pipe_t *stdin_pipe;
static uv_pipe_t *stdout_pipe;
static uv_pipe_t *file_pipe;

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*)req;
    free(wr->buf.base);
    free(wr);
}

void on_stdout_write(uv_write_t *req, int status) {
    free_write_req(req);
}

void on_file_write(uv_write_t *req, int status) {
    free_write_req(req);
}

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb cb) {
    write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
    req->buf = uv_buf_init((char*)malloc(size), size);
    memcpy(req->buf.base, buf.base, size);
    uv_write((uv_write_t*)req, (uv_stream_t*)dest, &req->buf, 1, cb);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    printf("suggested_size = %zu\n", suggested_size);
    *buf = uv_buf_init((char*)malloc(suggested_size), suggested_size);
}

void read_stdin(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread == UV_EOF) {
            uv_close((uv_handle_t*)stdin_pipe, NULL);
            uv_close((uv_handle_t*)stdout_pipe, NULL);
            uv_close((uv_handle_t*)file_pipe, NULL);
        }
    } else if (nread > 0) {
        write_data((uv_stream_t*)stdout_pipe, nread, *buf, on_stdout_write);
        write_data((uv_stream_t*)file_pipe, nread, *buf, on_file_write);
    }

    if (buf->base) {
        free(buf->base);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    loop = uv_default_loop();

    stdin_pipe = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    stdout_pipe = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    file_pipe = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));

    uv_pipe_init(loop, stdin_pipe, 0);
    uv_pipe_open(stdin_pipe, STDIN_FILENO);

    uv_pipe_init(loop, stdout_pipe, 0);
    uv_pipe_open(stdout_pipe, STDOUT_FILENO);

    uv_fs_t file_req;
    int fd = uv_fs_open(loop, &file_req, argv[1], O_CREAT | O_RDWR, 0644, NULL );
    uv_pipe_init(loop, file_pipe, 0);
    uv_pipe_open(file_pipe, fd);

    uv_read_start((uv_stream_t*)stdin_pipe, alloc_buffer, read_stdin);

    uv_run(loop, UV_RUN_DEFAULT);

    free(stdin_pipe);
    free(stdout_pipe);
    free(file_pipe);
    return EXIT_SUCCESS;
}
