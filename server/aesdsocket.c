#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "../server/aesdsocket_app/include/aesd_socket.h"
#include "../server/aesdsocket_app/include/signal_handler.h"
#include "../server/aesdsocket_app/include/socket_ops.h"
#include "../server/aesdsocket_app/include/thread_manager.h"

// Global variables defined in aesd_socket.h
volatile sig_atomic_t keep_running = 1;
int server_socket = -1;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
thread_node_t *thread_list_head = NULL;

#if !USE_AESD_CHAR_DEVICE
pthread_t timer_thread;
#endif

int main(int argc, char *argv[])
{
    int daemon_mode = (argc == 2 && strcmp(argv[1], "-d") == 0);

    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Initialize signal handlers
    init_signal_handlers();

    // Setup server socket
    if (setup_server_socket() != 0)
    {
        cleanup_on_signal();
        return -1;
    }

    if (daemon_mode && daemon(0, 0) == -1)
    {
        syslog(LOG_ERR, "Failed to daemonize");
        cleanup_on_signal();
        return -1;
    }

#if !USE_AESD_CHAR_DEVICE
    if (pthread_create(&timer_thread, NULL, timestamp_thread, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to create timer thread");
        cleanup_on_signal();
        return -1;
    }
#endif

    while (keep_running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        thread_data_t *thread_data = malloc(sizeof(thread_data_t));
        if (!thread_data)
        {
            syslog(LOG_ERR, "Failed to allocate thread data");
            close(client_socket);
            continue;
        }

        thread_data->client_socket = client_socket;
        inet_ntop(AF_INET, &client_addr.sin_addr, thread_data->client_ip, INET_ADDRSTRLEN);

        pthread_t thread = {0};
        if (pthread_create(&thread, NULL, handle_client, thread_data) != 0)
        {
            syslog(LOG_ERR, "Failed to create thread");
            free(thread_data);
            close(client_socket);
            continue;
        }

        // Cleanup completed threads
        remove_completed_threads();
    }

    cleanup_on_signal();
    return 0;
}
