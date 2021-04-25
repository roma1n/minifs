#ifndef MINIFS_CLIENT_H
#define MINIFS_CLIENT_H


#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int run_client(in_addr_t addr, in_port_t port);


#endif