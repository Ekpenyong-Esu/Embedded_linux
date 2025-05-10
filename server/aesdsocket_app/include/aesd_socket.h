#ifndef AESD_SOCKET_H
#define AESD_SOCKET_H

#ifndef USE_AESD_CHAR_DEVICE
#define USE_AESD_CHAR_DEVICE 1
#endif

#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


#if USE_AESD_CHAR_DEVICE
#define FILE_PATH "/dev/aesdchar"
#else
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INTERVAL 10
#endif

// Thread node structure for managing client threads
typedef struct thread_node
{
    pthread_t thread;
    struct thread_node *next;
    int completed;
} thread_node_t;

// Thread data structure for client handling
typedef struct
{
    int client_socket;
    char client_ip[INET_ADDRSTRLEN];
} thread_data_t;

// Global variables declarations
extern volatile sig_atomic_t keep_running;
extern int server_socket;
extern pthread_mutex_t file_mutex;
extern pthread_mutex_t thread_list_mutex;
extern thread_node_t *thread_list_head;

#if !USE_AESD_CHAR_DEVICE
extern pthread_t timer_thread;
#endif

#endif // AESD_SOCKET_H
