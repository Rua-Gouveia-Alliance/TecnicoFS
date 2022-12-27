#include "betterassert.h"
#include "betterpipes.h"
#include "clientconfig.h"
#include <errno.h>
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
    ALWAYS_ASSERT(argc != 3, "usage: pub <register_pipe> <box_name>\n");
    char *provided_box_name = (char *)argv[2];
    size_t box_name_size = strlen(provided_box_name) + 1;
    ALWAYS_ASSERT(box_name_size < 33, "invalid box name\n");

    // Creating the FIFO, using PID to guarantee that the name is unique
    pid_t pid;
    size_t path_size = get_pid(&pid) + 6;
    char path[path_size];
    snprintf(path, path_size, "%d.pipe", pid);
    MK_FIFO(path);

    // Creating the string that will be sent to the server
    char pipe_name[PIPE_NAME], box_name[BOX_NAME];
    strncpy(pipe_name, path, path_size);
    memset(pipe_name + path_size, '\0', PIPE_NAME - path_size);
    strncpy(box_name, provided_box_name, box_name_size);
    memset(box_name + box_name_size, '\0', BOX_NAME - box_name_size);
    return 0;
}
