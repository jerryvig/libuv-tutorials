#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>

#include <curl/curl.h>

#define HANDLE_COUNT 4

static CURL* handles[HANDLE_COUNT];
static CURL *multi_handle;
static int *still_running;

int main(void) {
    register int i;
    still_running = (int*)malloc(sizeof(int));

    multi_handle = curl_multi_init();

    char* urls[] = {
            "https://news.google.com/?hl=en-US&gl=US&ceid=US:en",
            "https://www.bing.com/news",
            "https://www.google.com/maps",
            "https://www.bing.com/maps"
    };
    for (i = 0; i < HANDLE_COUNT; ++i) {
        handles[i] = curl_easy_init();
        curl_easy_setopt(handles[i], CURLOPT_URL, urls[i]);
        curl_multi_add_handle(multi_handle, handles[i]);
    }

    curl_multi_perform(multi_handle, still_running);

    while (*still_running) {
        struct timeval timeout = {
                .tv_sec = 1,
                .tv_usec = 0
        };
        int select_result;
        CURLMcode multi_result_code;

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
           }
        } else {
            timeout.tv_usec = (curl_timeout % 1000) * 1000;
        }

        multi_result_code = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fderror, &maxfd);

        if (multi_result_code != CURLM_OK) {
            fprintf(stderr, "curl_multi_fdset() failed with code %d\n", multi_result_code);
            break;
        }

        if (maxfd == -1) {
            struct timeval wait = {0, 100 * 1000};
            select_result = select(0, NULL, NULL, NULL, &wait);
        } else {
            select_result = select(maxfd + 1, &fdread, &fdwrite, &fderror, &timeout);
        }

        if (select_result == -1) {
            printf("select error\n");
        } else {
            curl_multi_perform(multi_handle, still_running);
        }
    }

    curl_multi_cleanup(multi_handle);

    for (i = 0; i < HANDLE_COUNT; ++i) {
        curl_easy_cleanup(handles[i]);
    }
    free(still_running);

    return EXIT_SUCCESS;
}
