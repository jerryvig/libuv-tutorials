/* uv_wget.c - An example of using the libcurl multi-socket api with libuv (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <curl/curl.h>

static uv_loop_t *loop;
static CURLM *curl_multi_handle;
static uv_timer_t *timeout;

typedef struct curl_context_s {
    uv_poll_t poll_handle;
    curl_socket_t sockfd;
} curl_context_t;

typedef struct {
    char *memory;
    size_t size;
} memory_t;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    memory_t *mem = (memory_t*)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Insufficient memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static curl_context_t* create_curl_context(curl_socket_t sockfd) {
    //allocate the curl context for the socket.
    curl_context_t *context = (curl_context_t*)malloc(sizeof(curl_context_t));
    //set the context's socket-file-descriptor
    context->sockfd = sockfd;

    //initialize polling on the socket's file descriptor.
    int r = uv_poll_init_socket(loop, &context->poll_handle, sockfd);
    assert(r == 0);

    //set the data for poll_handle to the context.
    context->poll_handle.data = context;
    return context;
}

static void curl_close_cb(uv_handle_t *handle) {
    //frees the context that was allocated in create_curl_context().
    curl_context_t *context = (curl_context_t*)handle->data;
    free(context);
}

static void destroy_curl_context(curl_context_t *context) {
    //close the poller and call the close callback.
    uv_close((uv_handle_t*)&context->poll_handle, curl_close_cb);
}

static void add_download(const char *url, int num) {
    char filename[50];
    memset(filename, 0, 50);
    sprintf(filename, "%d.download", num);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error opening file %s.\n", filename);
        return;
    }

    CURL *ez = curl_easy_init();
    curl_easy_setopt(ez, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(ez, CURLOPT_PRIVATE, file);
    curl_easy_setopt(ez, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(ez, CURLOPT_URL, url);
    curl_multi_add_handle(curl_multi_handle, ez);
    fprintf(stderr, "Added download->file %s -> %s\n", url, filename);
}

static void check_multi_info(void) {
    char *done_url;
    CURLMsg *message;
    int pending;

    CURL *easy_handle;
    FILE *file;

    while ((message = curl_multi_info_read(curl_multi_handle, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE:
                easy_handle = message->easy_handle;
                curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
                curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &file);
                fprintf(stderr, "Finished fetching %s. DONE\n", done_url);

                curl_multi_remove_handle(curl_multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);
                if (file) {
                    fclose(file);
                }
                break;
            default:
                fprintf(stderr, "CURLMSG default\n");
                break;
        }
    }
}

void curl_perform(uv_poll_t *req, int status, int events) {
    int running_handles;
    int flags = 0;
    curl_context_t *context;

    if (status < 0) {
        flags = CURL_CSELECT_ERR;
    }

    if (events & UV_READABLE) {
        flags |= CURL_CSELECT_IN;
    }
    if (events & UV_WRITABLE) {
        flags |= CURL_CSELECT_OUT;
    }

    context = (curl_context_t*)req->data;
    curl_multi_socket_action(curl_multi_handle, context->sockfd, flags, &running_handles);
    check_multi_info();
}

static void on_timeout(uv_timer_t *req) {
    int running_handles;
    curl_multi_socket_action(curl_multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    check_multi_info();
}

static int start_timeout(CURLM *curl_multi, long timeout_ms, void *userp) {
    if (timeout_ms < 0) {
        uv_timer_stop(timeout);
    } else {
        if (timeout_ms == 0) {
            timeout_ms = 1;
        }
        // fprintf(stderr, "starting timeout with timeout_ms = %lu\n", timeout_ms);
        uv_timer_start(timeout, on_timeout, timeout_ms, 0);
    }
    return 0;
}

static int handle_socket(CURL *curl_easy, curl_socket_t curl_sock, int action, void *userp, void *socketp) {
    curl_context_t *curl_context;
    int events = 0;

    switch (action) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
            curl_context = socketp ? (curl_context_t*)socketp : create_curl_context(curl_sock);
            curl_multi_assign(curl_multi_handle, curl_sock, (void*)curl_context);

            if (action != CURL_POLL_IN) {
                events |= UV_WRITABLE;
            }
            if (action != CURL_POLL_OUT) {
                events |= UV_READABLE;
            }

            uv_poll_start(&curl_context->poll_handle, events, curl_perform);
            break;

        case CURL_POLL_REMOVE:
            if (socketp) {
                uv_poll_stop(&((curl_context_t*)socketp)->poll_handle);
                destroy_curl_context((curl_context_t*)socketp);
                curl_multi_assign(curl_multi_handle, curl_sock, NULL);
            }
            break;
        default:
            abort();
    }

    return 0;
}

int run_loop(char* urls[], int url_count) {
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        fprintf(stderr, "Failed to initialize cURL.\n");
        return 1;
    }

    //initialize the default libuv event loop.
    loop = uv_default_loop();

    //allocate the timeout and init the timeout.
    timeout = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, timeout);

    //initialize the curl multi handle and set the socket and timer callbacks.
    curl_multi_handle = curl_multi_init();
    curl_multi_setopt(curl_multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(curl_multi_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
    curl_multi_setopt(curl_multi_handle, CURLMOPT_TIMERFUNCTION, start_timeout);

    for (int i = 0; i < url_count; ++i) {
        fprintf(stderr, "adding \"%s\" to downloads\n", urls[i]);
        add_download(urls[i], i);
    }

    fprintf(stderr, "starting the event loop run....\n");
    uv_run(loop, UV_RUN_DEFAULT);
    fprintf(stderr, "event loop run ended...shutting down...\n");

    curl_multi_cleanup(curl_multi_handle);
    free(timeout);
    return 0;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: ./uv_wget <url1> <url2>...\n");
        return EXIT_SUCCESS;
    }

    char* urls[16];
    for (int i = 1; i < argc; ++i) {
        urls[i-1] = argv[i];
    }

    int loop_success = run_loop(urls, argc - 1);
    if (!loop_success) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
