/**
 * @file aesd-circular-buffer-common.h
 * @brief Common definitions and utilities for AESD circular buffer implementation
 *
 * This header file provides common definitions, macros, and utilities shared
 * across all circular buffer implementation files. It handles the differences
 * between kernel and userspace environments, providing a unified interface
 * for debugging and basic operations.
 *
 * @author Mahonri Wozny
 * @date Created: November 2024
 * @version 1.0
 *
 * Features:
 * - Cross-platform compatibility (kernel/userspace)
 * - Unified debug logging interface
 * - Centralized buffer size configuration
 * - Common includes and definitions
 */

#ifndef AESD_CIRCULAR_BUFFER_COMMON_H
#define AESD_CIRCULAR_BUFFER_COMMON_H

// Conditional compilation for kernel vs userspace environments
#ifdef __KERNEL__
#include <linux/printk.h>
#include <linux/string.h>
/**
 * @brief Debug logging macro for kernel space
 *
 * In kernel space, debug messages are logged using printk with KERN_DEBUG
 * level and a consistent prefix for easy identification in kernel logs.
 * Messages can be filtered using kernel log level settings.
 */
#define DEBUG_LOG(fmt, ...) printk(KERN_DEBUG "aesd_circular: " fmt, ##__VA_ARGS__)
#else
#include <string.h>
/**
 * @brief Debug logging macro for userspace
 *
 * In userspace, debug logging is disabled to avoid dependencies on
 * specific logging libraries. This allows the same code to compile
 * in both environments without modification.
 */
#define DEBUG_LOG(fmt, ...) /* No logging in user space */
#endif

/**
 * @brief Maximum number of write operations supported by the circular buffer
 *
 * This constant defines the fixed size of the circular buffer array.
 * It determines how many separate write commands can be stored simultaneously
 * before older commands are overwritten. The value provides a balance between
 * memory usage and the ability to maintain command history.
 *
 * @note This value must be consistent across all files using the circular buffer
 * @note Changing this value requires recompilation of all dependent modules
 */
#define AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED 10

// Include the main circular buffer structure definitions
#include "aesd-circular-buffer.h"

#endif /* AESD_CIRCULAR_BUFFER_COMMON_H */
