#include "betterassert.h"
#include "betterpipes.h"
#include "betterthreads.h"
#include "producer-consumer.h"
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define TRUE 1

pc_queue_t *pc_queue;

void *consumer() {
    while (TRUE) {
    }
}

void *producer() {
    while (TRUE) {
    }
}

void server_init(char *fifo_path, pthread_t *threads, size_t max_sessions) {
    MK_FIFO(fifo_path);
    /*
        ALWAYS_ASSERT(pcq_create(pc_queue, max_sessions) != -1,
                      "producer-consumer critical error");
    */
    for (size_t i = 0; i < max_sessions; i++)
        THREAD_CREATE(threads + i, consumer);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc != 3, "usage: mbroker <pipename> <max_sessions>\n");

    errno = 0;
    char *endptr;
    size_t sessions = strtoul(argv[2], &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != argv[2],
                  "invalid max_sessions value\n");

    pthread_t threads[sessions];
    server_init(argv[1], threads, sessions);

    return 0;
}
