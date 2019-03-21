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

typedef struct curl_context_s {
    uv_poll_t poll_handle;
    curl_socket_t sockfd;
} curl_context_t;

typedef struct {
    char *memory;
    size_t size;
} memory_t;

#define POOL_SIZE 4

typedef struct {
    CURL *ez_pool[POOL_SIZE];
    CURLM *curl_multi;
} curl_multi_ez_t;

static uv_loop_t *loop;
static uv_timer_t *timeout;
static curl_multi_ez_t curl_multi_ez;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    memory_t *mem = (memory_t*)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Insufficient memory (realloc returned NULL)...\n");
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
    int ez_pool_idx = num % POOL_SIZE;

    CURL *ez = curl_multi_ez.ez_pool[ez_pool_idx];
    curl_easy_setopt(ez, CURLOPT_URL, url);

    curl_multi_add_handle(curl_multi_ez.curl_multi, ez);
    fprintf(stderr, "Added download->buffer %s\n", url);
}

static void check_multi_info(void) {
    char *done_url;
    CURLMsg *message;
    int pending;

    CURL *ez;
    memory_t *buffer;

    while ((message = curl_multi_info_read(curl_multi_ez.curl_multi, &pending))) {
        switch (message->msg) {
            case CURLMSG_DONE:
                ez = message->easy_handle;
                curl_easy_getinfo(ez, CURLINFO_EFFECTIVE_URL, &done_url);
                curl_easy_getinfo(ez, CURLINFO_PRIVATE, &buffer);
                fprintf(stderr, "Finished fetching %s. DONE\n", done_url);

                if (buffer) {
                    printf("buffer = %s\n", buffer->memory);

                    //You would begin processing the data here, so I think this is where
                    // you would init threads for a work queue.
                    free(buffer->memory);
                    buffer->memory = (char*)malloc(1);
                    buffer->size = 0;
                }

                curl_multi_remove_handle(curl_multi_ez.curl_multi, ez);
                //curl_easy_cleanup(ez);
                //if (buffer) {
                //    printf("buffer = \"%s\"", buffer->memory);
                //    free(buffer->memory);
                //    free(buffer);
                //}
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
    curl_multi_socket_action(curl_multi_ez.curl_multi, context->sockfd, flags, &running_handles);
    check_multi_info();
}

static void on_timeout(uv_timer_t *req) {
    int running_handles;
    curl_multi_socket_action(curl_multi_ez.curl_multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    check_multi_info();
}

static int start_timeout(CURLM *curl_multi, long timeout_ms, void *userp) {
    if (timeout_ms < 0) {
        uv_timer_stop(timeout);
    } else {
        if (timeout_ms == 0) {
            timeout_ms = 1;
        }
        uv_timer_start(timeout, on_timeout, timeout_ms, 0);
    }
    return 0;
}

static int handle_socket(CURL *ez, curl_socket_t curl_sock, int action, void *userp, void *socketp) {
    curl_context_t *curl_context;
    int events = 0;

    switch (action) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
            curl_context = socketp ? (curl_context_t*)socketp : create_curl_context(curl_sock);
            curl_multi_assign(curl_multi_ez.curl_multi, curl_sock, (void*)curl_context);
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
                curl_multi_assign(curl_multi_ez.curl_multi, curl_sock, NULL);
            }
            break;
        default:
            abort();
    }

    return 0;
}

static CURLM *create_and_init_curl_multi() {
    CURLM *multi_handle = curl_multi_init();
    curl_multi_setopt(multi_handle, CURLMOPT_MAXCONNECTS, (long)POOL_SIZE);
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_setopt(multi_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
    curl_multi_setopt(multi_handle, CURLMOPT_TIMERFUNCTION, start_timeout);
    return multi_handle;
}

CURL *create_and_init_curl(void) {
    CURL *ez = curl_easy_init();
    curl_easy_setopt(ez, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(ez, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(ez, CURLOPT_ACCEPT_ENCODING, "br, gzip");
    curl_easy_setopt(ez, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(ez, CURLOPT_TCP_KEEPIDLE, 180L);
    curl_easy_setopt(ez, CURLOPT_TCP_KEEPINTVL, 60L);
    curl_easy_setopt(ez, CURLOPT_TCP_FASTOPEN, 1L);
    curl_easy_setopt(ez, CURLOPT_TCP_NODELAY, 0);
    curl_easy_setopt(ez, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(ez, CURLOPT_WRITEFUNCTION, &write_callback);
    //put the header function back.
    // curl_easy_setopt(ez, CURLOPT_HEADERFUNCTION, &header_callback);
    return ez;
}

static void create_and_init_multi_ez() {
    curl_multi_ez.curl_multi = create_and_init_curl_multi();
    for (int i = 0; i < POOL_SIZE; ++i) {
        curl_multi_ez.ez_pool[i] = create_and_init_curl();

        memory_t *buffer = (memory_t*)malloc(sizeof(memory_t));
        buffer->memory = (char*)malloc(1);
        buffer->size = 0;

        curl_easy_setopt(curl_multi_ez.ez_pool[i], CURLOPT_WRITEDATA, (void*)buffer);
        curl_easy_setopt(curl_multi_ez.ez_pool[i], CURLOPT_PRIVATE, buffer);
    }
}

int run_loop(const char* urls[], const int url_count) {
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
    create_and_init_multi_ez();

    register int i;
    for (i = 0; i < POOL_SIZE; ++i) {
        fprintf(stderr, "adding \"%s\" to downloads...\n", urls[i]);
        add_download(urls[i], i);
    }

    fprintf(stderr, "Starting the event loop run....\n");
    uv_run(loop, UV_RUN_DEFAULT);
    fprintf(stderr, "event loop run ended...shutting down...\n");

    //TODO: cleanup all of the ez handles and their write-buffers here.

    curl_multi_cleanup(curl_multi_ez.curl_multi);
    curl_global_cleanup();
    free(timeout);
    return 0;
}

int main(void) {
    const char *urls[] = {
            "https://finance.yahoo.com/quote/AAL/history",
            "https://finance.yahoo.com/quote/AAPL/history",
            "https://finance.yahoo.com/quote/ADBE/history",
            "https://finance.yahoo.com/quote/ADI/history",
            "https://finance.yahoo.com/quote/ADP/history",
            "https://finance.yahoo.com/quote/ADSK/history",
            "https://finance.yahoo.com/quote/ALGN/history",
            "https://finance.yahoo.com/quote/ALXN/history",
            "https://finance.yahoo.com/quote/AMAT/history",
            "https://finance.yahoo.com/quote/AMD/history",
            "https://finance.yahoo.com/quote/AMGN/history",
            "https://finance.yahoo.com/quote/AMZN/history",
            "https://finance.yahoo.com/quote/ASML/history",
            "https://finance.yahoo.com/quote/ATVI/history",
            "https://finance.yahoo.com/quote/AVGO/history",
            "https://finance.yahoo.com/quote/BIDU/history",
            "https://finance.yahoo.com/quote/BIIB/history",
            "https://finance.yahoo.com/quote/BKNG/history",
            "https://finance.yahoo.com/quote/BMRN/history",
            "https://finance.yahoo.com/quote/CDNS/history",
            "https://finance.yahoo.com/quote/CELG/history",
            "https://finance.yahoo.com/quote/CERN/history",
            "https://finance.yahoo.com/quote/CHKP/history",
            "https://finance.yahoo.com/quote/CHTR/history"
    };
    const int url_count = sizeof(urls) / sizeof(char *);

    int loop_success = run_loop(urls, url_count);
    if (!loop_success) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
