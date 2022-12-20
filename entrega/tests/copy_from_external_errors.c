#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>

int main() {
    char *path1 = "/f1";

    /* Tests different scenarios where tfs_copy_from_external_fs is expected to
     * fail */

    assert(tfs_init(NULL) != -1);

    int f1 = tfs_open(path1, TFS_O_CREAT);
    assert(f1 != -1);
    assert(tfs_close(f1) != -1);

    // Scenario 1: source file does not exist
    assert(tfs_copy_from_external_fs("./unexistent", path1) == -1);

    // Scenario 2: destination path is not a valid path
    assert(tfs_copy_from_external_fs("tests/file_to_copy.txt", "not_a_valid_path") == -1);

    printf("Successful test.\n");

    return 0;
}
