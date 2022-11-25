#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path1[] = "/f1";
char const target_path2[] = "/f2";
char const target_path3[] = "/f3";
char const target_path4[] = "/f4";
char const link_path1[] = "/l1";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {

    // init TécnicoFS
    tfs_params params = tfs_default_params();
    params.max_inode_count = 4;
    params.max_block_count = 2;
    assert(tfs_init(&params) != -1);

    // create file with content
    {
        int f1 = tfs_open(target_path1, TFS_O_CREAT);
        assert(f1 != -1);
        assert(tfs_close(f1) != -1);
        write_contents(target_path1);
        assert_contents_ok(target_path1); // sanity check
    }

    // create hard link
    assert(tfs_link(target_path1, link_path1) != -1);

    // create file with content
    {
        int f2 = tfs_open(target_path2, TFS_O_CREAT);
        assert(f2 != -1);
        assert(tfs_write(f2, file_contents, sizeof(file_contents)) ==
               -1); // not possible, as the maximum number of data blocks was
                    // reached
        assert(tfs_close(f2) != -1);
    }

    // unlink file
    assert(tfs_unlink(target_path1) != -1);

    // create file with content
    {
        int f3 = tfs_open(target_path3, TFS_O_CREAT);
        assert(f3 != -1);
        assert(tfs_write(f3, file_contents, sizeof(file_contents)) ==
               -1); // not possible, as the maximum number of data blocks was
                    // reached
        assert(tfs_close(f3) != -1);
    }

    // unlink file
    assert(tfs_unlink(link_path1) != -1);

    // create file with content
    {
        int f4 = tfs_open(target_path4, TFS_O_CREAT);
        assert(f4 != -1);
        assert(tfs_close(f4) != -1);
        write_contents(target_path4);
        assert_contents_ok(target_path4); // sanity check
    }

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
