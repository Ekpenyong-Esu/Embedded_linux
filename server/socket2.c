#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define SOCKET_DATA "/var/tmp/aesdsocketdata"

static volatile int keep_running = 1;
static int server_socket = -1;

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        keep_running = 0;
    }
}

void cleanup() {
    if (server_socket != -1) {
        close(server_socket);
    }
    unlink(SOCKET_DATA);
    closelog();
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed");
        cleanup();
        return -1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Bind failed");
        cleanup();
        return -1;
    }

    if (listen(server_socket, 10) < 0) {
        syslog(LOG_ERR, "Listen failed");
        cleanup();
        return -1;
    }

    if (daemon_mode) {
        if (daemon(0, 0) < 0) {
            syslog(LOG_ERR, "Failed to daemonize");
            cleanup();
            return -1;
        }
    }

    while (keep_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        int file_des = open(SOCKET_DATA, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_des == -1) {
            syslog(LOG_ERR, "Failed to open data file");
            close(client_socket);
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = 0;
        int complete_packet = 0;

        while (!complete_packet && (bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            write(file_des, buffer, bytes_received);
            if (memchr(buffer, '\n', bytes_received)) {
                complete_packet = 1;
            }
        }

        lseek(file_des, 0, SEEK_SET);
        while ((bytes_received = read(file_des, buffer, BUFFER_SIZE)) > 0) {
            send(client_socket, buffer, bytes_received, 0);
        }

        close(file_des);
        close(client_socket);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    cleanup();
    return 0;
}
