#include "generatepath.h"
#include "serverrequests.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void generate_path(char *dest) {
    // Using PID to guarantee that the name is unique
    pid_t pid = getpid();
    snprintf(dest, PIPE_PATH_SIZE, "%d.pipe", pid);
}