#ifndef AESD_CIRCULAR_BUFFER_COMMON_H
#define AESD_CIRCULAR_BUFFER_COMMON_H

#ifdef __KERNEL__
#include <linux/printk.h>
#include <linux/string.h>
#define DEBUG_LOG(fmt, ...) printk(KERN_DEBUG "aesd_circular: " fmt, ##__VA_ARGS__)
#else
#include <string.h>
#define DEBUG_LOG(fmt, ...) /* No logging in user space */
#endif

#define AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED 10

#include "aesd-circular-buffer.h"

#endif /* AESD_CIRCULAR_BUFFER_COMMON_H */
