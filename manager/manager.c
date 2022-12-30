#include "betterpipes.h"
#include "clients/opcodes.h"
#include "generatepath.h"
#include "logging.h"
#include "serverrequests.h"
#include "string.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    // Creating the FIFO
    char path[PIPE_SIZE];
    generate_path(path);
    MK_FIFO(path);

    char request[REQUEST_SIZE];
    if (argc == 4 && !strcmp(argv[2], "create"))
        create_request(request, BOX_CREATION, path, argv[3]);
    else if (argc == 4 && !strcmp(argv[2], "remove"))
        create_request(request, BOX_DELETION, path, argv[3]);
    else if (argc == 3 && !strcmp(argv[2], "list"))
        create_list_request(request, path);
    else {
        print_usage();
        exit(EXIT_FAILURE);
    }

    // Opening the server FIFO and sending the request
    int register_fd = open(argv[1], O_WRONLY);
    ALWAYS_ASSERT(register_fd != -1, "invalid register_pipe");
    write(register_fd, request, REQUEST_SIZE);
    close(register_fd);
}
