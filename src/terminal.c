#include "terminal.h"


#define CMD_MAX_LEN 256


int parse_command(char* cmd) {
    int split = 0;
    while(split < CMD_MAX_LEN && cmd[split] != ' ' && cmd[split] != '\0') {
        ++split;
    }
    if (cmd[split] == ' ') {
        cmd[split] = '\0';
        return split + 1;
    }
    return split;
}

int process_ls(struct terminal* terminal, char* arg) {
    size_t inode_index = terminal->inode_index_;
    if (strcmp(arg, "") != 0) { // is argument
        inode_index = parse_path(terminal->fs_fd_, inode_index, arg);
    }
    if (inode_index == -1) {
        return 0;
    }
    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, inode_index);

    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 1) {
        printf("Not a directory!\n");
    }

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;

    for (size_t file_num = 0; file_num < directory->file_num_; ++file_num) {
        size_t file_inode_index = directory->files_[file_num];
        struct inode file_inode;
        read_inode(terminal->fs_fd_, &file_inode, file_inode_index);
        printf("%s\t", file_inode.name_);
    }
    printf("\n");

    return 0;
}

int mkdir(char* name, struct terminal* terminal) {
    size_t inode_index = make_directory(terminal->fs_fd_, name, terminal->inode_index_);

    struct inode inode;

    read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;
    directory->files_[directory->file_num_] = inode_index;
    ++directory->file_num_;

    write_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    return 0;
}

int change_directory(char* name, struct terminal* terminal) {
    size_t inode_index = terminal->inode_index_;
    size_t found_dir = exists(terminal->fs_fd_, inode_index, name);
    if (found_dir != -1) {
        struct inode inode;
        read_inode(terminal->fs_fd_, &inode, found_dir);
        while (inode.type_ == 2) { // find origin
            found_dir = inode.origin_;
            read_inode(terminal->fs_fd_, &inode, inode.origin_);
        }
        if (inode.type_ != 1) {
            printf("Not a directory.\n");
            return 0;
        }
        terminal->inode_index_ = found_dir;
    }
    return 0;
}

int touch(char* name, struct terminal* terminal) {
    size_t inode_index = make_file(terminal->fs_fd_, name);

    struct inode inode;

    read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;
    directory->files_[directory->file_num_] = inode_index;
    ++directory->file_num_;

    write_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    return 0;
}

int write_file(char* arg, struct terminal* terminal) {
    int split = parse_command(arg);
    char* name = arg;
    char* content = arg + split;

    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        printf("File not found. Use touch to create file.\n");
        return 0;
    }

    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, found_file);
    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 0) {
        printf("Not a writable file (directory or link).\n");
        return 0;
    }

    for (size_t i = 0; i < inode.data_blocks_num_; ++i) { // release old data
        release_block(terminal->fs_fd_, inode.data_blocks_[i]);
    }
    inode.data_blocks_num_ = 0;

    char last_writen = 0;
    while (strlen(content) >= BLOCK_SIZE || last_writen == 0) {
        size_t block_index = capture_block(terminal->fs_fd_);
        inode.data_blocks_[inode.data_blocks_num_] = block_index;
        ++inode.data_blocks_num_;
        struct block block;
        if(strlen(content) < BLOCK_SIZE) {
            memcpy(block.data_, content, strlen(content) + 1);
            last_writen = 1;
        } else {
            memcpy(block.data_, content, BLOCK_SIZE);
            content += BLOCK_SIZE;
        }
        write_block(terminal->fs_fd_, &block, block_index);
    }
    write_inode(terminal->fs_fd_, &inode, found_file);
    return 0;
}

int cat(char* name, struct terminal* terminal) {
    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        printf("File not found. Use touch to create file.\n");
        return 0;
    }

    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, found_file);
    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 0) {
        printf("Not a readable file (directory or link).\n");
        return 0;
    }

    for (size_t i = 0; i < inode.data_blocks_num_; ++i) {
        struct block block;
        read_block(terminal->fs_fd_, &block, inode.data_blocks_[i]);
        printf(block.data_);
    }

    printf("\n");

    return 0;
}

int remove_file_index(size_t file_index, struct terminal* terminal) {
    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, file_index);

    // remove recursively if directory
    if (inode.type_ == 1) { // is directory
        struct block block;
        read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

        struct directory* directory = (struct directory*)block.data_;
        for (size_t i = 0; i < directory->file_num_; ++i) {
            remove_file_index(directory->files_[i], terminal);
        }
    }

    // remove link from parent
    struct inode parent_inode;
    read_inode(terminal->fs_fd_, &parent_inode, terminal->inode_index_);
    struct block block;
    read_block(terminal->fs_fd_, &block, parent_inode.data_blocks_[0]);
    struct directory* directory = (struct directory*)block.data_;
    size_t index_in_dir = -1;
    for (size_t i = 0; i < directory->file_num_; ++i) {
        if (directory->files_[i] == file_index) {
            index_in_dir = i;
        }
    }
    if (index_in_dir != -1) {
        directory->files_[index_in_dir] = directory->files_[directory->file_num_ - 1];
        --directory->file_num_;
    }
    write_block(terminal->fs_fd_, &block, parent_inode.data_blocks_[0]);

    // release data blocks
    struct super_block sb;
    read_super_block(terminal->fs_fd_, &sb);
    inode.next_free_inode_index_ = sb.free_inode_;
    sb.free_inode_ = file_index;
    write_super_block(terminal->fs_fd_, &sb);
    write_inode(terminal->fs_fd_, &inode, file_index);
    return 0;
}

int remove_file(char* name, struct terminal* terminal) {
    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        printf("File not found. Use touch to create file.\n");
        return 0;
    }
    return remove_file_index(found_file, terminal);
}

int process_command(char* cmd, struct terminal* terminal) {
    int split = parse_command(cmd);
    char* arg = cmd + split;
    if (strcmp(cmd, "exit") == 0) {
        return 1;
    }
    if (strcmp(cmd, "ls") == 0) {
        return process_ls(terminal, arg);
    }
    if (strcmp(cmd, "mkdir") == 0) {
        return mkdir(arg, terminal);
    }
    if (strcmp(cmd, "cd") == 0) {
        return change_directory(arg, terminal);
    }
    if (strcmp(cmd, "touch") == 0) {
        return touch(arg, terminal);
    }
    if (strcmp(cmd, "write") == 0) {
        return write_file(arg, terminal);
    }
    if (strcmp(cmd, "cat") == 0) {
        return cat(arg, terminal);
    }
    if (strcmp(cmd, "rm") == 0) {
        return remove_file(arg, terminal);
    }
    printf("Command not found.\n");
    return -1;
}

void serve_terminal(struct terminal* terminal) {
//    struct super_block sb;
//    read_super_block(fs_fd_, &sb);
    printf("Welcome to terminal!\n");
    while (1) {
        struct inode inode;
        read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);

        printf("user::%s:: ", inode.name_);
        char cmd[CMD_MAX_LEN];
        for (int i = 0; i < CMD_MAX_LEN; ++i) {
            cmd[i] = '\0';
        }
        scanf("%[^\n]%*c", cmd);
        int status = process_command(cmd, terminal);
        if (status == 1) {
            break;
        }
    }
    printf("Exiting terminal.\n");
}