#include "operations.h"
#include "util/util.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILES 3

const char *source_paths[] = {"tests/empty_file.txt", "tests/file_to_copy.txt",
                              "tests/file_to_copy_over512.txt"};
const char *dest_path = "/f1";

int main(void) {

    assert(tfs_init(NULL) != -1);

    // Copying to the same destination
    for (int i = 0; i < FILES; i++)
        assert(tfs_copy_from_external_fs(source_paths[i], dest_path) != -1);

    // Checking if the destination is properly written, should be equal to the
    // last file coppied
    int tfs = tfs_open(dest_path, TFS_O_CREAT);
    assert(tfs != -1);
    FILE *local_fs = fopen(source_paths[FILES - 1], "r");
    assert(local_fs != NULL);
    int result = equal_files_local(local_fs, tfs);
    assert(result == 0);
    assert(tfs_close(tfs) != -1);
    fclose(local_fs);

    printf("Successful test.\n");

    return 0;
}
