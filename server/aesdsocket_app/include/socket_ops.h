#ifndef SOCKET_OPS_H
#define SOCKET_OPS_H

#include "aesd_socket.h"

// Socket operations
int setup_server_socket(void);
ssize_t write_all(int fd, const char *buf, size_t count);
ssize_t read_all(int fd, char *buf, size_t count);

#endif // SOCKET_OPS_H
