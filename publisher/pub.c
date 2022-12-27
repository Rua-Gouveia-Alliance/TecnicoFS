#include "betterassert.h"
#include "betterpipes.h"
#include "clientconfig.h"
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
    size_t box_name_size = strlen(box_name) + 1;
    ALWAYS_ASSERT(box_name_size < 33, "invalid box name\n");

    // Creating the FIFO, using PID to guarantee that the name is unique
    pid_t pid;
    size_t path_size = get_pid(&pid) + 6;
    char path[path_size];
    snprintf(path, path_size, "%d.pipe", pid);
    MK_FIFO(path);

    // Creating the string that will be sent to the server
    char request[REQUEST_SIZE];
    create_request(request, PUBLISHER, path, box_name, path_size,
                   box_name_size);

    // Opening the server FIFO and sending the request
    int fifo_fd = open(argv[1], O_WRONLY);
    ALWAYS_ASSERT(fifo_fd != -1, "invalid register_pipe");
    write(fifo_fd, request, REQUEST_SIZE);

    // Opening the FIFO that communicates with the server
    fifo_fd = open(path, O_WRONLY);
    ALWAYS_ASSERT(fifo_fd != -1, "opening pipe critical error");

    // Reading from stdin and sending it through the fifo
    char buffer[MESSAGE_SIZE];
    for (size_t i = 0; i < 2; i++) {
        int size;
        size = (int)read(STDIN_FILENO, buffer, 5);
        ALWAYS_ASSERT(size > 0, "reading error");
        memset(buffer + (size_t)size, '\0', MESSAGE_SIZE - (size_t)size);
        write(fifo_fd, buffer, MESSAGE_SIZE);
    }

    return 0;
}
