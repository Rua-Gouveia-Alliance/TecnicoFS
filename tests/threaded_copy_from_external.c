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
const char *dest_paths[] = {"/f1", "/f2", "/f3"};

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
        int result = equal_files_local(local_fs, tfs);
        assert(result == 0);
        assert(tfs_close(tfs) != -1);
        fclose(local_fs);
    }

    printf("Successful test.\n");

    return 0;
}
