#ifndef _SERVER_REQUESTS_H
#define _SERVER_REQUESTS_H

#define CONTENTS_SIZE 1024
#define PIPE_SIZE 256
#define BOX_SIZE 32
#define BOX_PATH_SIZE 33
#define OP_CODE_SIZE 2
#define REQUEST_SIZE OP_CODE_SIZE + PIPE_SIZE + BOX_SIZE
#define MESSAGE_SIZE OP_CODE_SIZE + CONTENTS_SIZE

#include <stdlib.h>

void create_request(char *dest, int op_code, char *session_pipe, char *box);
void create_list_request();

void create_return();
void create_list_return();

void create_message(char *dest, int op_code, char *message);

void parse_request(char *request, int *op_code, char *session_pipe,
                   char *box_path);

void parse_message(char *message, int *op_code, char *contents,
                   size_t *contents_size);

#endif