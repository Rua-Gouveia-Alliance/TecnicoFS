#ifndef _SERVER_REQUESTS_H
#define _SERVER_REQUESTS_H

#define MESSAGE_SIZE 1024
#define PIPE_SIZE 256
#define BOX_SIZE 32
#define BOX_PATH_SIZE 33
#define REQUEST_SIZE 291
#define OP_CODE_SIZE 2

#include <stdlib.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box,
                    size_t pipe_size, size_t box_size);
void create_list_request();

void create_return();
void create_list_return();

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path);

void parse_message(char *);

#endif