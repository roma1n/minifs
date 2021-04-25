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

int process_ls(struct terminal* terminal, char* arg, char* buf) {
    size_t inode_index = terminal->inode_index_;
    if (strcmp(arg, "") != 0) { // is argument
        inode_index = parse_path(terminal->fs_fd_, inode_index, arg);
    }
    if (inode_index == -1) {
        sprintf(buf + strlen(buf), "\n");
        return 0;
    }
    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, inode_index);

    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 1) {
        sprintf(buf + strlen(buf), "Not a directory!\n");
        return 0;
    }

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;

    for (size_t file_num = 0; file_num < directory->file_num_; ++file_num) {
        size_t file_inode_index = directory->files_[file_num];
        struct inode file_inode;
        read_inode(terminal->fs_fd_, &file_inode, file_inode_index);
        sprintf(buf + strlen(buf), "%s\t", file_inode.name_);
    }
    sprintf(buf + strlen(buf), "\n");

    return 0;
}

int mkdir(char* name, struct terminal* terminal, char* buf) {
    size_t inode_index = make_directory(terminal->fs_fd_, name, terminal->inode_index_);

    struct inode inode;

    read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;
    directory->files_[directory->file_num_] = inode_index;
    ++directory->file_num_;

    write_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    sprintf(buf + strlen(buf), "\n");

    return 0;
}

int change_directory(char* name, struct terminal* terminal, char* buf) {
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
            sprintf(buf + strlen(buf), "Not a directory.\n");
            return 0;
        }
        terminal->inode_index_ = found_dir;
    }

    sprintf(buf + strlen(buf), "\n");

    return 0;
}

int touch(char* name, struct terminal* terminal, char* buf) {
    size_t inode_index = make_file(terminal->fs_fd_, name);

    struct inode inode;

    read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);

    struct block block;
    read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    struct directory* directory = (struct directory*)block.data_;
    directory->files_[directory->file_num_] = inode_index;
    ++directory->file_num_;

    write_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

    sprintf(buf + strlen(buf), "\n");

    return 0;
}

int write_file(char* arg, struct terminal* terminal, char* buf) {
    int split = parse_command(arg);
    char* name = arg;
    char* content = arg + split;

    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        sprintf(buf + strlen(buf), "File not found. Use touch to create file.\n");
        return 0;
    }

    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, found_file);
    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 0) {
        sprintf(buf + strlen(buf), "Not a writable file (directory or link).\n");
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
    sprintf(buf + strlen(buf), "\n");
    return 0;
}

int cat(char* name, struct terminal* terminal, char* buf) {
    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        sprintf(buf + strlen(buf), "File not found. Use touch to create file.\n");
        return 0;
    }

    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, found_file);
    while (inode.type_ == 2) { // find origin
        read_inode(terminal->fs_fd_, &inode, inode.origin_);
    }

    if (inode.type_ != 0) {
        sprintf(buf + strlen(buf), "Not a readable file (directory or link).\n");
        return 0;
    }

    for (size_t i = 0; i < inode.data_blocks_num_; ++i) {
        struct block block;
        read_block(terminal->fs_fd_, &block, inode.data_blocks_[i]);
        sprintf(buf + strlen(buf), "%s", block.data_);
    }

    sprintf(buf + strlen(buf), "\n");

    return 0;
}

