#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char *file_path = "/f1";
    const char *link_path = "/l1";

    assert(tfs_init(NULL) != -1);

    // Create file
    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);

    const char write_contents[] = "Hello World!";

    // Write to file
    assert(tfs_write(fd, write_contents, sizeof(write_contents)));

    assert(tfs_close(fd) != -1);

    assert(tfs_link(file_path, link_path) != -1);

    // Unlink file
    assert(tfs_unlink(file_path) != -1);

    // Create new file with the same name
    fd = tfs_open(link_path, TFS_O_CREAT);
    assert(fd != -1);

    // Check if file still contains the correct data
    char read_contents[sizeof(write_contents)];
    assert(tfs_read(fd, read_contents, sizeof(read_contents)) != -1);
    assert(strcmp(read_contents, write_contents) == 0);

    printf("Successful test.\n");
}
