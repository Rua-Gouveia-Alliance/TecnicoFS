#include "betterassert.h"
#include "betterpipes.h"
#include "generatepath.h"
#include "messages.h"
#include "opcodes.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char path[PIPE_PATH_SIZE];

void finish_publisher(int sig) {
    unlink(path);
    if (sig == SIGINT)
        exit(EXIT_SUCCESS);
    exit(sig);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: pub <register_pipe> <box_name>");
    char *box_name = argv[2];
    ALWAYS_ASSERT(strlen(box_name) < BOX_NAME_SIZE, "invalid box name");

    // Setting up SIGINT handling
    struct sigaction act;
    act.sa_handler = &finish_publisher;
    sigaction(SIGINT, &act, NULL);

    // Creating the FIFO
    generate_path(path);
    MK_FIFO(path);

    // Creating the string that will be sent to the server and send
    char request[REQUEST_SIZE];
    create_request(request, PUBLISHER, path, box_name);
    if (send_content(argv[1], request, REQUEST_SIZE) == -1) {
        fprintf(stdout, "invalid register pipe name\n");
        finish_publisher(EXIT_FAILURE);
    }

    // Reading from stdin and sending it through the fifo
    char buffer[MESSAGE_CONTENT_SIZE];
    for (;;) {
        memset(buffer, '\0', MESSAGE_CONTENT_SIZE);
        ssize_t read_result = read(STDIN_FILENO, buffer, MESSAGE_CONTENT_SIZE);

        if (read_result < 1)
            finish_publisher(EXIT_SUCCESS);

        // Removing newline
        size_t size = (size_t)read_result;
        buffer[size - 1] = '\0';

        // Create and sending message to server
        char message[MESSAGE_SIZE];
        create_message(message, PUBLISHER_MESSAGE, buffer);

        if (send_content(path, message, MESSAGE_SIZE) == -1)
            finish_publisher(EXIT_FAILURE);
    }

    finish_publisher(EXIT_SUCCESS);

    return 0;
}
