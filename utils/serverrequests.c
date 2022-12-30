#include "serverrequests.h"
#include "betterassert.h"
#include "clients/opcodes.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box) {

    char pipe_name[PIPE_SIZE], box_name[BOX_SIZE], op_code_str[INT_SIZE];

    snprintf(op_code_str, INT_SIZE, "%d", op_code);
    op_code_str[INT_SIZE - 1] = '\0';

    strncpy(pipe_name, session_pipe, PIPE_SIZE - 1);
    pipe_name[PIPE_SIZE - 1] = '\0';

    strncpy(box_name, box, BOX_SIZE - 1);
    box_name[BOX_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, INT_SIZE);
    memcpy(dest + INT_SIZE, pipe_name, PIPE_SIZE);
    memcpy(dest + INT_SIZE + PIPE_SIZE, box_name, BOX_SIZE);
}

void create_list_request(char *dest, char *session_pipe) {
    char pipe_name[PIPE_SIZE], box_name[BOX_SIZE], op_code_str[INT_SIZE];

    snprintf(op_code_str, INT_SIZE, "%d", BOX_LIST);
    op_code_str[INT_SIZE - 1] = '\0';

    strncpy(pipe_name, session_pipe, PIPE_SIZE - 1);
    pipe_name[PIPE_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, INT_SIZE);
    memcpy(dest + INT_SIZE, pipe_name, PIPE_SIZE);
    memset(dest + INT_SIZE + PIPE_SIZE, '\0', BOX_SIZE);
}

void create_response(char *dest, int op_code, int return_code, char *error) {
    char error_msg[ERROR_SIZE], return_code_str[INT_SIZE],
        op_code_str[INT_SIZE];

    snprintf(op_code_str, INT_SIZE, "%d", op_code);
    op_code_str[INT_SIZE - 1] = '\0';

    snprintf(return_code_str, INT_SIZE, "%d", return_code);
    op_code_str[INT_SIZE - 1] = '\0';

    strncpy(error_msg, error, ERROR_SIZE - 1);
    error_msg[ERROR_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, INT_SIZE);
    memcpy(dest + INT_SIZE, return_code_str, INT_SIZE);
    memcpy(dest + 2 * INT_SIZE, error_msg, ERROR_SIZE);
}

void create_list_response(char *dest, int last, char *box, int box_size,
                          int n_publishers, int n_subscribers) {
    char last_str[INT_SIZE], box_name[BOX_SIZE], box_size_str[INT_SIZE],
        n_publishers_str[INT_SIZE], n_subscribers_str[INT_SIZE],
        op_code_str[INT_SIZE];

    snprintf(op_code_str, INT_SIZE, "%d", BOX_LIST_ANS);
    op_code_str[INT_SIZE - 1] = '\0';

    snprintf(last_str, INT_SIZE, "%d", last);
    last_str[INT_SIZE - 1] = '\0';

    strncpy(box_name, box, BOX_SIZE - 1);
    box_name[BOX_SIZE - 1] = '\0';

    snprintf(box_size_str, INT_SIZE, "%d", box_size);
    box_size_str[INT_SIZE - 1] = '\0';

    snprintf(n_publishers_str, INT_SIZE, "%d", n_publishers);
    n_publishers_str[INT_SIZE - 1] = '\0';

    snprintf(n_subscribers_str, INT_SIZE, "%d", n_subscribers);
    n_subscribers_str[INT_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, INT_SIZE);
    memcpy(dest + INT_SIZE, last_str, INT_SIZE);
    memcpy(dest + 2 * INT_SIZE, box_name, BOX_SIZE);
    memcpy(dest + 2 * INT_SIZE + BOX_SIZE, box_size_str, INT_SIZE);
    memcpy(dest + 3 * INT_SIZE + BOX_SIZE, n_publishers_str, INT_SIZE);
    memcpy(dest + 4 * INT_SIZE + BOX_SIZE, n_subscribers_str, INT_SIZE);
}

void create_message(char *dest, int op_code, char *message) {
    char contents[CONTENTS_SIZE], op_code_str[INT_SIZE];

    snprintf(op_code_str, INT_SIZE, "%d", op_code);
    op_code_str[INT_SIZE - 1] = '\0';

    strncpy(contents, message, CONTENTS_SIZE - 1);
    contents[CONTENTS_SIZE - 1] = '\0';

    memcpy(dest, op_code_str, INT_SIZE);
    memcpy(dest + INT_SIZE, contents, CONTENTS_SIZE);
}

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path) {
    errno = 0;
    char *endptr, box_name[BOX_SIZE];
    *op_code = (int)strtol(request, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != request, "invalid op_code\n");

    strncpy(session_pipe, request + INT_SIZE, PIPE_SIZE - 1);
    session_pipe[PIPE_SIZE - 1] = '\0';

    strncpy(box_name, request + INT_SIZE + PIPE_SIZE, BOX_SIZE - 1);
    box_name[BOX_SIZE - 1] = '\0';

    snprintf(box_path, BOX_PATH_SIZE, "/%s", box_name);
}

void parse_response(char *response, int *op_code, int *return_code,
                    char *error) {
    errno = 0;
    char *endptr;

    *op_code = (int)strtol(response, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response, "invalid op_code\n");

    *return_code = (int)strtol(response + INT_SIZE, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response + INT_SIZE,
                  "invalid op_code\n");

    strncpy(error, response + 2 * INT_SIZE, ERROR_SIZE - 1);
    error[ERROR_SIZE - 1] = '\0';
}

void parse_list_response(char *response, int *op_code, int *last,
                         char *box_path, size_t *box_size, size_t *n_publishers,
                         size_t *n_subscribers) {
    errno = 0;
    char *endptr, box_name[BOX_SIZE];

    *op_code = (int)strtol(response, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response, "invalid op_code\n");

    *last = (int)strtol(response + INT_SIZE, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response + INT_SIZE,
                  "invalid op_code\n");

    strncpy(box_name, response + 2 * INT_SIZE, BOX_SIZE - 1);
    box_name[BOX_SIZE - 1] = '\0';
    snprintf(box_path, BOX_PATH_SIZE, "/%s", box_name);

    *box_size = (int)strtol(response + 2 * INT_SIZE + BOX_SIZE, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response + 2 * INT_SIZE + BOX_SIZE,
                  "invalid op_code\n");

    *n_publishers =
        (int)strtol(response + 3 * INT_SIZE + BOX_SIZE, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response + 3 * INT_SIZE + BOX_SIZE,
                  "invalid op_code\n");

    *n_subscribers =
        (int)strtol(response + 4 * INT_SIZE + BOX_SIZE, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != response + 4 * INT_SIZE + BOX_SIZE,
                  "invalid op_code\n");
}

void parse_message(char *message, int *op_code, char *contents,
                   size_t *buffer_size) {
    errno = 0;
    char *endptr;
    *op_code = (int)strtol(message, &endptr, 10);
    ALWAYS_ASSERT(errno == 0 && endptr != message, "invalid op_code\n");

    strncpy(contents, message + INT_SIZE, CONTENTS_SIZE - 1);
    contents[CONTENTS_SIZE - 1] = '\0';

    // Removing newline character
    size_t size = strlen(contents);
    *buffer_size = size;
}

int send_content(char *fifo, char *content, size_t size) {
    int fd = open(fifo, O_WRONLY);
    if (fd == -1)
        return -1;

    write(fd, content, size);
    close(fd);
    return 0;
}

int receive_content(char *fifo, char *content, size_t size) {
    int fd = open(fifo, O_RDONLY);
    if (fd == -1)
        return -1;

    read(fd, content, size);
    close(fd);
    return 0;
}
