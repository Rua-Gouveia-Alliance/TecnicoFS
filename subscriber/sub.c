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
    ALWAYS_ASSERT(strlen(box_name) < BOX_NAME_SIZE, "invalid box name\n");

    // Creating the FIFO
    char path[PIPE_SIZE];
    generate_path(path);
    MK_FIFO(path);

    // Creating the string that will be sent to the server and send it
    char request[REQUEST_SIZE];
    create_request(request, SUBSCRIBER, path, box_name);
    ALWAYS_ASSERT(receive_content(argv[1], request, REQUEST_SIZE),
                  "critical error sending request");

    // Reading to stdout what is sent through the FIFO
    char message[MESSAGE_SIZE];
    for (;;) {
        memset(message, '\0', CONTENTS_SIZE);
        ALWAYS_ASSERT(receive_content(path, message, MESSAGE_SIZE) != -1,
                      "critical error receiving message");

        // Printing the received message
        int op_code;
        size_t size;
        char buffer[MESSAGE_CONTENT_SIZE];
        parse_message(message, &op_code, buffer, &size);

        fprintf(stdout, "%s\n", message);
    }

    return 0;
}
