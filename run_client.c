#include <stdio.h>
#include <stdlib.h>
#include "client.h"


void check_args(int argc, char **argv) {
    if (argc != 3) {
        printf("Expected exactly one argument: address port");
        exit(1);
    }
}

int main(int argc, char **argv) {
    check_args(argc, argv);

    in_addr_t addr = inet_addr(argv[1]);
    in_port_t port = htons(strtol(argv[2], NULL, 10));

    run_client(addr, port);

    return 0;
}
