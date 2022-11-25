#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "AAA!";
char const target_path1[] = "/f1";
char const link_path1[] = "/l1";
char const target_path2[] = "/f2";
char const link_path2[] = "/l2";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

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
    assert(tfs_init(NULL) != -1);

    // Write to symlink and read original file
    {
        int f = tfs_open(target_path1, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);

        assert_empty_file(target_path1); // sanity check
    }

    assert(tfs_sym_link(target_path1, link_path1) != -1);
    assert_empty_file(link_path1);

    write_contents(link_path1);
    assert_contents_ok(target_path1);

    // Write to original file and read through symlink
    {
        int f = tfs_open(target_path2, TFS_O_CREAT);
        assert(f != -1);
        assert(tfs_close(f) != -1);

        write_contents(target_path2);

        assert_contents_ok(target_path2); // sanity check
    }

    assert(tfs_sym_link(target_path2, link_path2) != -1);
    assert_contents_ok(link_path2);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
