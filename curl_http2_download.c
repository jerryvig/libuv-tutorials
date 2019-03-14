#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <curl/curl.h>

#define NUM_HANDLES 1000
#define DEFAULT_NUM_TRANSFERS 3

#ifndef CURLPIPE_MULTIPLEX
#define CURLPIPE_MULTIPLEX 0
#endif

typedef struct transfer_s {
    CURL *ez;
    unsigned int num;
    FILE *out;
} transfer_t;

static CURLM *multi_handle;
static int still_running;

static int my_trace(CURL *ez, curl_infotype type, char *data, size_t size, void *userp) {
    const char *text;
    transfer_t *t = (transfer_t*)userp;
    unsigned int num = t->num;
    (void)ez;

    switch(type) {
        case CURLINFO_TEXT:
            fprintf(stderr, "== %d Info: %s", num, data);
        default:
            return 0;

        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }

    printf("text = %s\n", text);

    return 0;
}

static void setup(transfer_t *t, int num) {
    char filename[128];
    memset(filename, 0, 128);

    t->ez = curl_easy_init();

    snprintf(filename, 128, "dl-%d", num);

    t->out = fopen(filename, "wb");

    curl_easy_setopt(t->ez, CURLOPT_WRITEDATA, t->out);
    curl_easy_setopt(t->ez, CURLOPT_URL, "https://www.google.com/maps");
    curl_easy_setopt(t->ez, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(t->ez, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(t->ez, CURLOPT_DEBUGDATA, t);

    /* HTTP/2 please */
    curl_easy_setopt(t->ez, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

    /* curl_easy_setopt(t->ez, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(t->ez, CURLOPT_SSL_VERIFYHOST, 0L); */

#if (CURLPIPE_MULTIPLEX > 0)
    curl_easy_setopt(t->ez, CURLOPT_PIPEWAIT, 1L);
#endif
}

int main(int argc, char **argv) {
    register int i;
    transfer_t trans[NUM_HANDLES];

    int num_transfers = DEFAULT_NUM_TRANSFERS;

    if (argc > 1) {
        num_transfers = atoi(argv[1]);
        if (num_transfers < 1 || num_transfers > NUM_HANDLES) {
            num_transfers = DEFAULT_NUM_TRANSFERS;
        }
    }

    multi_handle = curl_multi_init();

    for (i = 0; i < num_transfers; ++i) {
        setup(&trans[i], i);
        curl_multi_add_handle(multi_handle, trans[i].ez);
    }

    /* enable http/2 multiplexing on the multi handle */
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    curl_multi_perform(multi_handle, &still_running);

    while (still_running) {
        struct timeval timeout = {
                .tv_sec = 1,
                .tv_usec = 0
        };
        int select_result = 0;
        CURLMcode fdset_result;

        fd_set fdread;
        fd_set fdwrite;
        fd_set fderror;

        int maxfd = -1;

        long curl_timeout = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fderror);

        curl_multi_timeout(multi_handle, &curl_timeout);
        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            if (timeout.tv_sec > 1) {
                timeout.tv_sec = 1;
            } else {
                timeout.tv_usec = (curl_timeout % 1000) * 1000;
            }
        }

        fdset_result = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fderror, &maxfd);

        if (fdset_result != CURLM_OK) {
            fprintf(stderr, "curl_multi_fdset() failed with code %d.\n", fdset_result);
            break;
        }

        if (maxfd == -1) {
            struct timeval wait = {0, 100000};
            select_result = select(0, NULL, NULL, NULL, &wait);
        } else {
            select_result = select(maxfd + 1, &fdread, &fdwrite, &fderror, &timeout);
        }

        if (select_result == -1) {
            perror("select() error\n");
        } else {
            curl_multi_perform(multi_handle, &still_running);
        }
    }

    for (i = 0; i < num_transfers; ++i) {
        curl_multi_remove_handle(multi_handle, trans[i].ez);
        curl_easy_cleanup(trans[i].ez);
        fclose(trans[i].out);
    }

    curl_multi_cleanup(multi_handle);

    return EXIT_SUCCESS;
}
