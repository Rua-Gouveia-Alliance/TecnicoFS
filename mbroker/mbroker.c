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
#include <signal.h>
#include <stdint.h>
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
    size_t messages_size[MAX_MESSAGES];
    uint64_t message_count;
    uint64_t n_publishers;
    uint64_t n_subscribers;
    size_t size;
} box_t;

box_t **boxes;
int *free_box;
int box_count = 0;
pthread_cond_t *box_cond;
pthread_mutex_t *box_mutex;
pthread_rwlock_t *box_rwl;
pthread_mutex_t box_crdel_mutex;
size_t boxes_allocated_size;
pc_queue_t *pc_queue;
char *path;

void finish_mbroker(int sig) {
    unlink(path);
    if (sig == SIGINT)
        exit(EXIT_SUCCESS);
    exit(sig);
}

box_t *create_box(char *box_path) {
    box_t *box = malloc(sizeof(box_t));
    if (box == NULL) {
        fprintf(stdout, "create_box: out of memory");
        finish_mbroker(EXIT_FAILURE);
    }

    int fd = tfs_open(box_path, TFS_O_CREAT | TFS_O_TRUNC);
    if (fd == -1)
        return NULL;
    tfs_close(fd);

    box->size = 0;
    box->n_publishers = 0;
    box->n_subscribers = 0;
    box->message_count = 0;

    strncpy(box->path, box_path, BOX_PATH_SIZE - 1);
    box->path[BOX_PATH_SIZE - 1] = '\0';

    return box;
}

int box_lookup(char *box_path) {
    MUTEX_LOCK(&box_crdel_mutex);
    for (int i = 0; i < boxes_allocated_size; i++)
        if (!free_box[i] &&
            strncmp(box_path, boxes[i]->path, BOX_PATH_SIZE) == 0) {
            MUTEX_UNLOCK(&box_crdel_mutex);
            return i;
        }
    MUTEX_UNLOCK(&box_crdel_mutex);
    return -1;
}

