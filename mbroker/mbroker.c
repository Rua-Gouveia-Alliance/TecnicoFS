#include "logging.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    fprintf(stderr, "usage: mbroker <pipename>\n");
    WARN("unimplemented"); // TODO: implement
    return -1;
}
