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

size_t exists(int fs_fd, size_t dir_index, char* name) {
    struct inode inode;
    read_inode(fs_fd, &inode, dir_index);
    while (inode.type_ == 2) { // find origin
        read_inode(fs_fd, &inode, inode.origin_);
    }

    if (inode.type_ != 1) {
        printf("Not a directory!\n");
    }

    struct block block;
    read_block(fs_fd, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;

    size_t found_inode = 0;

    for (int i = 0; i < directory->file_num_; ++i) {
        struct inode file_inode;
        read_inode(fs_fd, &file_inode, directory->files_[i]);
        if (strcmp(file_inode.name_, name) == 0) {
            found_inode = directory->files_[i];
            break;
        }
        if (i == directory->file_num_ - 1) {
            printf("No such file or directory.\n");
            return -1;
        }
    }
    return found_inode;
}

size_t parse_path (int fs_fd, size_t inode_index, char* path) {
    struct inode inode;
    if (path[0] == '/') {
        read_inode(fs_fd, &inode, 0); // read root
        path += 1;
    } else {
        read_inode(fs_fd, &inode, inode_index);
    }

    if (strcmp(path, "") == 0) {
        return 0;
    }

    char* nxt = path;
    while (*nxt != '/' && *nxt != '\0') {
        ++nxt;
    }
    char last = (*nxt == '\0');
    *nxt = '\0';

    size_t found_inode = exists(fs_fd, inode_index, path);

    if (last) {
        return found_inode;
    } else {
        return parse_path(fs_fd, found_inode, nxt + 1);
    }
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
    size_t found_dir = exists(terminal->fs_fd_, terminal->inode_index_, name);
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