#include "serverrequests.h"
#include "betterassert.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box) {

    char pipe_name[PIPE_SIZE], box_name[BOX_SIZE], op_code_str[OP_CODE_SIZE];

    snprintf(op_code_str, OP_CODE_SIZE, "%d", op_code);

    strncpy(pipe_name, session_pipe, PIPE_SIZE - 1);
    pipe_name[PIPE_SIZE - 1] = '\0';

    strncpy(box_name, box, BOX_SIZE - 1);
    box_name[BOX_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, pipe_name, PIPE_SIZE);
    memcpy(dest + OP_CODE_SIZE + PIPE_SIZE, box_name, BOX_SIZE);
}

void create_message(char *dest, int op_code, char *message) {
    char contents[CONTENTS_SIZE], op_code_str[OP_CODE_SIZE];

    snprintf(op_code_str, OP_CODE_SIZE, "%d", op_code);

    strncpy(contents, message, CONTENTS_SIZE - 1);
    contents[CONTENTS_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, contents, CONTENTS_SIZE);
}

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path) {
    errno = 0;
    char *endptr, box_name[BOX_SIZE];
    *op_code = (int)strtol(request, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != request, "invalid op_code\n");

    strncpy(session_pipe, request + OP_CODE_SIZE, PIPE_SIZE - 1);
    session_pipe[PIPE_SIZE - 1] = '\0';

    strncpy(box_name, request + OP_CODE_SIZE + PIPE_SIZE, BOX_SIZE);
    box_name[BOX_SIZE - 1] = '\0';

    snprintf(box_path, BOX_PATH_SIZE, "/%s", box_name);
}

void parse_message(char *message, int *op_code, char *contents,
                   size_t *buffer_size) {
    errno = 0;
    char *endptr;
    *op_code = (int)strtol(message, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != message, "invalid op_code\n");

    strncpy(contents, message + OP_CODE_SIZE, CONTENTS_SIZE - 1);
    contents[MESSAGE_SIZE - 1] = '\0';

    // Removing newline character
    size_t size = strlen(contents);
    contents[size - 1] = '\0';

    *buffer_size = size;
}