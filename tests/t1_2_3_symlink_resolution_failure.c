#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char *file_path = "/f1";
    const char *link_path = "/l1";

    tfs_params params = tfs_default_params();
    params.max_inode_count = 3;
    params.max_block_count = 3;
    assert(tfs_init(&params) != -1);

    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);

    // Immediately close file
    assert(tfs_close(fd) != -1);

    // Create symbolic link
    assert(tfs_sym_link(file_path, link_path) == 0);

    // Remove original file
    assert(tfs_unlink(file_path) != -1);

    // Link unusable - target no longer exists
    assert(tfs_open(link_path, TFS_O_APPEND) == -1);

    // Create new file with same filename
    fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_close(fd) != -1);

    // Link usable again - target was recreated
    assert(tfs_open(link_path, TFS_O_APPEND) != -1);

    printf("Successful test.\n");
}
