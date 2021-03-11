#include <string.h>
# include "fs.h"


const size_t SUPER_BLOCK_OFFSET = 0;
const size_t INODES_OFFSET = SUPER_BLOCK_OFFSET + sizeof(struct super_block);
const size_t BLOCKS_OFFSET = INODES_OFFSET+ INODE_NUM * sizeof(struct inode);
const size_t FS_SIZE = BLOCKS_OFFSET + BLOCK_NUM * sizeof(struct block);

void init_inode(struct inode* inode_ptr, size_t next_free_inode_index) {
    inode_ptr->type_ = 0;
    inode_ptr->next_free_inode_index_ = next_free_inode_index;
    inode_ptr->data_blocks_num_ = 0;
}

void init_block(struct block* block_ptr, size_t next_free_block_index) {
    block_ptr->next_free_block_index_ = next_free_block_index;
}

void init_super_block(struct super_block* sb_ptr) {
    sb_ptr->free_block_ = 0;
    sb_ptr->free_inode_ = 0;
}

void init_fs(int fs_fd) {
    for (size_t inode_index = 0; inode_index < INODE_NUM; ++inode_index) {
        struct inode inode;
        read_inode(fs_fd, &inode, inode_index);
        init_inode(&inode, inode_index + 1);
        write_inode(fs_fd, &inode, inode_index);
    }
    for (size_t block_index = 0; block_index < BLOCK_NUM; ++block_index) {
        struct block block;
        read_block(fs_fd, &block, block_index);
        init_block(&block, block_index + 1);
        write_block(fs_fd, &block, block_index);
    }
    struct super_block sb;
    read_super_block(fs_fd, &sb);
    init_super_block(&sb);
    write_super_block(fs_fd, &sb);

    make_directory(fs_fd, "root", 0);
}

void read_super_block(int fs_fd, struct super_block* sb_ptr) {
    lseek(fs_fd, 0, SEEK_SET);
    read(fs_fd, sb_ptr, sizeof(struct super_block));
}

void read_inode(int fs_fd, struct inode* inode_ptr, size_t index) {
    size_t offset = INODES_OFFSET + index * sizeof(struct inode);
    lseek(fs_fd, offset, SEEK_SET);
    read(fs_fd, inode_ptr, sizeof(struct inode));
}

void read_block(int fs_fd, struct block* block_ptr, size_t index) {
    size_t offset = BLOCKS_OFFSET + index * sizeof(struct block);
    lseek(fs_fd, offset, SEEK_SET);
    read(fs_fd, block_ptr, sizeof(struct block));
}

void write_super_block(int fs_fd, struct super_block* sb_ptr) {
    size_t offset = SUPER_BLOCK_OFFSET;
    lseek(fs_fd, offset, SEEK_SET);
    write(fs_fd, sb_ptr, sizeof(struct super_block));
}

void write_inode(int fs_fd, struct inode* inode_ptr, size_t index) {
    size_t offset = INODES_OFFSET + index * sizeof(struct inode);
    lseek(fs_fd, offset, SEEK_SET);
    write(fs_fd, inode_ptr, sizeof(struct inode));
}

void write_block(int fs_fd, struct block* block_ptr, size_t index) {
    size_t offset = BLOCKS_OFFSET + index * sizeof(struct block);
    lseek(fs_fd, offset, SEEK_SET);
    write(fs_fd, block_ptr, sizeof(struct block));
}

int open_fs(char* fs_name) {
    printf("Opening file system at %s\n", fs_name);
    int flags = O_RDWR;
    int fs_fd = 0;
    if( access( fs_name, F_OK ) == 0 ) { // file exists
        fs_fd = open(fs_name, flags);
    } else { // file doesn't exist
        printf("File system not exists. Creating new file system.\n");
        flags |= O_CREAT;
        fs_fd = open(fs_name, flags, 0666);
        lseek(fs_fd, FS_SIZE , SEEK_SET);
        dprintf(fs_fd, "0");
        lseek(fs_fd, 0 , SEEK_SET);
        init_fs(fs_fd);
    }
    printf("File system opened.\n");
    return fs_fd;
}

size_t make_file(int fs_fd, char* name) {
    struct super_block sb;
    read_super_block(fs_fd, &sb);

    size_t inode_index = sb.free_inode_;
    struct inode inode;

    read_inode(fs_fd, &inode, inode_index);

    inode.type_ = 0;
    sb.free_inode_ = inode.next_free_inode_index_;
    strcpy(inode.name_, name);

    write_inode(fs_fd, &inode, inode_index);

    write_super_block(fs_fd, &sb);

    return inode_index;
}

size_t make_directory(int fs_fd, char* name, size_t parent_inode_index) {
    size_t inode_index = make_file(fs_fd, name);
    struct inode inode;

    read_inode(fs_fd, &inode, inode_index);

    inode.type_ = 1;

    size_t block_index = capture_block(fs_fd);
    inode.data_blocks_num_ = 1;
    inode.data_blocks_[0] = block_index;

    struct block block;
    read_block(fs_fd, &block, block_index);

    struct directory directory;
    directory.file_num_ = 0;

    // create "."
    size_t self_link_inode_index = make_file(fs_fd, ".");
    struct inode self_inode;
    read_inode(fs_fd, &self_inode, self_link_inode_index);

    self_inode.type_ = 2;
    self_inode.origin_ = inode_index;

    write_inode(fs_fd, &self_inode, self_link_inode_index);

    directory.file_num_++;
    directory.files_[0] = self_link_inode_index;

    // create ".."
    size_t parent_link_inode_index = make_file(fs_fd, "..");
    struct inode parent_link_inode;
    read_inode(fs_fd, &parent_link_inode, parent_link_inode_index);

    parent_link_inode.type_ = 2;
    parent_link_inode.origin_ = parent_inode_index;

    write_inode(fs_fd, &parent_link_inode, parent_link_inode_index);

    directory.file_num_++;
    directory.files_[1] = parent_link_inode_index;

    memcpy(block.data_, (char*) &directory, sizeof(struct directory));

    write_block(fs_fd, &block, block_index);

    write_inode(fs_fd, &inode, inode_index);

    return inode_index;
}

size_t capture_block(int fs_fd) {
    struct super_block sb;
    read_super_block(fs_fd, &sb);

    size_t block_index = sb.free_block_;
    struct block block;
    read_block(fs_fd, &block, block_index);
    sb.free_block_ = block.next_free_block_index_;
    write_block(fs_fd, &block, block_index);

    write_super_block(fs_fd, &sb);

    return block_index;
}
