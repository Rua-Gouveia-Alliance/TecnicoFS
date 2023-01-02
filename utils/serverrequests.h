#ifndef _SERVER_REQUESTS_H
#define _SERVER_REQUESTS_H

#define MESSAGE_CONTENT_SIZE 1024
#define PIPE_PATH_SIZE 256
#define ERROR_SIZE 1024
#define BOX_PATH_SIZE 33
#define BOX_NAME_SIZE 32
#define OP_CODE_SIZE sizeof(uint8_t)
#define REQUEST_SIZE OP_CODE_SIZE + PIPE_PATH_SIZE + BOX_NAME_SIZE
#define MESSAGE_SIZE OP_CODE_SIZE + MESSAGE_CONTENT_SIZE
#define RESPONSE_SIZE OP_CODE_SIZE + sizeof(uint32_t) + ERROR_SIZE
#define LIST_RESPONSE_SIZE                                                     \
    OP_CODE_SIZE + sizeof(uint8_t) + BOX_NAME_SIZE + 3 * sizeof(uint64_t)

#include <stdint.h>
#include <stdlib.h>

void create_request(char *dest, uint8_t op_code, char *session_pipe, char *box);
void create_list_request(char *dest, char *session_pipe);

void create_response(char *dest, uint8_t op_code, int32_t return_code,
                     char *error);
void create_list_response(char *dest, uint8_t last, char *box,
                          uint64_t box_size, uint64_t n_publishers,
                          uint64_t n_subscribers);

void create_message(char *dest, uint8_t op_code, char *message);

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path);

void parse_response(char *response, int *op_code, int *return_code,
                    char *error);

void parse_list_response(char *response, int *op_code, int *last,
                         char *box_name, size_t *box_size, size_t *n_publishers,
                         size_t *n_subscribers);

void parse_message(char *message, int *op_code, char *contents,
                   size_t *contents_size);

int send_content(char *fifo, char *content, size_t size);

int receive_content(char *fifo, char *content, size_t size);

#endif
