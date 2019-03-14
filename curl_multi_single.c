#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>

#include <curl/curl.h>

int main(void) {
    CURL *http_handle;
    CURL *multi_handle;

    int still_running = 0;
    int repeats = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    http_handle = curl_easy_init();

    curl_easy_setopt(http_handle, CURLOPT_URL, "https://news.google.com/");

    multi_handle = curl_multi_init();

    curl_multi_add_handle(multi_handle, http_handle);

    curl_multi_perform(multi_handle, &still_running);

    while (still_running) {
        CURLMcode mc;
        int numfds;

        mc = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);

        if (mc != CURLM_OK) {
            fprintf(stderr, "curl_multi_wait() failed with code %d.\n", mc);
            break;
        }

        if (!numfds) {
            repeats++;
            if (repeats > 1) {
                struct timeval wait = { 0, 100 * 1000 };
                select(0, NULL, NULL, NULL, &wait);
            }
        } else {
            repeats = 0;
        }
        curl_multi_perform(multi_handle, &still_running);
    }

    curl_multi_remove_handle(multi_handle, http_handle);
    curl_easy_cleanup(http_handle);
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();

    printf("Finished the download\n");
    return EXIT_SUCCESS;
}
