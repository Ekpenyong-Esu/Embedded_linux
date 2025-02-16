// Standard library includes
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
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

// Configuration constants
#define PORT 9000                           // Server port number
#define BUFFER_SIZE 1024                    // Size of data buffer for reading/writing
#define FILE_PATH "/var/tmp/aesdsocketdata" // Path to data storage file
// File permissions: User(RW) Group(RW) Others(RW)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define TIMESTAMP_INTERVAL 10 // Interval for timestamp logging (seconds)

// Structure to maintain linked list of threads
typedef struct thread_node
{
    pthread_t thread;         // Thread identifier
    struct thread_node *next; // Pointer to next node
} thread_node_t;

// Structure to pass data to thread handlers
typedef struct
{
    int client_socket;               // Client socket file descriptor
    char client_ip[INET_ADDRSTRLEN]; // Client IP address string
} thread_data_t;

// Global variables
int server_socket = -1;                 // Server socket file descriptor
int file_des = -1;                      // Data file descriptor
volatile sig_atomic_t keep_running = 1; // Control flag for main loop

// Mutexes for thread synchronization
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;        // Protects file operations
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects thread list
thread_node_t *thread_list_head = NULL;                        // Head of thread linked list
pthread_t timer_thread;                                        // Thread for timestamp logging

// Function prototypes
void *handle_client(void *arg);
void *timestamp_thread(void *arg);
void cleanup_thread_list(void);
void add_thread_to_list(pthread_t thread);
void remove_thread_from_list(pthread_t thread);

/**
 * Signal handler for graceful shutdown
 * Handles SIGINT and SIGTERM signals
 */
void signal_handler(int signal_number)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    keep_running = 0;

    // Cancel the timer thread
    pthread_cancel(timer_thread);
    pthread_join(timer_thread, NULL);

    // Cleanup and close sockets
    cleanup_thread_list();

    if (server_socket != -1)
    {
        close(server_socket);
    }
    if (file_des != -1)
    {
        close(file_des);
    }

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&thread_list_mutex);

    unlink(FILE_PATH);
    closelog();
    exit(0);
}

/**
 * Timer thread function
 * Writes timestamp to file every TIMESTAMP_INTERVAL seconds
 */
void *timestamp_thread(void *arg)
{
    while (keep_running)
    {
        time_t now = 0;
        struct tm *time_info = NULL;
        char timestamp[100];

        time(&now);
        time_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "timestamp: %a, %d %b %Y %H:%M:%S %z\n", time_info);

        pthread_mutex_lock(&file_mutex);
        file_des = open(FILE_PATH, O_WRONLY | O_APPEND | O_CREAT, FILE_PERMISSIONS);
        if (file_des != -1)
        {
            write(file_des, timestamp, strlen(timestamp));
            close(file_des);
        }
        pthread_mutex_unlock(&file_mutex);

        sleep(TIMESTAMP_INTERVAL);
    }
    return NULL;
}

/**
 * Client handler thread function
 * Handles individual client connections
 * Receives data and sends back file contents
 */
void *handle_client(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = 0;

    syslog(LOG_INFO, "Accepted connection from %s", data->client_ip);

    while ((bytes_received = recv(data->client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        pthread_mutex_lock(&file_mutex);
        file_des = open(FILE_PATH, O_WRONLY | O_APPEND | O_CREAT, FILE_PERMISSIONS);
        if (file_des != -1)
        {
            write(file_des, buffer, bytes_received);
            close(file_des);
        }
        pthread_mutex_unlock(&file_mutex);

        if (memchr(buffer, '\n', bytes_received) != NULL)
        {
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }

    // Send file contents back to client
    pthread_mutex_lock(&file_mutex);
    file_des = open(FILE_PATH, O_RDONLY);
    if (file_des != -1)
    {
        while ((bytes_received = read(file_des, buffer, BUFFER_SIZE)) > 0)
        {
            send(data->client_socket, buffer, bytes_received, 0);
        }
        close(file_des);
    }
    pthread_mutex_unlock(&file_mutex);

    close(data->client_socket);
    syslog(LOG_INFO, "Closed connection from %s", data->client_ip);
    free(data);
    return NULL;
}

/**
 * Adds a thread to the thread tracking list
 * @param thread Thread ID to add
 */
void add_thread_to_list(pthread_t thread)
{
    thread_node_t *node = malloc(sizeof(thread_node_t));
    node->thread = thread;

    pthread_mutex_lock(&thread_list_mutex);
    node->next = thread_list_head;
    thread_list_head = node;
    pthread_mutex_unlock(&thread_list_mutex);
}

/**
 * Removes a thread from the thread tracking list
 * @param thread Thread ID to remove
 */
void remove_thread_from_list(pthread_t thread)
{
    pthread_mutex_lock(&thread_list_mutex);
    thread_node_t *current = thread_list_head;
    thread_node_t *prev = NULL;

    while (current != NULL)
    {
        if (pthread_equal(current->thread, thread))
        {
            if (prev == NULL)
            {
                thread_list_head = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&thread_list_mutex);
}

/**
 * Cleans up all threads in the list
 * Joins all threads and frees memory
 */
void cleanup_thread_list(void)
{
    pthread_mutex_lock(&thread_list_mutex);
    thread_node_t *current = thread_list_head;

    while (current != NULL)
    {
        thread_node_t *next = current->next;
        pthread_join(current->thread, NULL);
        free(current);
        current = next;
    }
    thread_list_head = NULL;
    pthread_mutex_unlock(&thread_list_mutex);
}

/**
 * Main program entry point
 * Sets up server socket and handles incoming connections
 */
int main(int argc, char *argv[])
{
    // Parse command line arguments for daemon mode
    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
    {
        daemon_mode = 1;
    }

    // Initialize syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Set up signal handlers for graceful termination
    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(signal_action));
    signal_action.sa_handler = signal_handler;
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGTERM, &signal_action, NULL);

    // Create and configure server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        return -1;
    }

    // Enable address reuse
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configure server address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    // Start listening for connections
    if (listen(server_socket, 10) < 0)
    {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_socket);
        return -1;
    }

    // Switch to daemon mode if requested
    if (daemon_mode)
    {
        daemon(0, 0);
    }

    // Start timestamp logging thread
    pthread_create(&timer_thread, NULL, timestamp_thread, NULL);

    // Main server loop
    while (keep_running)
    {
        // Accept new client connections
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        // Create new thread for client handling
        thread_data_t *thread_data = malloc(sizeof(thread_data_t));
        thread_data->client_socket = client_socket;
        inet_ntop(AF_INET, &client_addr.sin_addr, thread_data->client_ip, INET_ADDRSTRLEN);

        pthread_t thread = 0;
        pthread_create(&thread, NULL, handle_client, thread_data);
        add_thread_to_list(thread);
    }

    // Cleanup and exit
    cleanup_thread_list();
    return 0;
}
