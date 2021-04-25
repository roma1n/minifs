#include "client.h"


int run_client(in_addr_t addr, in_port_t port) {
    // init connection
    int conn = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sock_addr;
    memset(&sock_addr, sizeof(sock_addr), 0);
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = port;
    sock_addr.sin_addr.s_addr = addr;

    if (connect(conn, (struct sockaddr*) &sock_addr, sizeof(sock_addr)) == -1) {
        perror("connect");
        close(conn);
        exit(1);
    }

    int in_fd = dup(conn);
    FILE* in = fdopen(in_fd, "r");
    int out_fd = dup(conn);
    FILE* out = fdopen(out_fd, "w");

    // client loop
    int stop = 0;

    char* in_buf;
    size_t in_len;
    char* out_buf;
    size_t out_len;

    while(stop != 1) {
        getline(&in_buf, &in_len, in);
        in_len = strlen(in_buf);
        in_buf[in_len - 1] = '\0';
        printf("%s", in_buf);
        fflush(stdout);

        out_buf = (char*) malloc(4096 * sizeof(char));
        scanf("%[^\n]%*c", out_buf);
        out_len = strlen(out_buf);
        out_buf[out_len] = '\n';
        out_buf[out_len + 1] = '\0';
        write(conn, out_buf, strlen(out_buf));

        if (strcmp(out_buf, "exit\n") == 0) {
            stop = 1;
            fflush(stdout);
            continue;
        }

        free(out_buf);

        getline(&in_buf, &in_len, in);
        in_len = strlen(in_buf);
        printf("%s", in_buf);
        fflush(stdout);
    }

    close(in_fd);
    fclose(in);
    close(out_fd);
    fclose(out);

    close(conn);
}