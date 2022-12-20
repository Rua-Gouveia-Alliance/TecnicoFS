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

const char *symlink_paths[] = {"/f1_l", "/f2_l", "/f3_l"};

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

int main(void) {

    assert(tfs_init(NULL) != -1);

    // Creating the files that will be the source for the links
    for (int i = 0; i < FILES; i++)
        assert(tfs_copy_from_external_fs(source_paths[i], dest_paths[i]) != -1);

    // Creating the sym_links
    for (int i = 0; i < FILES; i++)
        assert(tfs_sym_link(dest_paths[i], symlink_paths[i]) != -1);

    // Checking if the link files are equal to their source
    for (int i = 0; i < FILES; i++) {
        int source = tfs_open(dest_paths[i], TFS_O_CREAT);
        assert(source != -1);
        int link = tfs_open(symlink_paths[i], TFS_O_CREAT);
        assert(link != -1);
        int result = equal_files(source, link);
        assert(result == 0);
    }

    // Unlinking the files
    for (int i = 0; i < FILES; i++)
        assert(tfs_unlink(symlink_paths[i]) != -1);

    // Checking if the files have been properly unlinked
    for (int i = 0; i < FILES; i++) {
        int link = tfs_open(symlink_paths[i], TFS_O_APPEND);
        assert(link == -1);
    }

    printf("Successful test.\n");

    return 0;
}
