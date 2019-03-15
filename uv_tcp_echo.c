/* uv_tcp_echo.c - A TCP echo server implemented with libuv (https://libuv.org).
 *
 * Compiled with: gcc uv_tcp_echo.c -o uv_tcp_echo -luv -Wall -Wextra -pedantic -std=c11
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define DEFAULT_PORT 8080
#define DEFAULT_BACKLOG 128

static uv_loop_t *loop;
static struct sockaddr_in addr;
static uv_tcp_t *tcp_echo_server;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*)req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t *handle) {
    free(handle);
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    fprintf(stderr, "closing the send handle mofo\n");
    uv_close((uv_handle_t*)req->handle, on_close);
    free_write_req(req);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
        req->buf = uv_buf_init(buf->base, nread);
        uv_write((uv_write_t*)req, client, &req->buf, 1, echo_write);
    }
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }
        fprintf(stderr, "About to close the client connection.\n");
        uv_close((uv_handle_t*)client, on_close);
    }
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
    }
}

int main(void) {
    loop = uv_default_loop();

    tcp_echo_server = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

    uv_tcp_init(loop, tcp_echo_server);

    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

    uv_tcp_bind(tcp_echo_server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)tcp_echo_server, DEFAULT_BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return EXIT_FAILURE;
    }

    printf("TCP echo server running on port %d...\n", DEFAULT_PORT);
    uv_run(loop, UV_RUN_DEFAULT);

    free(tcp_echo_server);
    return EXIT_SUCCESS;
}
