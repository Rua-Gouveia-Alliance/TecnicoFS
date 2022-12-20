#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <string.h>

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

int equal_files_local(FILE *local_fs, int tfs) {
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


#endif
