
#include "threading.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    pthread_t thread;
    pthread_mutex_t mutex;

    // Initialize the mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Failed to initialize mutex\n");
        return EXIT_FAILURE;
    }

    // Start the thread
    if (!start_thread_obtaining_mutex(&thread, &mutex, 1000, 2000)) {
        printf("Failed to start thread\n");
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    // Wait for the thread to complete
    if (pthread_join(thread, NULL) != 0) {
        printf("Failed to join thread\n");
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    // Destroy the mutex
    pthread_mutex_destroy(&mutex);

    printf("Thread completed successfully\n");
    return EXIT_SUCCESS;
}