int remove_box(char *box_path) {
    int index = box_lookup(box_path);
    MUTEX_LOCK(&box_crdel_mutex);
    if (index == -1) {
        MUTEX_UNLOCK(&box_crdel_mutex);
        return -1;
    }

    tfs_unlink(box_path);
    free(boxes[index]);
    free_box[index] = 1;
    box_count--;

    MUTEX_UNLOCK(&box_crdel_mutex);
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

    box_rwl = realloc(box_rwl, (boxes_allocated_size + BOXES_BLOCK) *
                                   sizeof(pthread_rwlock_t));
    if (box_rwl == NULL)
        return -1;

    free_box =
        realloc(free_box, (boxes_allocated_size + BOXES_BLOCK) * sizeof(int));
    if (free_box == NULL)
        return -1;

    for (size_t i = 0; i < BOXES_BLOCK; i++) {
        if (pthread_mutex_init(box_mutex + boxes_allocated_size + i, NULL) !=
            0) {
            fprintf(stdout, "pthread_mutex_init critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        if (pthread_cond_init(box_cond + boxes_allocated_size + i, NULL) != 0) {
            fprintf(stdout, "pthread_cond_init critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        if (pthread_rwlock_init(box_rwl + boxes_allocated_size + i, NULL) !=
            0) {
            fprintf(stdout, "pthread_rwlock_init critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        free_box[boxes_allocated_size + i] = 1;
    }

    boxes_allocated_size += BOXES_BLOCK;
    return 0;
}

int add_box(char *box_path) {
    MUTEX_LOCK(&box_crdel_mutex);
    if (box_count + 1 > boxes_allocated_size) {
        if (realloc_boxes() != 0) {
            fprintf(stdout, "no memory\n");
            MUTEX_UNLOCK(&box_crdel_mutex);
            finish_mbroker(EXIT_FAILURE);
        }
    }
    MUTEX_UNLOCK(&box_crdel_mutex);
    box_t *box = create_box(box_path);
    if (box == NULL)
        return -1;

    MUTEX_LOCK(&box_crdel_mutex);
    int i;
    for (i = 0; i < boxes_allocated_size; i++) {
        if (free_box[i]) {
            boxes[i] = box;
            free_box[i] = 0;
            box_count++;
            break;
        }
    }
    MUTEX_UNLOCK(&box_crdel_mutex);
    return i;
}

void box_list_session(char *fifo_path) {
    char response[LIST_RESPONSE_SIZE];

    // if no boxes exist send response with last=1 and box_name with just '\0's
    if (box_count == 0) {
        char empty_name[BOX_NAME_SIZE];
        memset(empty_name, '\0', BOX_NAME_SIZE);
        create_list_response(response, 1, empty_name, 0, 0, 0);
        send_content(fifo_path, response, LIST_RESPONSE_SIZE);
    }

    for (size_t i = 0; i < box_count; i++) {
        // create and send response, box path +1 to remove the '/'
        create_list_response(response, i == box_count - 1, boxes[i]->path + 1,
                             boxes[i]->message_count, boxes[i]->n_publishers,
                             boxes[i]->n_subscribers);
        send_content(fifo_path, response, LIST_RESPONSE_SIZE);
    }
}

void box_creation_session(char *fifo_path, char *box_path) {
    char response[RESPONSE_SIZE];
    if (add_box(box_path) == -1) {
        create_response(response, BOX_CREATION_ANS, -1,
                        "failed saving box to tfs");
    } else {
        create_response(response, BOX_CREATION_ANS, 0, "");
    }
    send_content(fifo_path, response, RESPONSE_SIZE);
}

void box_deletion_session(char *fifo_path, char *box_path) {
    char response[RESPONSE_SIZE];
    if (remove_box(box_path) == -1) {
        create_response(response, BOX_DELETION_ANS, -1, "box doesnt exist");
    } else {
        create_response(response, BOX_DELETION_ANS, 0, "");
    }
    send_content(fifo_path, response, RESPONSE_SIZE);
}

void publisher_session(char *fifo_path, char *box_path) {
    char buffer[MESSAGE_SIZE];
    char contents[MESSAGE_CONTENT_SIZE];

    int id = box_lookup(box_path);
    if (id == -1) {
        unlink(fifo_path); // for the publisher to know there was an error
        return;
    }

    if (pthread_rwlock_wrlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    // stop if box already has a publisher
    if (boxes[id]->n_publishers > 0) {
        unlink(fifo_path); // for the publisher to know there was an error

        if (pthread_rwlock_unlock(box_rwl + id) != 0) {
            fprintf(stdout, "pthread_rwlock_unlock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        return;
    }
    boxes[id]->n_publishers++;

    if (pthread_rwlock_unlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_unlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    for (;;) {
        memset(buffer, '\0', MESSAGE_SIZE);

        // If the pipe has been closed the session ends
        if (receive_content(fifo_path, buffer, MESSAGE_SIZE) == -1)
            break;

        // Processing the received message
        uint8_t op_code;
        parse_message(buffer, &op_code, contents);
        if (op_code == ERROR_CODE) {
            break;
        }

        // If the box has been deleted the session ends
        int tfs_fd = tfs_open(box_path, TFS_O_APPEND);
        if (tfs_fd == -1) {
            unlink(fifo_path); // for the subscriber to know there was an error
            break;
        }

        if (pthread_rwlock_wrlock(box_rwl + id) != 0) {
            fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        // Writing the contents to tfs
        size_t message_size = strlen(contents) + 1; // +1 for the \0
        ssize_t written_size = tfs_write(tfs_fd, contents, message_size);
        tfs_close(tfs_fd);

        // If writing to the box fails its because its been deleted
        if (written_size == -1 || written_size < message_size) {
            unlink(fifo_path); // for the subscriber to know there was an error
            if (pthread_rwlock_unlock(box_rwl + id) != 0) {
                fprintf(stdout, "pthread_rwlock_unlock critical error\n");
                finish_mbroker(EXIT_FAILURE);
            }
            break;
        }

        // Updating the box
        boxes[id]->messages_size[boxes[id]->message_count++] =
            (size_t)written_size;
        boxes[id]->size += (size_t)written_size;

        if (pthread_rwlock_unlock(box_rwl + id) != 0) {
            fprintf(stdout, "pthread_rwlock_unlock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        // Notifying all subscribers
        pthread_cond_broadcast(box_cond + id);
    }

    if (pthread_rwlock_wrlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    boxes[id]->n_publishers--;

    if (pthread_rwlock_unlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_unlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }
}

void subscriber_session(char *fifo_path, char *box_path) {
    char buffer[MESSAGE_SIZE];
    char contents[MESSAGE_CONTENT_SIZE];

    int id = box_lookup(box_path);
    if (id == -1) {
        // Sending error notification
        memset(buffer, '\0', MESSAGE_SIZE);
        memset(contents, '\0', MESSAGE_CONTENT_SIZE);

        create_message(buffer, ERROR_CODE, contents);
        send_content(fifo_path, buffer, MESSAGE_SIZE);
        return;
    }

    int tfs_fd = tfs_open(box_path, 0);
    if (tfs_fd == -1) {
        // Sending error notification
        memset(buffer, '\0', MESSAGE_SIZE);
        memset(contents, '\0', MESSAGE_CONTENT_SIZE);

        create_message(buffer, ERROR_CODE, contents);
        send_content(fifo_path, buffer, MESSAGE_SIZE);
        return;
    }

    if (pthread_rwlock_wrlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    boxes[id]->n_subscribers++;

    if (pthread_rwlock_unlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_unlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    int64_t m_count = 0;
    for (;;) {
        memset(buffer, '\0', MESSAGE_SIZE);
        memset(contents, '\0', MESSAGE_CONTENT_SIZE);

        if (pthread_mutex_lock(box_mutex + id) != 0) {
            fprintf(stdout, "pthread_mutex_lock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        for (;;) {
            if (pthread_rwlock_rdlock(box_rwl + id) != 0) {
                fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
                finish_mbroker(EXIT_FAILURE);
            }

            if (m_count >= boxes[id]->message_count) {
                if (pthread_rwlock_unlock(box_rwl + id) != 0) {
                    fprintf(stdout, "pthread_rwlock_unlock critical error\n");
                    finish_mbroker(EXIT_FAILURE);
                }
                pthread_cond_wait(box_cond + id, box_mutex + id);
            } else
                break;
        }

        if (pthread_mutex_unlock(box_mutex + id) != 0) {
            fprintf(stdout, "pthread_mutex_unlock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        // If reading from the box fails its because its been deleted
        if (tfs_read(tfs_fd, contents, boxes[id]->messages_size[m_count++]) ==
            -1) {
            // Sending error notification
            memset(buffer, '\0', MESSAGE_SIZE);
            memset(contents, '\0', MESSAGE_CONTENT_SIZE);

            create_message(buffer, ERROR_CODE, contents);
            send_content(fifo_path, buffer, MESSAGE_SIZE);

            if (pthread_rwlock_unlock(box_rwl + id) != 0) {
                fprintf(stdout, "pthread_rwlock_unlock critical error\n");
                finish_mbroker(EXIT_FAILURE);
            }

            break;
        }

        if (pthread_rwlock_unlock(box_rwl + id) != 0) {
            fprintf(stdout, "pthread_rwlock_unlock critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }

        // Creating and sending the message
        create_message(buffer, SUBSCRIBER_MESSAGE, contents);

        // If the pipe has been closed the session ends
        if (send_content(fifo_path, buffer, MESSAGE_SIZE) == -1)
            break;
    }

    tfs_close(tfs_fd);

    if (pthread_rwlock_wrlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_wrlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }

    boxes[id]->n_subscribers--;

    if (pthread_rwlock_unlock(box_rwl + id) != 0) {
        fprintf(stdout, "pthread_rwlock_unlock critical error\n");
        finish_mbroker(EXIT_FAILURE);
    }
}

void *consumer() {
    for (;;) {
        char *request = (char *)pcq_dequeue(pc_queue);
        if (request == NULL) {
            fprintf(stdout, "producer-consumer critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }
        uint8_t op_code;
        char fifo[PIPE_PATH_SIZE], box_name[BOX_NAME_SIZE];
        parse_request(request, &op_code, fifo, box_name);
        free(request);

        char box_path[BOX_PATH_SIZE];
        snprintf(box_path, BOX_PATH_SIZE, "/%s", box_name);

        switch (op_code) {
        case PUBLISHER:
            publisher_session(fifo, box_path);
            break;
        case SUBSCRIBER:
            subscriber_session(fifo, box_path);
            break;
        case BOX_CREATION:
            box_creation_session(fifo, box_path);
            break;
        case BOX_DELETION:
            box_deletion_session(fifo, box_path);
            break;
        case BOX_LIST:
            box_list_session(fifo);
            break;
        default:
            break;
        }
    }
    return NULL;
}

void server_init(pthread_t *threads, size_t max_sessions) {
    ALWAYS_ASSERT(tfs_init(NULL) == 0, "tfs_init critical error");

    boxes = malloc(BOXES_BLOCK * sizeof(box_t *));
    ALWAYS_ASSERT(boxes != NULL, "no memory");

    free_box = malloc(BOXES_BLOCK * sizeof(int));
    ALWAYS_ASSERT(free_box != NULL, "no memory");

    box_mutex = malloc(BOXES_BLOCK * sizeof(pthread_mutex_t));
    ALWAYS_ASSERT(box_mutex != NULL, "no memory");

    box_cond = malloc(BOXES_BLOCK * sizeof(pthread_cond_t));
    ALWAYS_ASSERT(box_cond != NULL, "no memory");

    box_rwl = malloc(BOXES_BLOCK * sizeof(pthread_rwlock_t));
    ALWAYS_ASSERT(box_rwl != NULL, "no memory");

    for (size_t i = 0; i < BOXES_BLOCK; i++) {
        MUTEX_INIT(box_mutex + i);
        COND_INIT(box_cond + i);
        RWLOCK_INIT(box_rwl + i);
        free_box[i] = 1;
    }

    MUTEX_INIT(&box_crdel_mutex);
    pc_queue = malloc(sizeof(pc_queue_t));
    ALWAYS_ASSERT(pcq_create(pc_queue, max_sessions) != -1,
                  "producer-consumer critical error");

    boxes_allocated_size = BOXES_BLOCK;

    for (size_t i = 0; i < max_sessions; i++)
        THREAD_CREATE(threads + i, consumer);

    MK_FIFO(path);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: mbroker <pipename> <max_sessions>\n");

    path = argv[1];
    char *endptr;

    errno = 0;
    size_t sessions = strtoul(argv[2], &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != argv[2],
                  "invalid max_sessions value\n");

    pthread_t threads[sessions];
    server_init(threads, sessions);

    for (;;) {
        char *buffer = malloc(REQUEST_SIZE);
        if (receive_content(path, buffer, REQUEST_SIZE) == -1) {
            fprintf(stdout, "fifo was deleted\n");
            finish_mbroker(EXIT_FAILURE);
        }
        if (pcq_enqueue(pc_queue, buffer) == -1) {
            fprintf(stdout, "producer-consumer critical error\n");
            finish_mbroker(EXIT_FAILURE);
        }
    }
    return 0;
}
