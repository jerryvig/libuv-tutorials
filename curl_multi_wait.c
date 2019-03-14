#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <curl/curl.h>

#define WAIT_TIMEOUT_MS 1000
#define EASY_HANDLE_COUNT 2

static CURL *easy_handles[2];
static CURLM *multi_handle;
static int still_running;
static int repeats;
static const char* urls[] = {
        "https://curl.haxx.se/",
        "https://www.google.com/maps"
};
static int i;

int main(void) {
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        fprintf(stderr, "Failed to initialize cURL.\n");
        return EXIT_FAILURE;
    }

    struct timeval wait = { 0, 100 * 1000 };

    multi_handle = curl_multi_init();

    for (i = 0; i < EASY_HANDLE_COUNT; ++i) {
        easy_handles[i] = curl_easy_init();
        curl_easy_setopt(easy_handles[i], CURLOPT_URL, urls[i]);
        curl_multi_add_handle(multi_handle, easy_handles[i]);
    }

    do {
        CURLMcode mc;
        int numfds;

        mc = curl_multi_perform(multi_handle, &still_running);

        if (mc == CURLM_OK) {
            mc = curl_multi_wait(multi_handle, NULL, 0, WAIT_TIMEOUT_MS, &numfds);
        }

        if (mc != CURLM_OK) {
            fprintf(stderr, "curl_multi failed with code %d.\n", mc);
            break;
        }

        if (!numfds) {
            repeats++;
            if (repeats > 1) {
                select(0, NULL, NULL, NULL, &wait);
            }
        } else {
            repeats = 0;
        }
    } while (still_running);

    for (i = 0; i < EASY_HANDLE_COUNT; ++i) {
        curl_multi_remove_handle(multi_handle, easy_handles[i]);
        curl_easy_cleanup(easy_handles[i]);
    }

    curl_multi_cleanup(multi_handle);

    return EXIT_SUCCESS;
}
