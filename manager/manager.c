#include "../clients/opcodes.h"
#include "../utils/betterpipes.h"
#include "../utils/generatepath.h"
#include "../utils/serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int comparator(const void *a, const void *b) {
    return strcmp((char *)a, (char *)b) > 0;
}

void list_boxes_request(char *request, char *server_fifo, char *fifo) {
    char box[BOX_NAME_SIZE];
    memset(box, '\0', BOX_NAME_SIZE);
    create_request(request, BOX_LIST, fifo, box);
    ALWAYS_ASSERT(send_content(server_fifo, request, REQUEST_SIZE) != -1,
                  "server fifo critical error");

    char response[RESPONSE_SIZE], box_name[BOX_NAME_SIZE];
    int op_code, last;
    size_t box_size, n_publishers, n_subscribers;
    do {
        ALWAYS_ASSERT(receive_content(fifo, response, RESPONSE_SIZE) != -1,
                      "server fifo critical error");
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
        print_usage();
        exit(EXIT_FAILURE);
    }

    // Creating the FIFO
    char path[PIPE_PATH_SIZE];
    generate_path(path);
    MK_FIFO(path);

    char request[REQUEST_SIZE];
    int response_opcode;
    if (argc == 4 && !strcmp(argv[2], "create")) {
        create_request(request, BOX_CREATION, path, argv[3]);
        response_opcode = BOX_CREATION_ANS;
    } else if (argc == 4 && !strcmp(argv[2], "remove")) {
        create_request(request, BOX_DELETION, path, argv[3]);
        response_opcode = BOX_DELETION_ANS;
    } else if (argc == 3 && !strcmp(argv[2], "list")) {
        list_boxes_request(request, argv[1], path);
        return 0;
    } else {
        print_usage();
        exit(EXIT_FAILURE);
    }

    ALWAYS_ASSERT(send_content(argv[1], request, REQUEST_SIZE) != -1,
                  "server fifo critical error");

    char response[RESPONSE_SIZE], error[ERROR_SIZE];
    int op_code, return_code;
    ALWAYS_ASSERT(receive_content(path, response, RESPONSE_SIZE) != -1,
                  "server fifo critical error");
    parse_response(response, &op_code, &return_code, error);

    ALWAYS_ASSERT(op_code == response_opcode,
                  "server response doesnt match request");
    if (return_code == -1) {
        printf("server error: %s", error);
        return -1;
    }
    return 0;
}
