#include "server.h"


int process_query(int fs_fd, int conn) {
    struct terminal terminal;
    terminal.fs_fd_ = fs_fd;
    terminal.inode_index_ = 0;

    int in_fd = dup(conn);
    FILE* in = fdopen(in_fd, "r");
    int out_fd = dup(conn);
    FILE* out = fdopen(out_fd, "w");

    serve_terminal(&terminal, in, out);

    close(in_fd);
    fclose(in);
    close(out_fd);
    fclose(out);
}

int run_server(int fs_fd, in_port_t port) {
    // config server
    int sockfd;
    int conn;
    struct sockaddr_in sock_addr;
    int opt = 1;
    int addrlen = sizeof(sock_addr);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = port;

    // run server
    if (bind(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr))) {
        goto exit_with_error;
    }
    if (listen(sockfd, 128) < 0) {
        goto exit_with_error;
    }

    // server loop
    int stop = 0;
    while (stop != 1) {
        if ((conn = accept(sockfd, (struct sockaddr *)&sock_addr, (socklen_t*)&addrlen)) < 0) {
            goto exit_with_error;
        }
        process_query(fs_fd, conn);
        close(conn);
    }

    // stop server
exit:
    close(sockfd);
    return 0;

exit_with_error:
    close(conn);
    close(sockfd);
    return 1;
}
