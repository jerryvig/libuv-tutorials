#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <curl/curl.h>

#define HANDLECOUNT 2
#define HTTP_HANDLE 0
#define FTP_HANDLE 1

static CURL *easy_handles[HANDLECOUNT];
static CURLM *multi_handle;
static int *still_running;

int main(void) {
    register int i;

    CURLMsg *msg;
    int msgs_left;

    for (i = 0; i < HANDLECOUNT; ++i) {
        easy_handles[i] = curl_easy_init();
    }

    curl_easy_setopt(easy_handles[HTTP_HANDLE], CURLOPT_URL, "https://gnu.org/");
    curl_easy_setopt(easy_handles[FTP_HANDLE], CURLOPT_URL, "ftp://ftp.gnu.org/");
    curl_easy_setopt(easy_handles[FTP_HANDLE], CURLOPT_UPLOAD, 1L);

    multi_handle = curl_multi_init();

    for (i = 0; i < HANDLECOUNT; ++i) {
        curl_multi_add_handle(multi_handle, easy_handles[i]);
    }

    still_running = (int*)malloc(sizeof(int));
    curl_multi_perform(multi_handle, still_running);

    while (*still_running) {
        struct timeval timeout = {
                .tv_sec = 1,
                .tv_usec = 0
        };
        int select_result;
        CURLMcode multiset_result;

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

        multiset_result = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fderror, &maxfd);

        if (multiset_result != CURLM_OK) {
            fprintf(stderr, "curl_multi_fdset() failed with code %d.\n", multiset_result);
            break;
        }

        if (maxfd == -1) {
            struct timeval wait = { 0, 100 * 1000 };
            select_result = select(0, NULL, NULL, NULL, &wait);
        } else {
            select_result = select(maxfd + 1, &fdread, &fdwrite, &fderror, &timeout);
        }

        if (select_result == -1) {
            perror("Select error\n");
        } else {
            curl_multi_perform(multi_handle, still_running);
        }


        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                for (i = 0; i < HANDLECOUNT; ++i) {
                    if (msg->easy_handle == easy_handles[i]) {
                        if (i == HTTP_HANDLE) {
                            printf("HTTP transfer completed with status %d\n", msg->data.result);
                            break;
                        } else if (i == FTP_HANDLE) {
                            printf("FTP transfer completed with status %d\n", msg->data.result);
                            break;
                        }
                    }
                }
            }
        }
    }

    curl_multi_cleanup(multi_handle);

    for (i = 0; i < HANDLECOUNT; ++i) {
        curl_easy_cleanup(easy_handles[i]);
    }

    curl_multi_cleanup(multi_handle);

    return EXIT_SUCCESS;
}
