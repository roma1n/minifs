#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "server.h"


void check_args(int argc, char **argv) {
    if (argc != 3) {
        printf("Expected exactly two argument: fs_name port");
        exit(1);
    }
}

int main(int argc, char **argv) {
    check_args(argc, argv);
    char* fs_name = argv[1];
    int fs_fd = open_fs(fs_name);

    in_port_t port = htons(strtol(argv[2], NULL, 10));

    run_server(fs_fd, port);

    close(fs_fd);
    return 0;
}
