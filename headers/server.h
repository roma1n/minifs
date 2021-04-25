#ifndef MINIFS_SERVER_H
#define MINIFS_SERVER_H


#include "terminal.h"
#include "fs.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

int run_server(int fs_fd, in_port_t port);

#endif