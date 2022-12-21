#include "logging.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}
