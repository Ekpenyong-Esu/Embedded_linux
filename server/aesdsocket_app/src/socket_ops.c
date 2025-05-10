#include "../include/socket_ops.h"
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

ssize_t write_all(int fd, const char *buf, size_t count)
{
    size_t bytes_written = 0;
    while (bytes_written < count)
    {
        ssize_t ret = write(fd, buf + bytes_written, count - bytes_written);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (ret == 0)
            return bytes_written;
        bytes_written += ret;
    }
    return bytes_written;
}

ssize_t read_all(int fd, char *buf, size_t count)
{
    size_t bytes_read = 0;
    while (bytes_read < count)
    {
        ssize_t ret = read(fd, buf + bytes_read, count - bytes_read);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (ret == 0)
            return bytes_read;
        bytes_read += ret;
    }
    return bytes_read;
}

int setup_server_socket(void)
{
    struct sockaddr_in server_addr;
    int optval = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 10) < 0)
    {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    return 0;
}
