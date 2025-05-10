#include "../include/signal_handler.h"
#include "../include/thread_manager.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static void signal_handler(int signo)
{
    (void)signo; // Unused parameter
    syslog(LOG_INFO, "Caught signal, exiting");
    keep_running = 0;
    cleanup_on_signal();
}

void cleanup_on_signal(void)
{
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

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&thread_list_mutex);

#if !USE_AESD_CHAR_DEVICE
    unlink(FILE_PATH);
#endif

    closelog();
}

void init_signal_handlers(void)
{
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = signal_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    if (sigaction(SIGINT, &sig_action, NULL) == -1)
    {
        syslog(LOG_ERR, "Failed to set SIGINT handler: %s", strerror(errno));
    }
    if (sigaction(SIGTERM, &sig_action, NULL) == -1)
    {
        syslog(LOG_ERR, "Failed to set SIGTERM handler: %s", strerror(errno));
    }
}
