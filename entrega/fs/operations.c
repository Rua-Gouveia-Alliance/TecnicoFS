#include "operations.h"
#include "config.h"
#include "state.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "betterassert.h"
#include "betterlocks.h"

static pthread_mutex_t file_exists_mutex;

static inline bool valid_file_handle(int file_handle) {
    return file_handle >= 0 && file_handle < max_open_files();
}

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    MUTEX_INIT(&file_exists_mutex);

    return 0;
}

int tfs_destroy() {
    MUTEX_DESTROY(&file_exists_mutex);

    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inumber: the root directory inode inumber
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, int root_inumber) {
    if (ROOT_DIR_INUM != root_inumber)
        return -1;

    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inumber, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    // If we have multiple tfs_open running we might be creating a file
    // at the same time we are checking for it's existence.
    MUTEX_LOCK(&file_exists_mutex);

    int inum = tfs_lookup(name, ROOT_DIR_INUM);
    size_t offset;

    if (inum >= 0) { // The file already exists
        MUTEX_UNLOCK(&file_exists_mutex);

        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        if (inode->i_node_type == T_SYMLINK) {
            char *buffer = malloc(MAX_FILE_NAME + 1);
            char *block = data_block_get(inode->i_data_block);
            strncpy(buffer, block, MAX_FILE_NAME);

            int new_handle = tfs_open(buffer, mode);
            if (new_handle == -1) {
                free(buffer);
                return -1;
            }

            free(buffer);
            return new_handle;
        }

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            RWLOCK_WRLOCK(inode_rwlocks + inum);
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
            RWLOCK_UNLOCK(inode_rwlocks + inum);
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            MUTEX_UNLOCK(&file_exists_mutex);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(ROOT_DIR_INUM, name + 1, inum) == -1) {
            inode_delete(inum);
            MUTEX_UNLOCK(&file_exists_mutex);
            return -1; // no space in directory
        }

        offset = 0;
        MUTEX_UNLOCK(&file_exists_mutex);
    } else {
        MUTEX_UNLOCK(&file_exists_mutex);
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    ALWAYS_ASSERT(
        MAX_FILE_NAME <= state_block_size(),
        "tfs_sym_link: maximum filename size is bigger than block size")

    // Checks if the path name is valid
    if (!valid_pathname(target) || !valid_pathname(link_name)) {
        return -1;
    }

    // check if target exists and link does not
    if (tfs_lookup(target, ROOT_DIR_INUM) == -1 ||
        tfs_lookup(link_name, ROOT_DIR_INUM) != -1)
        return -1;

    // create symlink inode
    int link_inum = inode_create(T_SYMLINK);
    if (link_inum == -1) {
        return -1; // no space in inode table
    }
    // add symlink in the root directory
    if (add_dir_entry(ROOT_DIR_INUM, link_name + 1, link_inum) == -1) {
        inode_delete(link_inum);
        return -1; // no space in directory
    }

    inode_t *link_inode = inode_get(link_inum);
    RWLOCK_WRLOCK(inode_rwlocks + link_inum);
    // allocate new block
    int bnum = data_block_alloc();
    if (bnum == -1) {
        RWLOCK_UNLOCK(inode_rwlocks + link_inum);
        return -1; // no space
    }
    link_inode->i_data_block = bnum;

    // copy the path to block
    char *block = data_block_get(bnum);
    strncpy(block, target, MAX_FILE_NAME);

    RWLOCK_UNLOCK(inode_rwlocks + link_inum);
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    int err;
    if (!valid_pathname(target) || !valid_pathname(link_name))
        return -1;

    // get inum
    int inum = tfs_lookup(target, ROOT_DIR_INUM);
    if (inum == -1)
        return -1;

    // get inode
    inode_t *inode = inode_get(inum);
    ALWAYS_ASSERT(inode != NULL,
                  "tfs_link: directory files must have an inode");
    RWLOCK_WRLOCK(inode_rwlocks + inum);
    if (inode->i_node_type == T_SYMLINK) {
        RWLOCK_UNLOCK(inode_rwlocks + inum);
        return -1;
    }

    // add dir entry
    err = add_dir_entry(ROOT_DIR_INUM, link_name + 1, inum);
    if (err == -1) {
        RWLOCK_UNLOCK(inode_rwlocks + inum);
        return -1;
    }

    // update hard link count
    inode->i_hardl++;

    RWLOCK_UNLOCK(inode_rwlocks + inum);
    return 0;
}

int tfs_close(int fhandle) {
    if (!valid_file_handle(fhandle))
        return -1;

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // Locking this file for writing
    RWLOCK_WRLOCK(open_file_rwlocks + fhandle);

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        // Locking the corresponding inode for writing
        RWLOCK_WRLOCK(inode_rwlocks + file->of_inumber);

        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                RWLOCK_UNLOCK(inode_rwlocks + file->of_inumber);
                RWLOCK_UNLOCK(open_file_rwlocks + fhandle);
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        RWLOCK_WRLOCK(data_rwlocks + inode->i_data_block);
        RWLOCK_UNLOCK(inode_rwlocks + file->of_inumber);

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);
        RWLOCK_UNLOCK(data_rwlocks + inode->i_data_block);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }

    RWLOCK_UNLOCK(open_file_rwlocks + fhandle);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // Locking this file for reading
    RWLOCK_RDLOCK(open_file_rwlocks + fhandle);
    // Locking the corresponding inode for reading
    RWLOCK_RDLOCK(inode_rwlocks + file->of_inumber);

    // From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");
        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
    }
    file->of_offset += to_read;
    RWLOCK_UNLOCK(inode_rwlocks + file->of_inumber);
    RWLOCK_UNLOCK(open_file_rwlocks + fhandle);

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    int err;

    // Checks if the path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    int inum = tfs_lookup(target, ROOT_DIR_INUM);
    if (inum == -1)
        return -1;

    // clear dir entry
    err = clear_dir_entry(ROOT_DIR_INUM, target + 1);
    if (err == -1)
        return -1;

    // get inode
    inode_t *inode = inode_get(inum);
    ALWAYS_ASSERT(inode != NULL,
                  "tfs_unlink: directory files must have an inode");

    RWLOCK_WRLOCK(inode_rwlocks + inum);
    // decrease hard link count and if 0 delete inode
    inode->i_hardl--;
    RWLOCK_UNLOCK(inode_rwlocks + inum);
    if (inode->i_hardl == 0) {
        inode_delete(inum);
        return 0;
    }

    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    FILE *source_handle = fopen(source_path, "r");
    if (source_handle == NULL)
        return -1;

    int dest_handle = tfs_open(dest_path, TFS_O_TRUNC | TFS_O_CREAT);
    if (dest_handle == -1) {
        fclose(source_handle);
        return -1;
    }

    size_t read_result;
    size_t block_size = state_block_size();
    char buffer[block_size];

    while (!feof(source_handle)) {
        read_result = fread(buffer, sizeof(char), block_size, source_handle);
        if (ferror(source_handle) ||
            tfs_write(dest_handle, buffer, read_result * sizeof(char)) !=
                read_result * sizeof(char)) {
            fclose(source_handle);
            tfs_close(dest_handle);
            return -1;
        }
    }

    fclose(source_handle);
    tfs_close(dest_handle);
    return 0;
}
