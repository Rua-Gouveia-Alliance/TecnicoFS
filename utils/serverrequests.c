#include "serverrequests.h"
#include "clientconfig.h"
#include "betterassert.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

void create_request(char *dest, int opcode, char *session_pipe, char *box,
                    size_t pipe_size, size_t box_size) {

    char pipe_name[PIPE_SIZE], box_name[BOX_SIZE];

    strncpy(pipe_name, session_pipe, pipe_size);
    memset(pipe_name + pipe_size, '\0', PIPE_SIZE - pipe_size);

    strncpy(box_name, box, box_size);
    memset(box_name + box_size, '\0', BOX_SIZE - box_size);

    snprintf(dest, REQUEST_SIZE, "%d %s %s", opcode, pipe_name, box_name);
}

void parse_request(char *request, int *op_code, char *session_pipe, char *box_name) {
    errno = 0;
    char *endptr;
    *op_code = (int)strtol(request, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != request, "invalid op_code\n");

    strncpy(session_pipe, request + 2, PIPE_SIZE);

    snprintf(box_name, 2, "%s", "/");
    strncpy(box_name + 1, request + 2 + PIPE_SIZE, BOX_SIZE);
}