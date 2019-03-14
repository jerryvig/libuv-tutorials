/* uv_thread_create.c - Threads using libuv (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

void hare(void *args) {
    int tracklen = *((int*) args);

    while (tracklen) {
        tracklen--;
        sleep(1);
        fprintf(stderr, "Hare ran another lap\n");
    }

    fprintf(stderr, "Hare is done running!\n");
}

void tortoise(void *args) {
    int tracklen = *((int*)args);

    while (tracklen) {
        tracklen--;
        fprintf(stderr, "Tortoise ran another lap\n");
        sleep(2);
    }
    fprintf(stderr, "Tortoise done running!\n");
}

int main(void) {
    int tracklen = 10;

    uv_thread_t hare_id;
    uv_thread_t tortoise_id;

    uv_thread_create(&hare_id, hare, &tracklen);
    uv_thread_create(&tortoise_id, tortoise, &tracklen);

    uv_thread_join(&hare_id);
    uv_thread_join(&tortoise_id);

    return 0;
}
