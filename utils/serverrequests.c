#include "serverrequests.h"
#include "betterassert.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box,
                    size_t pipe_size, size_t box_size) {

    char pipe_name[PIPE_SIZE], box_name[BOX_SIZE], op_code_str[OP_CODE_SIZE];

    snprintf(op_code_str, OP_CODE_SIZE, "%d", op_code);

    strncpy(pipe_name, session_pipe, pipe_size);
    memset(pipe_name + pipe_size, '\0', PIPE_SIZE - pipe_size);

    strncpy(box_name, box, box_size);
    memset(box_name + box_size, '\0', BOX_SIZE - box_size);

    memcpy(dest, op_code_str, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, pipe_name, PIPE_SIZE);
    memcpy(dest + OP_CODE_SIZE + PIPE_SIZE, box_name, BOX_SIZE);
}

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path) {
    errno = 0;
    char *endptr, box_name[BOX_SIZE];
    *op_code = (int)strtol(request, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != request, "invalid op_code\n");

    strncpy(session_pipe, request + OP_CODE_SIZE, PIPE_SIZE-1);

    strncpy(box_name, request + OP_CODE_SIZE + PIPE_SIZE, BOX_SIZE-1);
    snprintf(box_path, BOX_PATH_SIZE, "/%s", box_name);
}