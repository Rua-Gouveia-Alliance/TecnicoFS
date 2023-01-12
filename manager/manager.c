#include "betterpipes.h"
#include "generatepath.h"
#include "messages.h"
#include "opcodes.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define USAGE                                                                  \
    "usage:\n"                                                                 \
    "manager <register_pipe_name> create <box_name>\n"                         \
    "manager <register_pipe_name> remove <box_name>\n"                         \
    "manager <register_pipe_name> list"

char path[PIPE_PATH_SIZE];

void finish_manager(int sig) {
    // delete pipe
    unlink(path);
    if (sig == SIGINT)
        exit(EXIT_SUCCESS);
    exit(sig);
}

int comparator(const void *a, const void *b) {
    return strcmp((char *)a, (char *)b) > 0;
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

    char response[RESPONSE_SIZE], box_name[BOX_NAME_SIZE];
    uint8_t op_code, last;
    size_t box_size, n_publishers, n_subscribers;
    do {
        // receive response
        if (receive_content(fifo, response, RESPONSE_SIZE) == -1) {
            fprintf(stdout, "%s\n", SERVER_ERROR);
            finish_manager(EXIT_FAILURE);
        }
        parse_list_response(response, &op_code, &last, box_name, &box_size,
                            &n_publishers, &n_subscribers);

        // TODO: As caixas devem estar ordenadas por ordem alfabética, não sendo
        // garantido que o servidor as envie por essa ordem (i.e., o cliente
        // deve ordenar as caixas antes das imprimir).
        fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers,
                n_subscribers);
    } while (!last);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stdout, "%s\n", USAGE);
        finish_manager(EXIT_FAILURE);
    }

    // Setting up SIGINT handling
    struct sigaction act;
    act.sa_handler = &finish_manager;
    sigaction(SIGINT, &act, NULL);

    // Creating the FIFO
    generate_path(path);
    MK_FIFO(path);

    char request[REQUEST_SIZE];
    uint8_t ans_op_code;
    if (argc == 4 && !strcmp(argv[2], "create")) {
        // create box request
        create_request(request, BOX_CREATION, path, argv[3]);
        ans_op_code = BOX_CREATION_ANS;
    } else if (argc == 4 && !strcmp(argv[2], "remove")) {
        // remove box request
        create_request(request, BOX_DELETION, path, argv[3]);
        ans_op_code = BOX_DELETION_ANS;
    } else if (argc == 3 && !strcmp(argv[2], "list")) {
        // list box request -> diferent function
        list_boxes_request(request, argv[1], path);
        return 0;
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
        fprintf(stdout, "%s\n", OP_CODE_DIFF);
        finish_manager(EXIT_FAILURE);
    }
    if (return_code == -1) {
        fprintf(stdout, "%s\n", error);
        finish_manager(EXIT_FAILURE);
    }
    return 0;
}
