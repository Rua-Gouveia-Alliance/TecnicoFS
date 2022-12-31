#include "serverrequests.h"
#include "betterassert.h"
#include "clients/opcodes.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void create_request(char *dest, uint8_t op_code, char *session_pipe,
                    char *box) {
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, session_pipe, PIPE_PATH_SIZE);
    memcpy(dest + OP_CODE_SIZE + PIPE_PATH_SIZE, box, BOX_NAME_SIZE);
}

void create_list_request(char *dest, char *session_pipe) {
    uint8_t op_code = BOX_LIST;
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, session_pipe, PIPE_PATH_SIZE);
    memset(dest + OP_CODE_SIZE + PIPE_PATH_SIZE, '\0', BOX_NAME_SIZE);
}

void create_response(char *dest, uint8_t op_code, int32_t return_code,
                     char *error) {
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, &return_code, sizeof(int32_t));
    memcpy(dest + OP_CODE_SIZE + sizeof(int32_t), error, ERROR_SIZE);
}

void create_list_response(char *dest, uint8_t last, char *box,
                          uint64_t box_size, uint64_t n_publishers,
                          uint64_t n_subscribers) {
    uint8_t op_code = BOX_LIST_ANS;
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, &last, sizeof(uint8_t));
    memcpy(dest + OP_CODE_SIZE + sizeof(uint8_t), box, BOX_NAME_SIZE);
    memcpy(dest + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE, &box_size,
           sizeof(uint64_t));
    memcpy(dest + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE +
               sizeof(uint64_t),
           &n_publishers, sizeof(uint64_t));
    memcpy(dest + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE +
               2 * sizeof(uint64_t),
           &n_subscribers, sizeof(uint64_t));
}

void create_message(char *dest, uint8_t op_code, char *message) {
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest, message, MESSAGE_SIZE);
}

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path) {}

void parse_response(char *response, int *op_code, int *return_code,
                    char *error) {}

void parse_list_response(char *response, int *op_code, int *last,
                         char *box_path, size_t *box_size, size_t *n_publishers,
                         size_t *n_subscribers) {}

void parse_message(char *message, int *op_code, char *contents,
                   size_t *buffer_size) {}

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
