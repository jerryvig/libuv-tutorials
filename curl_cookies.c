#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

static void print_cookies(const CURL *ez) {

}

int main(void) {
    CURL *ez;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    ez = curl_easy_init();

    if (ez) {
        char nline[256];

        curl_easy_setopt(ez, CURLOPT_URL, "http://www.google.com/");
        curl_easy_setopt(ez, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(ez, CURLOPT_COOKIEFILE, "");

        res = curl_easy_perform(ez);

        if (res != CURLE_OK) {
            fprintf(stderr, "cURL perform failed with code %s\n", curl_easy_strerror(res));
            return EXIT_FAILURE;
        }

        print_cookies(ez);

        puts("Erasing cURL's cookie knowledge.\n");
        curl_easy_setopt(ez, CURLOPT_COOKIELIST, "ALL");

        print_cookies(ez);
    } else {
        fprintf(stderr, "cURL init failed.\n");
        return EXIT_FAILURE;
    }

    curl_global_cleanup();
    return EXIT_SUCCESS;
}