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
    ALWAYS_ASSERT(argc == 3, "usage: pub <register_pipe> <box_name>\n");
    char *box_name = argv[2];
    ALWAYS_ASSERT(strlen(box_name) < BOX_NAME_SIZE, "invalid box name\n");

    // Creating the FIFO
    char path[PIPE_PATH_SIZE];
    generate_path(path);
    MK_FIFO(path);

    // Creating the string that will be sent to the server and send
    char request[REQUEST_SIZE];
    create_request(request, PUBLISHER, path, box_name);
    ALWAYS_ASSERT(send_content(argv[1], request, REQUEST_SIZE) != -1,
                  "critical error sending request to server");

    // Reading from stdin and sending it through the fifo
    char buffer[MESSAGE_CONTENT_SIZE];
    for (;;) {
        memset(buffer, '\0', MESSAGE_CONTENT_SIZE);
        ssize_t size = read(STDIN_FILENO, buffer, MESSAGE_CONTENT_SIZE);
        buffer[size - 1] = '\0';

        // Create and send message to server
        char message[MESSAGE_SIZE];
        create_message(message, PUBLISHER_MESSAGE, buffer);
        ALWAYS_ASSERT(send_content(path, request, REQUEST_SIZE) != -1,
                      "critical error sending request to server");
    }

    return 0;
}
