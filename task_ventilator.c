#define _DEFAULT_SOURCE

#include <string.h>
#include "zhelpers.h"

#define NUM_TASKS 256
#define VENTILATOR_PORT 5557
#define SINK_PORT 5558

int main(void) {
    void *context = zmq_ctx_new();

    void *ventilator = zmq_socket(context, ZMQ_PUSH);
    char ventilator_port[16] = {'\0'};
    sprintf(ventilator_port, "tcp://*:%d", VENTILATOR_PORT);

    void *sink = zmq_socket(context, ZMQ_PUSH);
    char sink_port[32] = {'\0'};
    sprintf(sink_port, "tcp://localhost:%d", SINK_PORT);

    zmq_bind(ventilator, ventilator_port);
    zmq_connect(sink, sink_port);

    printf("Press <Enter> when the workers are ready: ");
    getchar();
    printf("Routing tasks to the workers...\n");

    s_send(sink, "0");

    srandom((unsigned)time(NULL));

    int total_msec;
    for (int task_nbr = 0; task_nbr < NUM_TASKS; ++task_nbr) {
        int workload = randof(100) + 1;
        total_msec += workload;
        char string[10];
        sprintf(string, "%d", workload);
        s_send(ventilator, string);
    }
    printf("Total expected cost: %d ms.\n", total_msec);

    zmq_close(sink);
    zmq_close(ventilator);
    zmq_ctx_destroy(context);

    return EXIT_SUCCESS;
}
