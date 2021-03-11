#ifndef MINIFS_FS_H
#define MINIFS_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


#define INODE_NUM 1024
#define BLOCK_NUM 1024
#define BLOCK_PER_NODE 16
#define BLOCK_SIZE 1024
#define MAX_FILES_IN_DIR (BLOCK_SIZE - 32) / sizeof(size_t)


struct inode {
    char name_[256];
    size_t data_blocks_[BLOCK_PER_NODE];
    char type_; // 0 -- common file; 1 -- directory; 2 -- weak link
    size_t origin_; // valid only for weak links
    size_t data_blocks_num_;
    size_t next_free_inode_index_; // valid only for free block
};

struct block {
    char data_[BLOCK_SIZE];
    size_t next_free_block_index_; // valid only for free blocks
};

struct super_block{
    size_t free_inode_;
    size_t free_block_;
};

struct directory {
    size_t files_[MAX_FILES_IN_DIR];
    size_t file_num_;
};

extern const size_t SUPER_BLOCK_OFFSET;
extern const size_t INODES_OFFSET;
extern const size_t BLOCKS_OFFSET;
extern const size_t FS_SIZE;

void init_fs(int fs_fd);

void read_super_block(int fs_fd, struct super_block* sb_ptr);
void read_inode(int fs_fd, struct inode* inode_ptr, size_t index);
void read_block(int fs_fd, struct block* block_ptr, size_t index);

void write_super_block(int fs_fd, struct super_block* sb_ptr);
void write_inode(int fs_fd, struct inode* inode_ptr, size_t index);
void write_block(int fs_fd, struct block* block_ptr, size_t index);

int open_fs(char* fs_name);

size_t make_file(int fs_fd, char* name);
size_t make_directory(int fs_fd, char* name, size_t parent_inode_index);
size_t capture_block(int fs_fd);


#endif
