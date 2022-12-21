#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    print_usage();
    WARN("unimplemented"); // TODO: implement
    return -1;
}
