#include "betterassert.h"
#include "betterpipes.h"
#include "opcodes.h"
#include "serverrequests.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

size_t get_pid(pid_t *pid) {
    size_t pid_len;
    pid_t i = getpid();
    *pid = i;
    for (pid_len = 0; i > 0; pid_len++, i /= 10)
        ;
    return pid_len;
}

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "usage: pub <register_pipe> <box_name>\n");
    char *box_name = (char *)argv[2];
    ALWAYS_ASSERT(strlen(box_name) < BOX_SIZE, "invalid box name\n");

    // Creating the FIFO, using PID to guarantee that the name is unique
    pid_t pid;
    size_t path_size = get_pid(&pid) + 6;
    char path[path_size];
    snprintf(path, path_size, "%d.pipe", pid);
    MK_FIFO(path);

    // Creating the string that will be sent to the server
    char request[REQUEST_SIZE];
    create_request(request, PUBLISHER, path, box_name);
    // Opening the server FIFO and sending the request
    int register_fd = open(argv[1], O_WRONLY);
    ALWAYS_ASSERT(register_fd != -1, "invalid register_pipe");
    write(register_fd, request, REQUEST_SIZE);
    close(register_fd);

    // Reading from stdin and sending it through the fifo
    char buffer[CONTENTS_SIZE];
    for (;;) {
        char message[MESSAGE_SIZE];
        memset(buffer, '\0', CONTENTS_SIZE);

        // Opening the FIFO that communicates with the server
        int fifo_fd = open(path, O_WRONLY);
        ALWAYS_ASSERT(fifo_fd != -1, "opening pipe critical error");

        read(STDIN_FILENO, buffer, CONTENTS_SIZE);
        create_message(message, PUBLISHER_MESSAGE, buffer);

        write(fifo_fd, message, MESSAGE_SIZE);

        close(fifo_fd);
    }

    return 0;
}
