#include "betterassert.h"
#include "betterpipes.h"
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

size_t m_count = 0;
char *path;

void finish_subscriber(int sig) {
    fprintf(stdout, "\n%lu\n", m_count);
    unlink(path);
    if (sig == SIGINT)
        exit(EXIT_SUCCESS);
    exit(sig);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 4,
                  "usage: sub <register_pipe> <pipe_name> <box_name>\n");
    char *box_name = argv[3];
    ALWAYS_ASSERT(strlen(box_name) < BOX_NAME_SIZE, "invalid box name\n");

    // Setting up SIGINT handling
    struct sigaction act;
    act.sa_handler = &finish_subscriber;
    sigaction(SIGINT, &act, NULL);

    // Creating the FIFO
    path = argv[2];
    MK_FIFO(path);

    // Creating the string that will be sent to the server and send it
    char request[REQUEST_SIZE];
    create_request(request, SUBSCRIBER, path, box_name);
    if (send_content(argv[1], request, REQUEST_SIZE) == -1) {
        fprintf(stdout, "%s\n", SERVER_ERROR);
        finish_subscriber(EXIT_FAILURE);
    }

    // Reading to stdout what is sent through the FIFO
    char message[MESSAGE_SIZE];
    for (;;) {
        memset(message, '\0', MESSAGE_CONTENT_SIZE);

        if (receive_content(path, message, MESSAGE_SIZE) == -1)
            finish_subscriber(EXIT_FAILURE);

        // Printing the received message
        uint8_t op_code;
        char buffer[MESSAGE_CONTENT_SIZE];
        parse_message(message, &op_code, buffer);

        if (op_code == ERROR_CODE) {
            finish_subscriber(EXIT_FAILURE);
        }

        if (op_code != SUBSCRIBER_MESSAGE) {
            fprintf(stdout, "%s\n", OP_CODE_DIFF);
            finish_subscriber(EXIT_FAILURE);
        }

        fprintf(stdout, "%s\n", buffer);
        m_count++;
    }

    finish_subscriber(EXIT_SUCCESS);

    return 0;
}
