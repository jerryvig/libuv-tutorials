#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin for input. */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    /* Wait up to five seconds. */
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    retval = select(1, &rfds, NULL, NULL, &tv);

    if (retval == -1) {
        perror("select()");
    } else if (retval) {
        printf("Data is available now.\n");
    } else {
        printf("No data available within ten seconds.\n");
    }

    return EXIT_SUCCESS;
}
