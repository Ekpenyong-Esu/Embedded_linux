#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int server_socket;
int client_socket;
int file_des;
volatile sig_atomic_t keep_running = 1;

void signal_handler(int signal_number) {
    (void)signal_number;
    syslog(LOG_INFO, "Caught signal, exiting");
    keep_running = 0;
    if (server_socket != -1) {
        close(server_socket);
    }
    if (client_socket != -1) {
        close(client_socket);
    }
    if (file_des != -1) {
        close(file_des);
    }
    remove(FILE_PATH);
    closelog();
    exit(0);
}

void cleanup() {
    if (server_socket != -1) {
        close(server_socket);
    }
    if (client_socket != -1) {
        close(client_socket);
    }
    if (file_des != -1) {
        close(file_des);
    }
    unlink(FILE_PATH);  // Use unlink instead of remove
    closelog();
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;

    // Check for daemon mode flag -d and set it if present
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    // Setup syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Setup signal handlers
    struct sigaction signal_hand;
    memset(&signal_hand, 0, sizeof(signal_hand));
    signal_hand.sa_handler = signal_handler;
    if (sigaction(SIGINT, &signal_hand, NULL) == -1 || sigaction(SIGTERM, &signal_hand, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handlers: %s", strerror(errno));
        return -1;
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }

    // Enable address reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    // Setup address structure
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    // Daemonize if -d flag is set
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
            return -1;
        }
        if (pid > 0) {
            exit(0);
        }
        if (setsid() < 0) {
            syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
            return -1;
        }
        if (chdir("/") < 0) {
            syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
            return -1;
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Accept connections from clients
    while (keep_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Open file for each connection
        file_des = open(FILE_PATH, O_RDWR | O_CREAT | O_APPEND, FILE_PERMISSIONS);
        if (file_des == -1) {
            syslog(LOG_ERR, "File open failed: %s", strerror(errno));
            continue;
        }

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            close(file_des);
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = 0;

        // Clear buffer before use
        memset(buffer, 0, BUFFER_SIZE);

        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            ssize_t bytes_written = write(file_des, buffer, bytes_received);
            if (bytes_written == -1) {
                syslog(LOG_ERR, "Write failed: %s", strerror(errno));
                break;
            }
            if (memchr(buffer, '\n', bytes_received) != NULL) {
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }

        // Flush the file to ensure data is written to disk
        if (fsync(file_des) == -1) {
            syslog(LOG_ERR, "fsync failed: %s", strerror(errno));
        }

        // Send file contents back to client
        lseek(file_des, 0, SEEK_SET);
        while ((bytes_received = read(file_des, buffer, BUFFER_SIZE)) > 0) {
            ssize_t bytes_sent = send(client_socket, buffer, bytes_received, 0);
            if (bytes_sent < 0) {
                syslog(LOG_ERR, "Send failed: %s", strerror(errno));
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_socket);
        close(file_des);
    }

    cleanup();
    return 0;
}
