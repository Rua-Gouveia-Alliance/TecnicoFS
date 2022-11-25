#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path1[] = "/f1";
char const target_path2[] = "/f2";
char const link_path1[] = "/l1";
char const link_path2[] = "/l2";

int main() {
    // init TécnicoFS
    tfs_params params = tfs_default_params();
    params.max_block_count = 2;
    assert(tfs_init(&params) != -1);

    // create file with content
    {
        int f1 = tfs_open(target_path1, TFS_O_CREAT);
        assert(f1 != -1);
        assert(tfs_write(f1, file_contents, sizeof(file_contents)) ==
               sizeof(file_contents));
        assert(tfs_close(f1) != -1);
    }

    // create hard link
    assert(tfs_link(target_path1, link_path1) != -1);
    // create soft link
    assert(tfs_sym_link(target_path1, link_path2) != -1);

    // create file with content
    {
        int f2 = tfs_open(target_path2, TFS_O_CREAT);
        assert(f2 != -1);
        assert(tfs_write(f2, file_contents, sizeof(file_contents)) ==
               -1); // not possible, as the maximum number of data blocks was
                    // reached
        assert(tfs_close(f2) != -1);
    }

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
