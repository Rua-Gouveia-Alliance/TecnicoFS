#include "betterassert.h"
#include "betterpipes.h"
#include "betterthreads.h"
#include "opcodes.h"
#include "operations.h"
#include "producer-consumer.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BOXES_BLOCK 100

typedef struct {
    char path[BOX_PATH_SIZE];
    size_t n_publishers;
    size_t n_subscribers;
    size_t size;
} box_t;

box_t **boxes;
int box_count = 0;
size_t boxes_allocated_size = 0;
pc_queue_t *pc_queue;

void box_init(box_t *box, char *path) {
    box = malloc(sizeof(box));
    box->size = 0;
    box->n_publishers = 0;
    box->n_subscribers = 0;
    strncpy(box->path, path, BOX_PATH_SIZE);
}

int box_lookup(char *path) {
    for (int i = 0; i < box_count; i++)
        if (strncmp(path, boxes[i]->path, BOX_SIZE) == 0)
            return i;
    return -1;
}

int remove_box(char *path) {
    int index = box_lookup(path);
    if (index != -1) {
        box_count--;
        for (int i = index; i < box_count; i++)
            boxes[i] = boxes[i + 1];
    }
    return index;
}

int add_box(box_t *box) {
    if (box_count + 1 > boxes_allocated_size) {
        boxes = realloc(boxes, boxes_allocated_size + BOXES_BLOCK);
        if (boxes == NULL)
            return -1;
        boxes_allocated_size += BOXES_BLOCK;
    }
    boxes[box_count++] = box;
    return box_count - 1;
}

void publisher_session(char *fifo_path, char *box_path, int id) {
    (void)id;
    char buffer[MESSAGE_SIZE];
    int tfs_fd = tfs_open(box_path, TFS_O_APPEND | TFS_O_CREAT);
    for (;;) {
        int pipe_fd = open(fifo_path, O_RDONLY);
        read(pipe_fd, buffer, MESSAGE_SIZE);
        close(pipe_fd);
    }
    tfs_close(tfs_fd);
}

void consumer(char *request) {
    for (;;) {
        int op_code;
        char fifo[PIPE_SIZE], box_path[BOX_PATH_SIZE];
        parse_request(request, &op_code, fifo, box_path);

        switch (op_code) {
        case PUBLISHER:
            int id = 0;
            // If the box doesnt exist, reject the publisher
            // if ((id = box_lookup(box_name)) == -1)
            //    return; // Esta implemencatacao nao esta bem, e so provisorio
            publisher_session(fifo, box_path, id);
            break;
        default:
            PANIC("invalid op_code")
            break;
        }
    }
}

void server_init(char *fifo_path, pthread_t *threads, size_t max_sessions) {
    (void)threads;
    (void)max_sessions;
    MK_FIFO(fifo_path);
    tfs_init(NULL);
    boxes = malloc(BOXES_BLOCK * sizeof(box_t *));
    /*
        ALWAYS_ASSERT(pcq_create(pc_queue, max_sessions) != -1,
                      "producer-consumer critical error");
    for (size_t i = 0; i < max_sessions; i++)
        THREAD_CREATE(threads + i, consumer);
    */
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: mbroker <pipename> <max_sessions>\n");

    // errno = 0;
    char *fifo_path = argv[1];
    /*
        size_t sessions = strtoul(argv[2], &endptr, 10);
        ALWAYS_ASSERT(errno == 0 && endptr != argv[2],
                    "invalid max_sessions value\n");
    */
    server_init(fifo_path, NULL, 0);
    for (;;) {
        int fifo = open(fifo_path, O_RDONLY);
        char buffer[REQUEST_SIZE];
        read(fifo, buffer, REQUEST_SIZE);
        consumer(buffer);
        close(fifo);
    }
    /*
        pthread_t threads[sessions];
        server_init(argv[1], threads, sessions);
    */
    return 0;
}