int remove_file_index(size_t file_index, struct terminal* terminal, char* buf) {
    if (file_index == 0) {
        sprintf(buf + strlen(buf), "Impossible to delete root.\n");
        return 0;
    }
    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, file_index);

    // remove recursively if directory
    if (inode.type_ == 1) { // is directory
        struct block block;
        read_block(terminal->fs_fd_, &block, inode.data_blocks_[0]);

        struct directory* directory = (struct directory*)block.data_;
        for (size_t i = 0; i < directory->file_num_; ++i) {
            remove_file_index(directory->files_[i], terminal, buf);
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
    if (strlen(buf) == 0) {
        sprintf(buf + strlen(buf), "\n");
    }
    return 0;
}

int remove_file(char* name, struct terminal* terminal, char* buf) {
    if (strcmp(name, ".") == 0) {
        sprintf(buf + strlen(buf), "Impossible to delete link to self.\n");
        return 0;
    }
    if (strcmp(name, "..") == 0) {
        sprintf(buf + strlen(buf), "Impossible to delete link to parent.\n");
        return 0;
    }
    size_t found_file = exists(terminal->fs_fd_, terminal->inode_index_, name);
    if (found_file == -1) {
        sprintf(buf + strlen(buf), "\n");
        return 0;
    }
    return remove_file_index(found_file, terminal, buf);
}

int upload(char* dest, char* host_src, struct terminal* terminal, char* buf) {
    char arg[BLOCK_SIZE];
    size_t len = strlen(dest);
    memcpy(arg, dest, len);
    *(arg + len) = ' ';

    int src_fd = open(host_src, O_RDONLY);
    lseek(src_fd, 0L, SEEK_END);
    size_t sz = lseek(src_fd, 0, SEEK_CUR);
    lseek(src_fd, 0L, SEEK_SET);
    read(src_fd, arg + len + 1, sz);
    close(src_fd);

    write_file(arg, terminal, buf);
    sprintf(buf + strlen(buf), "\n");
    return 0;
}

int download(char* src, char* host_dest, struct terminal* terminal, char* buf) {
    int dest_fd = open(host_dest, O_WRONLY | O_CREAT, 0666);

    size_t file_index = exists(terminal->fs_fd_, terminal->inode_index_, src);
    if (file_index == -1) {
        return 0;
    }
    struct inode inode;
    read_inode(terminal->fs_fd_, &inode, file_index);
    for (size_t i = 0; i < inode.data_blocks_num_; ++i) {
        struct block block;
        read_block(terminal->fs_fd_, &block, inode.data_blocks_[i]);
        write(dest_fd, block.data_, strlen(block.data_));
    }
    close(dest_fd);
    sprintf(buf + strlen(buf), "\n");
    return 0;
}

int process_command(char* cmd, struct terminal* terminal, FILE* out) {
    char* buf = (char*) malloc(4096 * sizeof(char));

    int split = parse_command(cmd);
    char* arg = cmd + split;
    int ret = -1;
    if (strcmp(cmd, "exit") == 0) {
        ret = 1;
    }
    if (strcmp(cmd, "ls") == 0) {
        ret = process_ls(terminal, arg, buf);
    }
    if (strcmp(cmd, "mkdir") == 0) {
        ret = mkdir(arg, terminal, buf);
    }
    if (strcmp(cmd, "cd") == 0) {
        ret = change_directory(arg, terminal, buf);
    }
    if (strcmp(cmd, "touch") == 0) {
        ret = touch(arg, terminal, buf);
    }
    if (strcmp(cmd, "write") == 0) {
        ret = write_file(arg, terminal, buf);
    }
    if (strcmp(cmd, "cat") == 0) {
        ret = cat(arg, terminal, buf);
    }
    if (strcmp(cmd, "rm") == 0) {
        ret = remove_file(arg, terminal, buf);
    }
    if (strcmp(cmd, "upload") == 0) {
        int arg_split = parse_command(arg);
        char* dest = arg;
        char* host_src = arg + arg_split;
        ret = upload(dest, host_src, terminal, buf);
    }
    if (strcmp(cmd, "download") == 0) {
        int arg_split = parse_command(arg);
        char* src = arg;
        char* host_desr = arg + arg_split;
        ret = download(src, host_desr, terminal, buf);
    }

    if (ret == -1) {
        sprintf(buf + strlen(buf), "Command not found: \"%s\".\n", cmd);
    }
    size_t len = strlen(buf);
    write(fileno(out), buf, strlen(buf));

    return ret;
}

void serve_terminal(struct terminal* terminal, FILE* in, FILE* out) {
    while (1) {
        struct inode inode;
        read_inode(terminal->fs_fd_, &inode, terminal->inode_index_);
        char* buf = (char*) malloc(4096 * sizeof(char));
        sprintf(buf + strlen(buf), "user::%s:: \n", inode.name_);
        size_t len = strlen(buf);
        write(fileno(out), buf, strlen(buf));

        char* cmd = NULL;
        len = 0;
        getline(&cmd, &len, in);
        len = strlen(cmd);
        cmd[len - 1] = '\0';
        int status = process_command(cmd, terminal, out);
        if (status == 1) {
            break;
        }
        free(cmd);
    }
}