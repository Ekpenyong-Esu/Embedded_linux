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

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#if USE_AESD_CHAR_DEVICE
#define FILE_PATH "/dev/aesdchar"
#else
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INTERVAL 10
#endif

typedef struct thread_node
{
    pthread_t thread;
    struct thread_node *next;
    int completed; // Flag to mark completed threads
} thread_node_t;

typedef struct
{
    int client_socket;
    char client_ip[INET_ADDRSTRLEN];
} thread_data_t;

static volatile sig_atomic_t keep_running = 1;
static int server_socket = -1;
static int data_fd = -1;
static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static thread_node_t *thread_list_head = NULL;

#if !USE_AESD_CHAR_DEVICE
static pthread_t timer_thread;
#endif

static void *handle_client(void *arg);
static void cleanup_thread_list(void);
static thread_node_t *add_thread_to_list(pthread_t thread);
static void remove_thread_from_list(pthread_t thread);
static int setup_server_socket(void);
static void handle_signals(void);
static ssize_t write_all(int fd, const char *buf, size_t count);
static ssize_t read_all(int fd, char *buf, size_t count);

#if !USE_AESD_CHAR_DEVICE
static void *timestamp_thread(void *arg);
#endif

static void signal_handler(int signo)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    keep_running = 0;

#if !USE_AESD_CHAR_DEVICE
    if (timer_thread)
    {
        pthread_cancel(timer_thread);
        pthread_join(timer_thread, NULL);
    }
#endif

    if (server_socket != -1)
    {
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
    }

    cleanup_thread_list();

    if (data_fd != -1)
    {
        close(data_fd);
        data_fd = -1;
    }

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&thread_list_mutex);

#if !USE_AESD_CHAR_DEVICE
    unlink(FILE_PATH);
#endif

    closelog();
    exit(0);
}

static ssize_t write_all(int fd, const char *buf, size_t count)
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

static ssize_t read_all(int fd, char *buf, size_t count)
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

static void *handle_client(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    char *buffer = NULL;
    size_t buffer_size = BUFFER_SIZE;
    ssize_t bytes_received;
    thread_node_t *my_node = NULL;

    pthread_t self = pthread_self();
    my_node = add_thread_to_list(self);

    syslog(LOG_INFO, "Accepted connection from %s", data->client_ip);

    buffer = malloc(buffer_size);
    if (!buffer)
    {
        syslog(LOG_ERR, "Failed to allocate buffer");
        goto cleanup;
    }

    while ((bytes_received = recv(data->client_socket, buffer, buffer_size - 1, 0)) > 0)
    {
        if (pthread_mutex_lock(&file_mutex) != 0)
        {
            syslog(LOG_ERR, "Failed to acquire mutex");
            break;
        }

        if (write_all(data_fd, buffer, bytes_received) != bytes_received)
        {
            syslog(LOG_ERR, "Write error: %s", strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        if (memchr(buffer, '\n', bytes_received))
        {
#if !USE_AESD_CHAR_DEVICE
            if (lseek(data_fd, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "Seek error: %s", strerror(errno));
                pthread_mutex_unlock(&file_mutex);
                break;
            }
#endif
            while ((bytes_received = read(data_fd, buffer, buffer_size - 1)) > 0)
            {
                if (write_all(data->client_socket, buffer, bytes_received) != bytes_received)
                {
                    syslog(LOG_ERR, "Send error: %s", strerror(errno));
                    break;
                }
            }
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        pthread_mutex_unlock(&file_mutex);
    }

cleanup:
    free(buffer);
    close(data->client_socket);
    syslog(LOG_INFO, "Closed connection from %s", data->client_ip);
    free(data);

    if (my_node)
    {
        pthread_mutex_lock(&thread_list_mutex);
        my_node->completed = 1;
        pthread_mutex_unlock(&thread_list_mutex);
    }

    return NULL;
}

static thread_node_t *add_thread_to_list(pthread_t thread)
{
    thread_node_t *node = malloc(sizeof(thread_node_t));
    if (!node)
        return NULL;

    node->thread = thread;
    node->completed = 0;

    pthread_mutex_lock(&thread_list_mutex);
    node->next = thread_list_head;
    thread_list_head = node;
    pthread_mutex_unlock(&thread_list_mutex);

    return node;
}

static void remove_completed_threads(void)
{
    pthread_mutex_lock(&thread_list_mutex);
    thread_node_t *current = thread_list_head;
    thread_node_t *prev = NULL;

    while (current)
    {
        if (current->completed)
        {
            thread_node_t *to_free = current;
            if (prev)
                prev->next = current->next;
            else
                thread_list_head = current->next;
            current = current->next;
            pthread_join(to_free->thread, NULL);
            free(to_free);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }
    pthread_mutex_unlock(&thread_list_mutex);
}

static void cleanup_thread_list(void)
{
    pthread_mutex_lock(&thread_list_mutex);
    while (thread_list_head)
    {
        thread_node_t *temp = thread_list_head;
        thread_list_head = thread_list_head->next;
        pthread_join(temp->thread, NULL);
        free(temp);
    }
    pthread_mutex_unlock(&thread_list_mutex);
}

#if !USE_AESD_CHAR_DEVICE
static void *timestamp_thread(void *arg)
{
    char timestamp[100];
    struct timespec sleep_time = {TIMESTAMP_INTERVAL, 0};

    while (keep_running)
    {
        time_t now;
        struct tm *timeinfo;

        time(&now);
        timeinfo = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "timestamp: %a, %d %b %Y %H:%M:%S %z\n", timeinfo);

        pthread_mutex_lock(&file_mutex);
        if (data_fd != -1)
            write_all(data_fd, timestamp, strlen(timestamp));
        pthread_mutex_unlock(&file_mutex);

        nanosleep(&sleep_time, NULL);
    }
    return NULL;
}
#endif

static int setup_server_socket(void)
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

static void handle_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
        syslog(LOG_ERR, "Failed to set SIGINT handler: %s", strerror(errno));
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        syslog(LOG_ERR, "Failed to set SIGTERM handler: %s", strerror(errno));
}

int main(int argc, char *argv[])
{
    int daemon_mode = (argc == 2 && strcmp(argv[1], "-d") == 0);

    openlog("aesdsocket", LOG_PID, LOG_USER);
    handle_signals();

    // Open data file/device first
    data_fd = open(FILE_PATH, O_RDWR | O_CREAT, FILE_PERMISSIONS);
    if (data_fd == -1)
    {
        syslog(LOG_ERR, "Failed to open %s: %s", FILE_PATH, strerror(errno));
        return -1;
    }

    if (setup_server_socket() != 0)
    {
        close(data_fd);
        return -1;
    }

    if (daemon_mode && daemon(0, 0) == -1)
    {
        syslog(LOG_ERR, "Failed to daemonize: %s", strerror(errno));
        close(server_socket);
        close(data_fd);
        return -1;
    }

#if !USE_AESD_CHAR_DEVICE
    if (pthread_create(&timer_thread, NULL, timestamp_thread, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed to create timer thread: %s", strerror(errno));
        close(server_socket);
        close(data_fd);
        return -1;
    }
#endif

    while (keep_running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket;

        remove_completed_threads();

        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (errno == EINTR)
                continue;
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
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

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, thread_data) != 0)
        {
            syslog(LOG_ERR, "Failed to create thread: %s", strerror(errno));
            free(thread_data);
            close(client_socket);
            continue;
        }
    }

    cleanup_thread_list();
    return 0;
}
