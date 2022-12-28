#include "betterassert.h"
#include "betterlocks.h"
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
#define MAX_MESSAGES 1024
/**
 * Each box is represented by a tfs file, so the max amount of messages possible
 * is the used tfs block size.
 **/

typedef struct {
    char path[BOX_PATH_SIZE];
    size_t messages[MAX_MESSAGES];
    size_t message_count;
    size_t n_publishers;
    size_t n_subscribers;
    size_t size;
} box_t;

box_t **boxes;
int box_count = 0;
pthread_cond_t *box_cond;
pthread_mutex_t *box_mutex;
size_t boxes_allocated_size;
pc_queue_t *pc_queue;

box_t *create_box(char *path) {
    box_t *box = malloc(sizeof(box_t));
    if (box == NULL)
        return NULL;

    // Damos so overwrite ao file de uma box se o nome ja existir , provisorio
    int fd = tfs_open(path, TFS_O_CREAT | TFS_O_TRUNC);
    if (fd == -1)
        return NULL;
    tfs_close(fd);

    box->size = 0;
    box->n_publishers = 0;
    box->n_subscribers = 0;
    box->message_count = 0;

    strncpy(box->path, path, BOX_PATH_SIZE - 1);
    box->path[BOX_PATH_SIZE - 1] = '\0';

    return box;
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
        free(boxes[index]);
        box_count--;
        for (int i = index; i < box_count; i++)
            boxes[i] = boxes[i + 1];
    }
    return index;
}

int realloc_boxes() {
    boxes =
        realloc(boxes, (boxes_allocated_size + BOXES_BLOCK) * sizeof(box_t *));
    if (boxes == NULL)
        return -1;

    box_mutex = realloc(box_mutex, (boxes_allocated_size + BOXES_BLOCK) *
                                       sizeof(pthread_mutex_t));
    if (box_mutex == NULL)
        return -1;

    box_cond = realloc(box_cond, (boxes_allocated_size + BOXES_BLOCK) *
                                     sizeof(pthread_cond_t));
    if (box_cond == NULL)
        return -1;

    for (size_t i = 0; i < BOXES_BLOCK; i++) {
        MUTEX_INIT(box_mutex + boxes_allocated_size + i);
        COND_INIT(box_cond + boxes_allocated_size + i);
    }

    boxes_allocated_size += BOXES_BLOCK;
    return 0;
}

int add_box(char *path) {
    if (box_count + 1 > boxes_allocated_size)
        ALWAYS_ASSERT(realloc_boxes() == 0, "no memory");

    box_t *box = create_box(path);
    boxes[box_count++] = box;
    return box_count - 1;
}

void publisher_session(char *fifo_path, int id) {
    char buffer[MESSAGE_SIZE];
    char contents[CONTENTS_SIZE];
    for (;;) {
        memset(buffer, '\0', MESSAGE_SIZE);
        memset(contents, '\0', CONTENTS_SIZE);

        // Reading one message from the FIFO
        int pipe_fd = open(fifo_path, O_RDONLY);
        read(pipe_fd, buffer, MESSAGE_SIZE);
        close(pipe_fd);

        // Processing the received message
        int op_code;
        size_t message_size;
        parse_message(buffer, &op_code, contents, &message_size);

        // Writing the contents to tfs
        int tfs_fd = tfs_open(boxes[id]->path, TFS_O_APPEND);
        ssize_t written_size = tfs_write(tfs_fd, contents, message_size);
        ALWAYS_ASSERT(written_size != -1, "tfs_write critical error");
        tfs_close(tfs_fd);

        // Updating the box
        boxes[id]->messages[boxes[id]->message_count] = (size_t)written_size;
        boxes[id]->size += (size_t)written_size;
        boxes[id]->message_count++;

        // Notifying all subscribers
        pthread_cond_broadcast(box_cond + id);
    }
}

