#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include "aesd_socket.h"

// Initialize signal handlers for the application
void init_signal_handlers(void);

// Cleanup resources on signal
void cleanup_on_signal(void);

#endif // SIGNAL_HANDLER_H
