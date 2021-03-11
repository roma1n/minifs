#ifndef MINIFS_TERMINAL_H
#define MINIFS_TERMINAL_H


#include "terminal.h"
#include "fs.h"
#include <string.h>


struct terminal {
    int fs_fd_;
    size_t inode_index_;
};

void serve_terminal(struct terminal* terminal);


#endif