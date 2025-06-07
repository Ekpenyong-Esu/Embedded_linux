/**
 * @file aesd-circular-buffer.c
 * @brief Main circular buffer module for AESD character driver
 *
 * This file serves as the main entry point for the AESD circular buffer
 * functionality, providing kernel and userspace compatibility through
 * conditional compilation. It defines the debug logging mechanism and
 * declares external function interfaces implemented in separate source files.
 *
 * @author Mahonri Wozny
 * @date Created: November 2024
 * @version 1.0
 *
 * Features:
 * - Kernel/userspace compatibility through conditional compilation
 * - Centralized debug logging configuration
 * - Clean separation of interface and implementation
 * - External function declarations for modular design
 */

// Conditional compilation for kernel vs userspace environments
#ifdef __KERNEL__
#include <linux/printk.h>
#include <linux/string.h>
// Define debug logging for kernel space using printk
#define DEBUG_LOG(fmt, ...) printk(KERN_DEBUG "aesd_circular: " fmt, ##__VA_ARGS__)
#else
#include <string.h>
// Disable debug logging in userspace to avoid dependencies
#define DEBUG_LOG(fmt, ...) /* No logging in user space */
#endif

#include "circular-buffer/include/aesd-circular-buffer-common.h"
#include "circular-buffer/include/aesd-circular-buffer.h"

// External function declarations - implementations are in circular-buffer/src/
// This design allows for modular compilation and clean separation of concerns

/**
 * @brief External declaration for circular buffer entry addition
 * Implemented in: circular-buffer/src/aesd-circular-buffer-add.c
 */
extern void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer,
                                           const struct aesd_buffer_entry *add_entry);

/**
 * @brief External declaration for circular buffer entry removal
 * Implemented in: circular-buffer/src/aesd-circular-buffer-remove.c
 */
extern void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer);

/**
 * @brief External declaration for circular buffer initialization
 * Implemented in: circular-buffer/src/aesd-circular-buffer-init.c
 */
extern void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer);

/**
 * @brief External declaration for circular buffer entry search by offset
 * Implemented in: circular-buffer/src/aesd-circular-buffer-find.c
 */
extern struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                                 size_t char_offset,
                                                                                 size_t *entry_offset_byte_rtn);
