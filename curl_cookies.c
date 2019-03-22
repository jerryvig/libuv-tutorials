#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

static void print_cookies(const CURL *ez) {
    CURLcode res;
    struct curl_slist *cookie_head;
    struct curl_slist *next_cookie;

    puts("\nCookies, curl knows:\n");
    res = curl_easy_getinfo(ez, CURLINFO_COOKIELIST, &cookie_head);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_getinfo() failed...\n");
    }

    next_cookie = cookie_head;
    int16_t i = 1;
    while (next_cookie) {
        printf("[%d]: %s\n", i, next_cookie->data);
        next_cookie = next_cookie->next;
        i++;
    }
    if (i == 1) {
        puts("(none)\n");
    }
    curl_slist_free_all(cookie_head);
}

int main(void) {
    CURL *ez;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    ez = curl_easy_init();

    if (ez) {
        curl_easy_setopt(ez, CURLOPT_URL, "http://www.google.com/");
        curl_easy_setopt(ez, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(ez, CURLOPT_COOKIEFILE, "");

        res = curl_easy_perform(ez);

        if (res != CURLE_OK) {
            fprintf(stderr, "cURL perform failed with code %s\n", curl_easy_strerror(res));
            return EXIT_FAILURE;
        }

        print_cookies(ez);

        puts("==== Erasing cURL's cookie knowledge. =====\n");
        curl_easy_setopt(ez, CURLOPT_COOKIELIST, "ALL");

        print_cookies(ez);

        curl_easy_cleanup(ez);
    } else {
        fprintf(stderr, "cURL init failed.\n");
        return EXIT_FAILURE;
    }

    curl_global_cleanup();
    return EXIT_SUCCESS;
}