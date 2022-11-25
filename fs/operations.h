#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "config.h"
#include <sys/types.h>

/**
 * TécnicoFS parameters.
 */
typedef struct {
    size_t max_inode_count;
    size_t max_block_count;
    size_t max_open_files_count;

    size_t block_size;
} tfs_params;

/**
 * Return a sane default set of parameters for tecnicofs.
 */
tfs_params tfs_default_params();

/**
 * Initialize tecnicofs, optionally with a given configuration.
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_init(tfs_params const *params);

/**
 * Destroy tecnicofs.
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_destroy();

/**
 * TécnicoFS file opening modes.
 */
typedef enum {
    TFS_O_CREAT = 0b001,
    TFS_O_TRUNC = 0b010,
    TFS_O_APPEND = 0b100,
} tfs_file_mode_t;

/**
 * Open a file.
 *
 * Input:
 *   - name: absolute path name
 *   - mode: can be a combination (with bitwise or) of the following flags:
 *     - append mode (TFS_O_APPEND)
 *     - truncate file contents (TFS_O_TRUNC)
 *     - create file if it does not exist (TFS_O_CREAT)
 *
 * Returns file handle of the opened file if successful, -1 otherwise.
 */
int tfs_open(char const *name, tfs_file_mode_t mode);

/**
 * Create a symbolic link to a file.
 *
 * Input:
 *   - target: absolute path name of the link target
 *   - link_name: absolute path name of the link to be created
 *
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_sym_link(char const *target, char const *link_name);

/**
 * Create a (hard) link to a file.
 *
 * Input:
 *   - target_file: absolute path name of the link target
 *   - link_name: absolute path name of the link to be created
 *
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_link(char const *target_file, char const *link_name);

/**
 * Close a file.
 *
 * Input:
 *   - fhandle: file handle (obtained from a previous call to tfs_open)
 *
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_close(int fhandle);

/**
 * Write to an open file, starting at the current offset.
 *
 * Input:
 *   - fhandle: file handle (obtained from a previous call to tfs_open)
 *   - buffer: buffer containing the contents to write
 *   - len: length of the buffer contents (in bytes)
 *
 * Returns the number of bytes that were written (can be lower than 'len' if the
 * maximum file size is exceeded), or -1 in case of error.
 */
ssize_t tfs_write(int fhandle, void const *buffer, size_t len);

/**
 * Read from an open file, starting at the current offset.
 *
 * Input:
 *   - fhandle: file handle (obtained from a previous call to tfs_open)
 *   - buffer: destination buffer
 *   - len: length of the buffer
 *
 * Returns the number of bytes that were copied from the file to the buffer (can
 * be lower than 'len' if the file size was reached), or -1 in case of error.
 */
ssize_t tfs_read(int fhandle, void *buffer, size_t len);

/**
 * Delete a link, or a file if the number of hard links reaches 0, that
 * exists in TécnicoFS.
 *
 * Input:
 *   - target: path name of the target (in TécnicoFS)
 *
 * Returns 0 if successful, -1 otherwise
 */
int tfs_unlink(char const *target);

/**
 * Copy the contents of a file that exists in the OS' file system tree
 * (outside TécnicoFS) to the TécnicoFS.
 *
 * Input:
 *   - source_path: path name of the source file (from the OS' file system)
 *   - dest_path: absolute path name of the destination file (in TécnicoFS),
 *    which is created if needed, and overwritten if it already exists.
 *
 * Returns 0 if successful, -1 otherwise.
 */
int tfs_copy_from_external_fs(char const *source_path, char const *dest_path);

#endif // OPERATIONS_H
