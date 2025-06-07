/**
 * @file aesdchar.h
 * @brief Header file for AESD character driver
 *
 * This header file contains all the necessary definitions, data structures,
 * and function declarations for the AESD character driver implementation.
 *
 * The driver provides:
 * - Character device interface for reading/writing data
 * - Circular buffer management for storing commands
 * - llseek operation support for positioning within data
 * - ioctl support for advanced seek operations
 * - Thread-safe operations using mutex locks
 *
 * @author Dan Walkes (original), Enhanced by Assignment Team
 * @date Created: Oct 23, 2019, Enhanced: June 7, 2025
 */

#ifndef AESD_CHAR_DRIVER_H
#define AESD_CHAR_DRIVER_H

#ifdef __KERNEL__
#include "../../aesd_ioctl.h"
#include "../../circular-buffer/include/aesd-circular-buffer.h"
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

/**
 * @brief Debug print macro for kernel space logging
 * @param fmt Format string for printf-style formatting
 * @param ... Variable arguments for the format string
 */
#define PDEBUG(fmt, ...) pr_debug("aesdchar: " fmt, ##__VA_ARGS__)
#else
#define PDEBUG(fmt, ...)
#endif

/**
 * @brief Main device structure for AESD character driver
 *
 * This structure contains all the state information needed for the
 * character driver operation, including the circular buffer for data
 * storage, synchronization primitives, and temporary buffers.
 */
struct aesd_dev
{
    /** @brief Circular buffer for storing complete commands */
    struct aesd_circular_buffer buffer;

    /** @brief Mutex for thread-safe access to device state */
    struct mutex lock;

    /** @brief Character device structure for kernel interface */
    struct cdev cdev;

    /** @brief Temporary buffer for accumulating partial write data */
    char *write_buf;

    /** @brief Current size of data in write_buf */
    size_t write_buf_size;
};

/* External variable declarations */
/** @brief Major device number (dynamically allocated) */
extern int aesd_major;

/** @brief Minor device number (always 0 for single device) */
extern int aesd_minor;

/** @brief File operations structure for the character device */
extern struct file_operations aesd_fops;

/* File operation function declarations */

/**
 * @brief Open operation for the AESD character device
 * @param inode Pointer to the inode structure
 * @param filp Pointer to the file structure
 * @return 0 on success, negative error code on failure
 */
int aesd_open(struct inode *inode, struct file *filp);

/**
 * @brief Release (close) operation for the AESD character device
 * @param inode Pointer to the inode structure
 * @param filp Pointer to the file structure
 * @return 0 on success, negative error code on failure
 */
int aesd_release(struct inode *inode, struct file *filp);

/**
 * @brief Read operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param buf User space buffer to read data into
 * @param count Number of bytes to read
 * @param f_pos Pointer to file position
 * @return Number of bytes read on success, negative error code on failure
 */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

/**
 * @brief Write operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param buf User space buffer containing data to write
 * @param count Number of bytes to write
 * @param f_pos Pointer to file position (not used in this driver)
 * @return Number of bytes written on success, negative error code on failure
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

/**
 * @brief Seek operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param offset Offset value for seeking
 * @param whence Seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return New file position on success, negative error code on failure
 */
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence);

/**
 * @brief ioctl operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param cmd ioctl command number
 * @param arg User space argument (command-specific)
 * @return 0 on success, negative error code on failure
 */
long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* Device setup and cleanup function declarations */

/**
 * @brief Setup character device structure and register with kernel
 * @param dev Pointer to the AESD device structure
 * @return 0 on success, negative error code on failure
 */
int aesd_setup_cdev(struct aesd_dev *dev);

/**
 * @brief Cleanup device resources and free allocated memory
 * @param dev Pointer to the AESD device structure
 */
void aesd_cleanup_device(struct aesd_dev *dev);

/* Buffer handling function declarations */

/**
 * @brief Handle incoming write data and buffer management
 * @param dev Pointer to the AESD device structure
 * @param new_data Pointer to the new data to be buffered
 * @param count Number of bytes in new_data
 * @return 0 on success, negative error code on failure
 */
int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count);

/**
 * @brief Process a complete command when newline is detected
 * @param dev Pointer to the AESD device structure
 *
 * This function is called when a newline character is detected in the
 * write buffer, indicating a complete command. It moves the buffered
 * data from the temporary write buffer to the circular buffer.
 */
void aesd_handle_complete_command(struct aesd_dev *dev);

#endif /* AESD_CHAR_DRIVER_H */
