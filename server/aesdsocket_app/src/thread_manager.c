#include "../include/thread_manager.h"
#include "../../../aesd-char-driver/aesd_ioctl.h"
#include "../include/socket_ops.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
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

    // Main client communication loop - receive data from socket
    while ((bytes_received = recv(data->client_socket, buffer, buffer_size - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0'; // Null terminate for string operations

        /**
         * Check if this is an AESDCHAR_IOCSEEKTO command
         * Format: "AESDCHAR_IOCSEEKTO:X,Y" where X=command_index, Y=offset
         * This command should NOT be written to the driver, but instead
         * triggers an ioctl call followed by a read operation
         */
        if (strncmp(buffer, "AESDCHAR_IOCSEEKTO:", 19) == 0)
        {
            struct aesd_seekto seekto;
            char *command_str = buffer + 19; // Skip "AESDCHAR_IOCSEEKTO:" prefix

            /**
             * Parse X,Y from the command string
             * X = write_cmd (command index, 0-based)
             * Y = write_cmd_offset (byte offset within that command)
             */
            if (sscanf(command_str, "%u,%u", &seekto.write_cmd, &seekto.write_cmd_offset) != 2)
            {
                syslog(LOG_ERR, "Invalid AESDCHAR_IOCSEEKTO format, expected X,Y");
                break;
            }

            // Acquire mutex lock for thread-safe file operations
            if (pthread_mutex_lock(&file_mutex) != 0)
            {
                syslog(LOG_ERR, "Failed to acquire mutex");
                break;
            }

#if USE_AESD_CHAR_DEVICE
            // Open the character device for ioctl and read operations
            file_desc = open(FILE_PATH, O_RDWR);
            if (file_desc == -1)
            {
                syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
                pthread_mutex_unlock(&file_mutex);
                break;
            }

            /**
             * Call ioctl to seek to the specified position
             * This positions the file pointer to the exact location specified
             * by the command index and offset within that command
             */
            if (ioctl(file_desc, AESDCHAR_IOCSEEKTO, &seekto) == -1)
            {
                syslog(LOG_ERR, "IOCTL error: %s", strerror(errno));
                close(file_desc);
                pthread_mutex_unlock(&file_mutex);
                break;
            }

            /**
             * Read from the current position (set by ioctl) and send back over socket
             * This reads all remaining data from the seek position to end of buffer
             */
            while ((bytes_received = read(file_desc, buffer, buffer_size - 1)) > 0)
            {
                if (write_all(data->client_socket, buffer, bytes_received) != bytes_received)
                {
                    syslog(LOG_ERR, "Send error: %s", strerror(errno));
                    break;
                }
            }

            close(file_desc);
#endif
            pthread_mutex_unlock(&file_mutex);
            continue; // Don't process this as a regular write - ioctl handling complete
        }

        /**
         * Regular write processing for non-ioctl commands
         * This handles normal data writes to the character device or file
         */
        if (pthread_mutex_lock(&file_mutex) != 0)
        {
            syslog(LOG_ERR, "Failed to acquire mutex");
            break;
        }

#if USE_AESD_CHAR_DEVICE
        // For char driver, directly write to device - it handles the buffer internally
        file_desc = open(FILE_PATH, O_RDWR);
#else
        // For regular file, open with create and append flags
        file_desc = open(FILE_PATH, O_RDWR | O_CREAT | O_APPEND, FILE_PERMISSIONS);
#endif
        if (file_desc == -1)
        {
            syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        // Write received data to the device/file
        if (write_all(file_desc, buffer, bytes_received) != bytes_received)
        {
            syslog(LOG_ERR, "Write error: %s", strerror(errno));
            close(file_desc);
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        /**
         * Check if we received a complete command (terminated with newline)
         * If so, read back all accumulated data and send to client
         */
        if (memchr(buffer, '\n', bytes_received))
        {
#if USE_AESD_CHAR_DEVICE
            // For character device, close and reopen to reset position for reading
            close(file_desc);
            file_desc = open(FILE_PATH, O_RDONLY);
            if (file_desc == -1)
            {
                syslog(LOG_ERR, "Failed to reopen file for reading: %s", strerror(errno));
                pthread_mutex_unlock(&file_mutex);
                break;
            }
#else
            // Only need to seek for regular file, not for char device
            // Character device maintains its own position automatically
            if (lseek(file_desc, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "Seek error: %s", strerror(errno));
                close(file_desc);
                pthread_mutex_unlock(&file_mutex);
                break;
            }
#endif
            // Read back all accumulated data and send to client
            // Reset buffer for reading
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
