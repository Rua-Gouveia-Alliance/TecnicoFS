#include "../clients/opcodes.h"
#include "../utils/betterassert.h"
#include "../utils/betterpipes.h"
#include "../utils/generatepath.h"
#include "../utils/serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

size_t m_count = 0;
char path[PIPE_PATH_SIZE];

void handle_sigint() {
    fprintf(stdout, "\n%lu\n", m_count);
    unlink(path);
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: sub <register_pipe> <box_name>\n");
    char *box_name = argv[2];
    ALWAYS_ASSERT(strlen(box_name) < BOX_NAME_SIZE, "invalid box name\n");

    // Setting up SIGINT handling
    struct sigaction act;
    act.sa_handler = &handle_sigint;
    sigaction(SIGINT, &act, NULL);

    // Creating the FIFO
    generate_path(path);
    MK_FIFO(path);

    // Creating the string that will be sent to the server and send it
    char request[REQUEST_SIZE];
    create_request(request, SUBSCRIBER, path, box_name);
    ALWAYS_ASSERT(send_content(argv[1], request, REQUEST_SIZE) != -1,
                  "critical error sending request");

    // Reading to stdout what is sent through the FIFO
    char message[MESSAGE_SIZE];
    for (;;) {
        memset(message, '\0', MESSAGE_CONTENT_SIZE);
        ALWAYS_ASSERT(receive_content(path, message, MESSAGE_SIZE) != -1,
                      "critical error receiving message");

        // Printing the received message
        int op_code;
        size_t size;
        char buffer[MESSAGE_CONTENT_SIZE];
        parse_message(message, &op_code, buffer, &size);

        fprintf(stdout, "%s\n", buffer);
        m_count++;
    }

    return 0;
}
