#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "aesd_socket.h"

// Thread management functions
void *handle_client(void *arg);
thread_node_t *add_thread_to_list(pthread_t thread);
void remove_completed_threads(void);
void cleanup_thread_list(void);

#if !USE_AESD_CHAR_DEVICE
void *timestamp_thread(void *arg);
#endif

#endif // THREAD_MANAGER_H
