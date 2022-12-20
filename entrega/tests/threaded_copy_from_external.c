#include "operations.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES 3

const char *source_paths[] = {"tests/empty_file.txt", "tests/file_to_copy.txt",
                              "tests/file_to_copy_over512.txt"};
const char *dest_paths[] = {"/f1", "/f2", "/f3"};

#define BLOCK_SIZE 1024

int equal_files(FILE *local_fs, int tfs) {
    char local_fs_buffer[BLOCK_SIZE], tfs_buffer[BLOCK_SIZE];
    size_t local_fs_result;
    ssize_t tfs_result;

    memset(local_fs_buffer, 0, BLOCK_SIZE);
    memset(tfs_buffer, 0, BLOCK_SIZE);
    while (!feof(local_fs) &&
           (tfs_result = tfs_read(tfs, tfs_buffer, BLOCK_SIZE)) != 0) {
        local_fs_result =
            fread(local_fs_buffer, sizeof(char), BLOCK_SIZE, local_fs);

        if (ferror(local_fs) || tfs_result == -1)
            return -1;

        if (local_fs_result != tfs_result)
            return -1;

        if (strncmp(local_fs_buffer, tfs_buffer, BLOCK_SIZE) != 0) {
            return -1;
        }
    }

    tfs_result = tfs_read(tfs, tfs_buffer, BLOCK_SIZE);
    local_fs_result =
        fread(local_fs_buffer, sizeof(char), BLOCK_SIZE, local_fs);
    if (local_fs_result != 0 || tfs_result != 0)
        return -1;

    return 0;
}

void *test(void *id) {
    int i = *((int *)id);
    assert(tfs_copy_from_external_fs(source_paths[i], dest_paths[i]) != -1);
    return NULL;
}

int main(void) {

    assert(tfs_init(NULL) != -1);

    pthread_t threads[FILES];
    int thread_id[FILES];

    for (int i = 0; i < FILES; i++) {
        thread_id[i] = i;
        assert(pthread_create(threads + i, NULL, test,
                              (void *)(thread_id + i)) == 0);
    }

    for (int i = 0; i < FILES; i++)
        assert(pthread_join(threads[i], NULL) == 0);

    for (int i = 0; i < FILES; i++) {
        int tfs = tfs_open(dest_paths[i], TFS_O_CREAT);
        assert(tfs != -1);
        FILE *local_fs = fopen(source_paths[i], "r");
        assert(local_fs != NULL);
        int result = equal_files(local_fs, tfs);
        assert(result == 0);
        assert(tfs_close(tfs) != -1);
        fclose(local_fs);
    }

    printf("Successful test.\n");

    return 0;
}
