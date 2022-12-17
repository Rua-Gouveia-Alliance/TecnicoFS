#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

/**
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY, T_SYMLINK } inode_type;

/**
 * Inode
 */
typedef struct {
    inode_type i_node_type;
    size_t i_size;
    int i_data_block;
    int i_hardl;
    pthread_rwlock_t i_rwlock;
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

/**
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
} open_file_entry_t;

int state_init(tfs_params);
int state_destroy(void);

size_t state_block_size(void);

int inode_create(inode_type n_type); /* inode_mutex */
void inode_delete(int inumber); /* inode_mutex */
inode_t *inode_get(int inumber);

int clear_dir_entry(inode_t *inode, char const *sub_name); /* i_rwlock */
int add_dir_entry(inode_t *inode, char const *sub_name, int sub_inumber); /* i_rwlock */
int find_in_dir(inode_t *inode, char const *sub_name); /* i_rwlock */

int data_block_alloc(void); /* data_mutex */
void data_block_free(int block_number); /* data_mutex */
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset); /* of_mutex */
void remove_from_open_file_table(int fhandle); /* of_mutex */
open_file_entry_t *get_open_file_entry(int fhandle); /* of_mutex */

#endif // STATE_H
