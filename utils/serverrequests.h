#ifndef _SERVER_REQUESTS_H
#define _SERVER_REQUESTS_H

#define CONTENTS_SIZE 1024
#define PIPE_SIZE 256
#define ERROR_SIZE 1024
#define BOX_SIZE 32
#define BOX_PATH_SIZE 33
#define INT_SIZE 3
#define REQUEST_SIZE INT_SIZE + PIPE_SIZE + BOX_SIZE
#define MESSAGE_SIZE INT_SIZE + CONTENTS_SIZE
#define RESPONSE_SIZE 2 * INT_SIZE + ERROR_SIZE
#define LIST_RESPONSE_SIZE 5 * INT_SIZE + BOX_SIZE

#include <stdlib.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box);
void create_list_request(char *dest, char *session_pipe);

void create_response(char *dest, int op_code, int return_code, char *error);
void create_list_response(char *dest, int last, char *box, int box_size,
                          int n_publishers, int n_subscribers);

void create_message(char *dest, int op_code, char *message);

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path);

void parse_response(char *response, int *op_code, int *return_code,
                    char *error);

void parse_list_response(char *response, int *op_code, int *last,
                         char *box_path, size_t *box_size, size_t *n_publishers,
                         size_t *n_subscribers);

void parse_message(char *message, int *op_code, char *contents,
                   size_t *contents_size);

int send_content(char *fifo, char *content, size_t size);

int receive_content(char *fifo, char *content, size_t size);

#endif
