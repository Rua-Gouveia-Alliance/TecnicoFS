#include "serverrequests.h"
#include "../clients/opcodes.h"
#include "betterassert.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void create_request(void *dest, uint8_t op_code, char *session_pipe,
                    char *box) {

    // clear bytes
    memset(dest, '\0', REQUEST_SIZE);

    // write data
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, session_pipe, PIPE_PATH_SIZE);
    memcpy(dest + OP_CODE_SIZE + PIPE_PATH_SIZE, box, BOX_NAME_SIZE);
}

void create_response(void *dest, uint8_t op_code, int32_t return_code,
                     char *error) {

    // clear bytes
    memset(dest, '\0', RESPONSE_SIZE);

    // write data
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, &return_code, RET_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE + RET_CODE_SIZE, error, ERROR_SIZE);
}

void create_list_response(void *dest, uint8_t last, char *box,
                          uint64_t box_size, uint64_t n_publishers,
                          uint64_t n_subscribers) {

    uint8_t op_code = BOX_LIST_ANS;

    // clear bytes
    memset(dest, '\0', LIST_RESPONSE_SIZE);

    // write data
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

void create_message(void *dest, uint8_t op_code, char *message) {
    // clear bytes
    memset(dest, '\0', MESSAGE_SIZE);

    // write data
    memcpy(dest, &op_code, OP_CODE_SIZE);
    memcpy(dest + OP_CODE_SIZE, message, MESSAGE_SIZE);
}

void parse_request(void *request, uint8_t *op_code, char *session_pipe,
                   char *box_name) {

    // clear bytes
    memset(session_pipe, '\0', PIPE_PATH_SIZE);
    memset(box_name, '\0', BOX_NAME_SIZE);

    // write data
    memcpy(op_code, request, OP_CODE_SIZE);
    memcpy(session_pipe, request + OP_CODE_SIZE, PIPE_PATH_SIZE);
    memcpy(box_name, request + OP_CODE_SIZE + PIPE_PATH_SIZE, BOX_NAME_SIZE);
}

void parse_response(void *response, uint8_t *op_code, int32_t *return_code,
                    char *error) {

    // clear bytes
    memset(error, '\0', ERROR_SIZE);

    // write data
    memcpy(op_code, response, OP_CODE_SIZE);
    memcpy(return_code, response + OP_CODE_SIZE, RET_CODE_SIZE);
    memcpy(error, response + OP_CODE_SIZE + RET_CODE_SIZE, ERROR_SIZE);
}

void parse_list_response(void *response, uint8_t *op_code, uint8_t *last,
                         char *box_name, uint64_t *box_size,
                         uint64_t *n_publishers, uint64_t *n_subscribers) {

    // clear bytes
    memset(box_name, '\0', BOX_NAME_SIZE);

    // write data
    memcpy(op_code, response, OP_CODE_SIZE);
    memcpy(last, response + OP_CODE_SIZE, sizeof(uint8_t));
    memcpy(box_name, response + OP_CODE_SIZE + sizeof(uint8_t), BOX_NAME_SIZE);
    memcpy(box_size, response + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE,
           sizeof(uint64_t));
    memcpy(n_publishers,
           response + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE +
               sizeof(uint64_t),
           sizeof(uint64_t));
    memcpy(n_subscribers,
           response + OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE +
               2 * sizeof(uint64_t),
           sizeof(uint64_t));
}

void parse_message(void *message, uint8_t *op_code, char *contents) {

    memcpy(op_code, message, OP_CODE_SIZE);
    memcpy(contents, message + OP_CODE_SIZE, MESSAGE_CONTENT_SIZE);
}

int send_content(char *fifo, void *content, size_t size) {
    int fd = open(fifo, O_WRONLY);
    if (fd == -1)
        return -1;

    if (write(fd, content, size) == -1) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int receive_content(char *fifo, void *content, size_t size) {
    int fd = open(fifo, O_RDONLY);
    if (fd == -1)
        return -1;

    if (read(fd, content, size) == -1) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
