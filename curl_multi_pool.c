#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

typedef struct {
    char *memory;
    size_t size;
} memory_t;

static const char *urls[] = {
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

#define POOL_SIZE 3
#define NUM_URLS 24

static CURL *ez_pool[POOL_SIZE];

static size_t write_callback(char *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    memory_t *mem = (memory_t*)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory for realloc()\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static void create_and_init_ez_pool() {
    for (register int i = 0; i < POOL_SIZE; ++i) {
        memory_t *buffer = (memory_t*)malloc(sizeof(memory_t));
        buffer->memory = (char*)malloc(1);
        buffer->size = 0;

        ez_pool[i] = curl_easy_init();
        curl_easy_setopt(ez_pool[i], CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(ez_pool[i], CURLOPT_WRITEDATA, (void*)buffer);
        curl_easy_setopt(ez_pool[i], CURLOPT_PRIVATE, buffer);
        curl_easy_setopt(ez_pool[i], CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(ez_pool[i], CURLOPT_USERAGENT, "libcurl-agent/1.0");
    }
}

static void add_transfer(CURLM *multi, int i) {
    int ez_pool_idx = i % POOL_SIZE;

    curl_easy_setopt(ez_pool[ez_pool_idx], CURLOPT_URL, urls[i]);
    curl_multi_add_handle(multi, ez_pool[ez_pool_idx]);
    fprintf(stderr, "Added download %s\n", urls[i]);
}

static int msgs_left = -1;
static int still_alive = 1;
static int transfers = 0;

static void check_multi_info(CURLM *multi) {
    CURLMsg  *msg;
    while ((msg = curl_multi_info_read(multi, &msgs_left))) {
       if (msg->msg == CURLMSG_DONE) {
           memory_t *buffer;
           CURL *ez = msg->easy_handle;
           curl_easy_getinfo(ez, CURLINFO_PRIVATE, &buffer);

           if (buffer) {
               printf("buffer = %s\n", buffer->memory);

               //You would begin processing the data here, so I think this is where
               // you would init threads for a work queue.
               free(buffer->memory);
               buffer->memory = (char*)malloc(1);
               buffer->size = 0;
               //free(buffer);
           }
           curl_multi_remove_handle(multi, ez);
           //curl_easy_cleanup(ez);

           if (transfers < NUM_URLS) {
               add_transfer(multi, transfers++);
           }
       } else {
           fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
       }
    }
}

int main(void) {
    CURLM *multi;

    curl_global_init(CURL_GLOBAL_ALL);
    multi = curl_multi_init();

    curl_multi_setopt(multi, CURLMOPT_MAXCONNECTS, (long)POOL_SIZE);
    curl_multi_setopt(multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    create_and_init_ez_pool();

    for (transfers = 0; transfers < POOL_SIZE; ++transfers) {
        add_transfer(multi, transfers);
    }

    do {
        curl_multi_perform(multi, &still_alive);

        check_multi_info(multi);

        if (still_alive) {
            curl_multi_wait(multi, NULL, 0, 1000, NULL);
        }
    } while (still_alive || (transfers < NUM_URLS));

    for (register int i = 0; i < POOL_SIZE; ++i) {
        CURL *ez = ez_pool[i];
        memory_t *buffer;
        curl_easy_getinfo(ez, CURLINFO_PRIVATE, &buffer);
        if (buffer) {
            free(buffer->memory);
            free(buffer);
        }
        curl_easy_cleanup(ez);
    }

    curl_multi_cleanup(multi);
    curl_global_cleanup();

    return EXIT_SUCCESS;
}
