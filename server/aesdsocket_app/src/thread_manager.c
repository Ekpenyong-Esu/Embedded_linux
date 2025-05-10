#include "../include/thread_manager.h"
#include "../include/socket_ops.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

void *handle_client(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    char *buffer = NULL;
    size_t buffer_size = BUFFER_SIZE;
    ssize_t bytes_received = 0;
    thread_node_t *my_node = NULL;
    int file_desc = -1;

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

#if USE_AESD_CHAR_DEVICE
        // For char driver, directly write to device - it handles the buffer internally
        file_desc = open(FILE_PATH, O_RDWR);
#else
        file_desc = open(FILE_PATH, O_RDWR | O_CREAT | O_APPEND, FILE_PERMISSIONS);
#endif
        if (file_desc == -1)
        {
            syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        if (write_all(file_desc, buffer, bytes_received) != bytes_received)
        {
            syslog(LOG_ERR, "Write error: %s", strerror(errno));
            close(file_desc);
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        if (memchr(buffer, '\n', bytes_received))
        {
#if !USE_AESD_CHAR_DEVICE
            // Only need to seek for regular file, not for char device
            if (lseek(file_desc, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "Seek error: %s", strerror(errno));
                close(file_desc);
                pthread_mutex_unlock(&file_mutex);
                break;
            }
#endif
            // Read back all accumulated data
            while ((bytes_received = read(file_desc, buffer, buffer_size - 1)) > 0)
            {
                if (write_all(data->client_socket, buffer, bytes_received) != bytes_received)
                {
                    syslog(LOG_ERR, "Send error: %s", strerror(errno));
                    break;
                }
            }
        }

        close(file_desc);
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

thread_node_t *add_thread_to_list(pthread_t thread)
{
    thread_node_t *node = malloc(sizeof(thread_node_t));
    if (!node)
    {
        return NULL;
    }

    node->thread = thread;
    node->completed = 0;

    pthread_mutex_lock(&thread_list_mutex);
    node->next = thread_list_head;
    thread_list_head = node;
    pthread_mutex_unlock(&thread_list_mutex);

    return node;
}

void remove_completed_threads(void)
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
            {
                prev->next = current->next;
            }
            else
            {
                thread_list_head = current->next;
            }
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

void cleanup_thread_list(void)
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
void *timestamp_thread(void *arg)
{
    char timestamp[100];
    struct timespec sleep_time = {TIMESTAMP_INTERVAL, 0};
    int file_desc = -1;

    while (keep_running)
    {
        time_t now;
        struct tm *timeinfo;

        time(&now);
        timeinfo = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "timestamp: %a, %d %b %Y %H:%M:%S %z\n", timeinfo);

        pthread_mutex_lock(&file_mutex);
        file_desc = open(FILE_PATH, O_RDWR | O_APPEND | O_CREAT, FILE_PERMISSIONS);
        if (file_desc != -1)
        {
            write_all(file_desc, timestamp, strlen(timestamp));
            close(file_desc);
        }
        pthread_mutex_unlock(&file_mutex);

        nanosleep(&sleep_time, NULL);
    }
    {
        return NULL;
    }
}
#endif
