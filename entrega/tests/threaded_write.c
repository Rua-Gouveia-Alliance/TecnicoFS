#include "operations.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 1024
#define AMOUNT 100

const char *dest_path = "/f1";
char final_msg[BLOCK_SIZE];
char buffer[BLOCK_SIZE];
const char *to_write = "A";

void *test(void *handle) {
    assert(tfs_write(*((int *)handle), to_write, 1) != -1);
    return NULL;
}

int main(void) {
    assert(AMOUNT <= BLOCK_SIZE);

    assert(tfs_init(NULL) != -1);
    int handle = tfs_open(dest_path, TFS_O_CREAT | TFS_O_APPEND);
    assert(handle != -1);

    // threads
    pthread_t threads[AMOUNT];
    for (int i = 0; i < AMOUNT; i++) {
        assert(pthread_create(&threads[i], NULL, test, &handle) == 0);
    }
    for (int i = 0; i < AMOUNT; i++) {
        assert(pthread_join(threads[i], NULL) == 0);
    }
    tfs_close(handle);

    // setup buffer
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (i < AMOUNT)
            final_msg[i] = 'A';
        else
            final_msg[i] = '\0';
    }

    // open and read result
    handle = tfs_open(dest_path, TFS_O_CREAT);
    assert(handle != -1);
    assert(tfs_read(handle, buffer, BLOCK_SIZE) != -1);

    // compare
    assert(strncmp(buffer, final_msg, BLOCK_SIZE) == 0);

    // close and exit
    assert(tfs_close(handle) != -1);
    printf("Successful test.\n");
    return 0;
}
