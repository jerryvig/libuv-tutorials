#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

uv_loop_t* create_loop() {
    uv_loop_t *loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    if (loop) {
        uv_loop_init(loop);
    }
    return loop;
}

void signal_handler(uv_signal_t *sig, int signum) {
    printf("signal recvd: %d\n", signum);
    uv_signal_stop(sig);
}

void sigint_handler(uv_signal_t *sig, int signum) {
    printf("YOU SENT THE SIGINT TO THIS PROCESS.\n");
    uv_signal_stop(sig);
    fprintf(stderr, "Starting graceful shutdown...\n");
    exit(0);
}

void worker1(void *args) {
    fprintf(stderr, "in worker1()\n");
    uv_loop_t *loop1 = create_loop();

    uv_signal_t sig1a, sig1b;
    uv_signal_init(loop1, &sig1a);
    uv_signal_init(loop1, &sig1b);

    uv_signal_start(&sig1a, signal_handler, SIGUSR1);
    uv_signal_start(&sig1b, signal_handler, SIGUSR1);

    uv_run(loop1, UV_RUN_DEFAULT);
}

void worker2(void *args) {
    fprintf(stderr, "in worker2()\n");
    uv_loop_t *loop2 = create_loop();
    uv_loop_t *loop3 = create_loop();

    uv_signal_t sig2;
    uv_signal_init(loop2, &sig2);
    uv_signal_start(&sig2, signal_handler, SIGUSR1);

    uv_signal_t sig3;
    uv_signal_init(loop2, &sig3);
    uv_signal_start(&sig3, signal_handler, SIGUSR1);

    while (uv_run(loop2, UV_RUN_NOWAIT) || uv_run(loop3, UV_RUN_NOWAIT)) {
    }
}

int main(void) {
    fprintf(stderr, "PID = %d\n", getpid());

    uv_loop_t *default_loop = uv_default_loop();

    uv_signal_t mainLoopSignal;
    uv_signal_init(default_loop, &mainLoopSignal);
    uv_signal_start(&mainLoopSignal, sigint_handler, SIGINT);

    uv_thread_t thread1, thread2;

    uv_thread_create(&thread1, worker1, NULL);
    uv_thread_create(&thread2, worker2, NULL);

    while (uv_run(default_loop, UV_RUN_NOWAIT) || uv_thread_join(&thread1) || uv_thread_join(&thread2)) {
    }

    return EXIT_SUCCESS;
}
