#include "operations.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINK_COUNT 10

const char *source_path = "tests/file_to_copy.txt";
const char *dest_path = "/f1";

#define BLOCK_SIZE 1024

int equal_files(int tfs_1, int tfs_2) {
    char tfs_1_buffer[BLOCK_SIZE], tfs_2_buffer[BLOCK_SIZE];
    ssize_t tfs_1_result ,tfs_2_result;

    memset(tfs_1_buffer, 0, BLOCK_SIZE);
    memset(tfs_2_buffer, 0, BLOCK_SIZE);
    while ((tfs_1_result = tfs_read(tfs_1, tfs_1_buffer, BLOCK_SIZE)) != 0 && (tfs_2_result = tfs_read(tfs_2, tfs_2_buffer, BLOCK_SIZE)) != 0) {

        if (tfs_1_result == -1 || tfs_2_result == -1)
            return -1;

        if (tfs_1_result != tfs_2_result )
            return -1;

        if (strncmp(tfs_1_buffer, tfs_2_buffer, BLOCK_SIZE) != 0)
            return -1;
    }

    tfs_1_result = tfs_read(tfs_1, tfs_1_buffer, BLOCK_SIZE);
    tfs_2_result = tfs_read(tfs_2, tfs_2_buffer, BLOCK_SIZE);
    if (tfs_1_result != 0 || tfs_2_result != 0)
        return -1;

    return 0;
}

void *link_file(void *id) {
    int i = *((int *)id);
    char link_path[4];
    sprintf(link_path, "/l%d", i);
    assert(tfs_link(dest_path, link_path) != -1);
    return NULL;
}

void *unlink_file(void *id) {
    int i = *((int *)id);
    char link_path[4];
    sprintf(link_path, "/l%d", i);
    assert(tfs_unlink(link_path) != -1);
    return NULL;
}

int main(void) {

    assert(tfs_init(NULL) != -1);

    assert(tfs_copy_from_external_fs(source_path, dest_path) != -1);

    pthread_t threads[LINK_COUNT];
    int thread_id[LINK_COUNT];

    // Creating several links to the same file
    for (int i = 0; i < LINK_COUNT; i++) {
        thread_id[i] = i;
        assert(pthread_create(threads + i, NULL, link_file, (void *)(thread_id + i)) == 0);
    }

    for (int i = 0; i < LINK_COUNT; i++)
        assert(pthread_join(threads[i], NULL) == 0);

    // Checking if the link files are equal to their source
    for (int i = 0; i < LINK_COUNT; i++) {
        int source = tfs_open(dest_path, TFS_O_CREAT);
        assert(source != -1);
        char link_path[4];
        sprintf(link_path, "/l%d", i);
        int link = tfs_open(link_path, TFS_O_CREAT);
        assert(link != -1);
        int result = equal_files(source, link);
        assert(result == 0);
        assert(tfs_close(source) != -1);
    }

    // Unlinking the files
    for (int i = 0; i < LINK_COUNT; i++) {
        thread_id[i] = i;
        assert(pthread_create(threads + i, NULL, unlink_file, (void *)(thread_id + i)) == 0);
    }

    for (int i = 0; i < LINK_COUNT; i++)
        assert(pthread_join(threads[i], NULL) == 0);

    // Checking if the files have been properly unlinked
    for (int i = 0; i < LINK_COUNT; i++) {
        char link_path[4];
        sprintf(link_path, "/l%d", i);
        int link = tfs_open(link_path, TFS_O_APPEND);
        assert(link == -1);
    }

    printf("Successful test.\n");

    return 0;
}
