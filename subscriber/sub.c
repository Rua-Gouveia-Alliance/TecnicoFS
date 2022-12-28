#include "betterassert.h"
#include "betterpipes.h"
#include "generatepath.h"
#include "opcodes.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: sub <register_pipe> <box_name>\n");
    char *box_name = argv[2];
    ALWAYS_ASSERT(strlen(box_name) < BOX_SIZE, "invalid box name\n");

    // Creating the FIFO
    char path[PIPE_SIZE];
    generate_path(path);
    MK_FIFO(path);

    // Creating the string that will be sent to the server
    char request[REQUEST_SIZE];
    create_request(request, SUBSCRIBER, path, box_name);

    // Opening the server FIFO and sending the request
    int register_fd = open(argv[1], O_WRONLY);
    ALWAYS_ASSERT(register_fd != -1, "invalid register_pipe");
    write(register_fd, request, REQUEST_SIZE);
    close(register_fd);

    // Reading to stdout what is sent through the FIFO
    char message[MESSAGE_SIZE];
    for (;;) {
        memset(message, '\0', CONTENTS_SIZE);

        // Opening the FIFO that communicates with the server
        int fifo_fd = open(path, O_RDONLY);
        ALWAYS_ASSERT(fifo_fd != -1, "invalid server pipe");
        read(fifo_fd, message, MESSAGE_SIZE);
        close(fifo_fd);

        // Printing the received message
        int op_code;
        size_t size;
        char buffer[CONTENTS_SIZE];
        parse_message(message, &op_code, buffer, &size);

        printf("%d %s\n", op_code, buffer);
    }

    return 0;
}
