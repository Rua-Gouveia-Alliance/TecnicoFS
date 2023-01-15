#include "betterpipes.h"
#include "messages.h"
#include "opcodes.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 100
#define USAGE                                                                  \
    "usage:\n"                                                                 \
    "manager <register_pipe_name> <pipe_name> create <box_name>\n"             \
    "manager <register_pipe_name> <pipe_name> remove <box_name>\n"             \
    "manager <register_pipe_name> <pipe_name> list"

char *path;

void finish_manager(int sig) {
    // delete pipe
    unlink(path);
    if (sig == SIGINT)
        exit(EXIT_SUCCESS);
    exit(sig);
}

int comparator(const void *r1, const void *r2) {
    return strcmp((char *)(r1 + LIST_RESPONSE_NAME_OFFSET),
                  (char *)(r2 + LIST_RESPONSE_NAME_OFFSET));
}

void list_boxes_request(char *request, char *server_fifo, char *fifo) {
    // create request
    char box[BOX_NAME_SIZE];
    memset(box, '\0', BOX_NAME_SIZE);
    create_request(request, BOX_LIST, fifo, box);

    // send request
    if (send_content(server_fifo, request, REQUEST_SIZE) == -1) {
        fprintf(stdout, "%s\n", SERVER_ERROR);
        finish_manager(EXIT_FAILURE);
    }

    char **responses, box_name[BOX_NAME_SIZE];
    responses = malloc(BUFFER_SIZE);
    uint8_t op_code, last;
    uint64_t box_size, n_publishers, n_subscribers;
    size_t b_count = 0;
    do {
        // receive response
        responses[b_count] = malloc(LIST_RESPONSE_SIZE);
        if (receive_content(fifo, responses[b_count], LIST_RESPONSE_SIZE) ==
            -1) {
            fprintf(stdout, "%s\n", SERVER_ERROR);
            finish_manager(EXIT_FAILURE);
        }

        // increment b_count and realloc buffer if necessary (current buffer
        // size exceeded)
        parse_list_response(responses[b_count++], &op_code, &last, box_name,
                            &box_size, &n_publishers, &n_subscribers);
        if (b_count % BUFFER_SIZE == 0) {
            responses =
                realloc(responses, BUFFER_SIZE * (b_count / BUFFER_SIZE + 1));
        }
    } while (!last);

    // if no boxes exist
    if (b_count == 1 && strncmp(responses[0] + LIST_RESPONSE_NAME_OFFSET, "",
                                BOX_NAME_SIZE) == 0) {
        fprintf(stdout, "NO BOXES FOUND\n");
        for (size_t i = 0; i < b_count; i++)
            free(responses[i]);
        free(responses);
        return;
    }

    qsort(responses, b_count, LIST_RESPONSE_SIZE, comparator);

    // write boxes (sorted) to stdout
    for (size_t i = 0; i < b_count; i++) {
        parse_list_response(responses[i], &op_code, &last, box_name, &box_size,
                            &n_publishers, &n_subscribers);

        fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers,
                n_subscribers);
    }

    for (size_t i = 0; i < b_count; i++)
        free(responses[i]);
    free(responses);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stdout, "%s\n", USAGE);
        finish_manager(EXIT_FAILURE);
    }

    // Setting up SIGINT handling
    struct sigaction act;
    act.sa_handler = &finish_manager;
    sigaction(SIGINT, &act, NULL);

    // Creating the FIFO
    path = argv[2];
    MK_FIFO(path);

    char request[REQUEST_SIZE];
    uint8_t ans_op_code;
    if (argc == 5 && !strcmp(argv[3], "create")) {
        // create box request
        create_request(request, BOX_CREATION, path, argv[4]);
        ans_op_code = BOX_CREATION_ANS;
    } else if (argc == 5 && !strcmp(argv[3], "remove")) {
        // remove box request
        create_request(request, BOX_DELETION, path, argv[4]);
        ans_op_code = BOX_DELETION_ANS;
    } else if (argc == 4 && !strcmp(argv[3], "list")) {
        // list box request -> diferent function
        list_boxes_request(request, argv[1], path);
        finish_manager(EXIT_SUCCESS);
    } else {
        fprintf(stdout, "%s\n", USAGE);
        finish_manager(EXIT_FAILURE);
    }

    // send request
    if (send_content(argv[1], request, REQUEST_SIZE) == -1) {
        fprintf(stdout, "%s\n", SERVER_ERROR);
        finish_manager(EXIT_FAILURE);
    }

    // receive response
    char response[RESPONSE_SIZE], error[ERROR_SIZE];
    uint8_t op_code;
    int32_t return_code;
    if (receive_content(path, response, RESPONSE_SIZE) == -1) {
        fprintf(stdout, "%s\n", SERVER_ERROR);
        finish_manager(EXIT_FAILURE);
    }
    parse_response(response, &op_code, &return_code, error);

    // process response and write errors to stdout
    if (op_code != ans_op_code) {
        fprintf(stdout, "ERROR %s\n", OP_CODE_DIFF);
        finish_manager(EXIT_FAILURE);
    }
    if (return_code == -1) {
        fprintf(stdout, "ERROR %s\n", error);
        finish_manager(EXIT_FAILURE);
    }
    fprintf(stdout, "OK\n");
    finish_manager(EXIT_SUCCESS);
    return 0;
}