void subscriber_session(char *fifo_path, int id) {
    size_t message_count = 0;
    // Nao quero criar mas tmb nao ha tfs RDONLY, so provisorio
    int tfs_fd = tfs_open(boxes[id]->path, TFS_O_CREAT);

    char buffer[MESSAGE_SIZE];
    char contents[CONTENTS_SIZE];
    for (;;) {
        memset(buffer, '\0', MESSAGE_SIZE);
        memset(contents, '\0', CONTENTS_SIZE);

        MUTEX_LOCK(box_mutex + id);

        while (message_count == boxes[id]->message_count)
            pthread_cond_wait(box_cond + id, box_mutex + id);

        MUTEX_UNLOCK(box_mutex + id);

        // Reading one message's contents from tfs
        ssize_t result =
            tfs_read(tfs_fd, contents, boxes[id]->messages[message_count]);
        ALWAYS_ASSERT(result != -1, "tfs_read critical error");

        // Creating the message
        create_message(buffer, SUBSCRIBER_MESSAGE, contents);

        // Sending the message through the FIFO
        int pipe_fd = open(fifo_path, O_WRONLY);
        write(pipe_fd, buffer, MESSAGE_SIZE);
        close(pipe_fd);

        message_count++;
    }
    tfs_close(tfs_fd);
}

void *consumer() {
    for (;;) {
        char *request = (char *)pcq_dequeue(pc_queue);
        ALWAYS_ASSERT(request != NULL, "producer-consumer critical error");
        int op_code;
        char fifo[PIPE_SIZE], box_path[BOX_PATH_SIZE];
        parse_request(request, &op_code, fifo, box_path);

        int id;
        switch (op_code) {
        case PUBLISHER:
            // If the box doesnt exist, reject the publisher
            if ((id = box_lookup(box_path)) == -1)
                break; // Esta implemencatacao e so provisoria
            publisher_session(fifo, id);
            break;
        case SUBSCRIBER:
            // If the box doesnt exist, reject the subscribers
            if ((id = box_lookup(box_path)) == -1)
                break; // Esta implemencatacao e so provisoria
            subscriber_session(fifo, id);
            break;
        default:
            break;
        }
    }
    return NULL;
}

void server_destroy(char *fifo_path) {
    MK_FIFO(fifo_path);

    ALWAYS_ASSERT(tfs_destroy() == 0, "tfs_destroy critical error");

    for (size_t i = 0; i < boxes_allocated_size; i++) {
        MUTEX_DESTROY(box_mutex + i);
        COND_DESTROY(box_cond + i);
        free(boxes[i]);
    }

    pcq_destroy(pc_queue);

    free(boxes);
    free(box_mutex);
    free(box_cond);
    free(pc_queue);
}

void server_init(char *fifo_path, pthread_t *threads, size_t max_sessions) {
    MK_FIFO(fifo_path);

    ALWAYS_ASSERT(tfs_init(NULL) == 0, "tfs_init critical error");

    boxes = malloc(BOXES_BLOCK * sizeof(box_t *));
    ALWAYS_ASSERT(boxes != NULL, "no memory");

    box_mutex = malloc(BOXES_BLOCK * sizeof(pthread_cond_t));
    ALWAYS_ASSERT(box_mutex != NULL, "no memory");

    box_cond = malloc(BOXES_BLOCK * sizeof(pthread_cond_t));
    ALWAYS_ASSERT(box_cond != NULL, "no memory");

    for (size_t i = 0; i < BOXES_BLOCK; i++) {
        MUTEX_INIT(box_mutex + i);
        COND_INIT(box_cond + i);
    }

    pc_queue = malloc(sizeof(pc_queue_t));
    ALWAYS_ASSERT(pcq_create(pc_queue, max_sessions) != -1,
                  "producer-consumer critical error");

    boxes_allocated_size = BOXES_BLOCK;

    for (size_t i = 0; i < max_sessions; i++)
        THREAD_CREATE(threads + i, consumer);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: mbroker <pipename> <max_sessions>\n");

    char *fifo_path = argv[1], *endptr;

    errno = 0;
    size_t sessions = strtoul(argv[2], &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != argv[2],
                  "invalid max_sessions value\n");

    pthread_t threads[sessions];
    server_init(argv[1], threads, sessions);

    add_box("/hello_world");

    for (;;) {
        char buffer[REQUEST_SIZE];
        int fifo = open(fifo_path, O_RDONLY);
        read(fifo, buffer, REQUEST_SIZE);
        close(fifo);
        ALWAYS_ASSERT(pcq_enqueue(pc_queue, buffer) != -1,
                      "producer-consumer critical error");
    }
    return 0;
}
