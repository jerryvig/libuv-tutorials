/* uv_progress.c - See libuv documentation (https://libuv.org).
 *
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

static uv_loop_t *loop;
static uv_async_t async;

void fake_download(uv_work_t *work_unit) {
    double percentage;
    int size = *((int*)work_unit->data);
    int downloaded = 0;
    while (downloaded < size) {
        percentage = downloaded * 100.0 / size;
        async.data = (void*)&percentage;
        uv_async_send(&async);
        sleep(1);

        downloaded += (500 + random())%1000;
    }
}

void after_fake_download(uv_work_t *work_unit, int status) {
    if (status == 0) {
        fprintf(stderr, "Download complete!!!\n");
        uv_close((uv_handle_t * ) & async, NULL);
    }
}

void print_progress(uv_async_t *handle) {
    double percentage = *((double*)handle->data);
    fprintf(stderr, "Downloaded %.2f%%...\n", percentage);
}

int main(void) {
    loop = uv_default_loop();

    uv_work_t work_unit;
    int size = 10240;
    work_unit.data = (void*)&size;

    uv_async_init(loop, &async, print_progress);
    uv_queue_work(loop, &work_unit, fake_download, after_fake_download);

    return uv_run(loop, UV_RUN_DEFAULT);
}
