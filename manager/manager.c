#include "logging.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

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

    if (argc == 4 && !strcmp(argv[2], "create")) {
        // create
    } else if (argc == 4 && !strcmp(argv[2], "remove")) {
        // remove
    } else if (argc == 3 && !strcmp(argv[2], "list")) {
        // list
    } else {
        print_usage();
        exit(EXIT_FAILURE);
    }
}
