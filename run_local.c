#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "terminal.h"


void check_args(int argc, char **argv) {
    if (argc != 2) {
        printf("Expected exactly one argument: fs_name");
        exit(1);
    }
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    char* fs_name = argv[1];
    int fs_fd = open_fs(fs_name);

    struct terminal terminal;
    terminal.fs_fd_ = fs_fd;
    terminal.inode_index_ = 0;

    serve_terminal(&terminal, stdin, stdout);

    close(fs_fd);
    return 0;
}
