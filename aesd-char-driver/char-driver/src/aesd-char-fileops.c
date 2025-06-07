/**
 * @file aesd-char-fileops.c
 * @brief File operations implementation for AESD character driver
 *
 * This file implements the file operations for the AESD character driver,
 * including read, write, open, release, llseek, and ioctl operations.
 *
 * Key features implemented:
 * - Basic file operations (open, release, read, write)
 * - llseek support for SEEK_SET, SEEK_CUR, and SEEK_END
 * - ioctl support for AESDCHAR_IOCSEEKTO command
 * - Thread-safe operations using mutex locks
 *
 * @author Ekpenyong-Esu
 * @date June 7, 2025
 */

#define __KERNEL__
#include "../../aesd_ioctl.h"
#include "../include/aesdchar.h"
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

/**
 * @brief Open the AESD character device
 * @param inode The inode structure
 * @param filp The file structure
 * @return 0 on success
 */
int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

/**
 * @brief Release the AESD character device
 * @param inode The inode structure
 * @param filp The file structure
 * @return 0 on success
 */
int aesd_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/**
 * @brief Read operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param buf User space buffer to read data into
 * @param count Number of bytes requested to read
 * @param f_pos Pointer to current file position
 * @return Number of bytes actually read on success, negative error code on failure
 *
 * This function reads data from the circular buffer based on the current file position.
 * It uses the circular buffer's find_entry_offset_for_fpos function to locate the
 * appropriate buffer entry and offset, then copies the requested data to user space.
 * The function is thread-safe and uses mutex locking.
 *
 * Return values:
 * - Positive: Number of bytes successfully read
 * - 0: End of file (no more data available)
 * - -EINVAL: Invalid parameters
 * - -ERESTARTSYS: Interrupted by signal while waiting for mutex
 * - -EFAULT: Failed to copy data to user space
 */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    size_t entry_offset = 0;
    size_t bytes_to_read;

    if (!dev || !buf || !f_pos)
    {
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset);
    if (!entry)
    {
        mutex_unlock(&dev->lock);
        return 0; // EOF - no more data
    }

    bytes_to_read = min(count, entry->size - entry_offset);
    if (copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_read))
    {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += bytes_to_read;
    retval = bytes_to_read;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

/**
 * @brief Helper function to calculate total buffer size
 * @param buffer Pointer to the circular buffer
 * @return Total size of all valid entries in the buffer
 *
 * This function iterates through all valid entries in the circular buffer
 * and calculates the cumulative size. Used by llseek for SEEK_END operations
 * and for bounds checking.
 */
static size_t aesd_get_total_buffer_size(struct aesd_circular_buffer *buffer)
{
    size_t total_size = 0;
    uint8_t current_idx = buffer->out_offs;
    uint8_t entries_checked = 0;
    uint8_t total_entries;

    // Calculate total valid entries in the circular buffer
    if (buffer->full)
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if (buffer->in_offs >= buffer->out_offs)
    {
        total_entries = buffer->in_offs - buffer->out_offs;
    }
    else
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }

    // Sum up sizes of all valid entries
    while (entries_checked < total_entries)
    {
        total_size += buffer->entry[current_idx].size;
        current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entries_checked++;
    }

    return total_size;
}

/**
 * @brief Implement llseek file operation for AESD character driver
 * @param filp Pointer to the file structure
 * @param offset Offset value for seeking
 * @param whence Seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return New file position on success, negative error code on failure
 *
 * This function implements seeking within the circular buffer data.
 * It supports all standard seek modes:
 * - SEEK_SET: Absolute position from beginning
 * - SEEK_CUR: Relative position from current location
 * - SEEK_END: Position relative to end of buffer
 *
 * Returns -EINVAL for out-of-bounds seeks or invalid whence values.
 */
loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t new_pos;
    size_t total_size;

    if (!dev)
    {
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    total_size = aesd_get_total_buffer_size(&dev->buffer);

    // Calculate new position based on seek mode
    switch (whence)
    {
    case SEEK_SET:
        new_pos = offset; // Absolute position from start
        break;
    case SEEK_CUR:
        new_pos = filp->f_pos + offset; // Relative to current position
        break;
    case SEEK_END:
        new_pos = total_size + offset; // Relative to end of buffer
        break;
    default:
        mutex_unlock(&dev->lock);
        return -EINVAL; // Invalid whence parameter
    }

    // Validate bounds - position must be within buffer range
    if (new_pos < 0 || new_pos > total_size)
    {
        mutex_unlock(&dev->lock);
        return -EINVAL; // Out of bounds seek
    }

    filp->f_pos = new_pos; // Update file position
    mutex_unlock(&dev->lock);
    return new_pos; // Return new position
}

/**
 * @brief Handle ioctl commands for AESD character driver
 * @param filp Pointer to the file structure
 * @param cmd The ioctl command number
 * @param arg User space argument (typically a pointer to data structure)
 * @return 0 on success, negative error code on failure
 *
 * Currently supports:
 * - AESDCHAR_IOCSEEKTO: Seek to a specific command and offset within that command
 *   Takes a struct aesd_seekto with write_cmd (command index) and
 *   write_cmd_offset (byte offset within the command)
 *
 * The function validates both the command index and offset before seeking.
 * Returns -EINVAL for invalid parameters, -EFAULT for copy_from_user errors.
 */
long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    loff_t new_pos = 0;
    uint8_t current_idx;
    uint8_t entries_checked = 0;
    uint8_t total_entries;

    if (!dev)
    {
        return -EINVAL;
    }

    switch (cmd)
    {
    case AESDCHAR_IOCSEEKTO:
        // Safely copy the seekto structure from user space
        if (copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto)))
        {
            return -EFAULT;
        }

        if (mutex_lock_interruptible(&dev->lock))
        {
            return -ERESTARTSYS;
        }

        // Calculate total valid entries in the circular buffer
        if (dev->buffer.full)
        {
            total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }
        else if (dev->buffer.in_offs >= dev->buffer.out_offs)
        {
            total_entries = dev->buffer.in_offs - dev->buffer.out_offs;
        }
        else
        {
            total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - dev->buffer.out_offs + dev->buffer.in_offs;
        }

        // Validate command index is within available entries
        if (seekto.write_cmd >= total_entries)
        {
            mutex_unlock(&dev->lock);
            return -EINVAL;
        }

        // Find the target entry and calculate position
        current_idx = dev->buffer.out_offs;
        for (entries_checked = 0; entries_checked < seekto.write_cmd; entries_checked++)
        {
            new_pos += dev->buffer.entry[current_idx].size;
            current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }

        // Validate offset within the command
        if (seekto.write_cmd_offset >= dev->buffer.entry[current_idx].size)
        {
            mutex_unlock(&dev->lock);
            return -EINVAL;
        }

        new_pos += seekto.write_cmd_offset;
        filp->f_pos = new_pos;

        mutex_unlock(&dev->lock);
        return 0;

    default:
        return -ENOTTY;
    }
}

/**
 * @brief Write operation for the AESD character device
 * @param filp Pointer to the file structure
 * @param buf User space buffer containing data to write
 * @param count Number of bytes to write
 * @param f_pos Pointer to file position (not used in this implementation)
 * @return Number of bytes written on success, negative error code on failure
 *
 * This function handles write operations to the character device. It:
 * 1. Copies data from user space to kernel space
 * 2. Appends data to a temporary write buffer
 * 3. Checks for newline character to detect complete commands
 * 4. When a complete command is detected, moves it to the circular buffer
 *
 * The function is thread-safe and uses mutex locking. Write data is accumulated
 * in a temporary buffer until a newline is encountered, at which point the
 * complete command is added to the circular buffer for later reading.
 *
 * Return values:
 * - Positive: Number of bytes successfully written (always equals input count)
 * - -EINVAL: Invalid parameters (NULL pointers or zero count)
 * - -ERESTARTSYS: Interrupted by signal while waiting for mutex
 * - -ENOMEM: Memory allocation failure
 * - -EFAULT: Failed to copy data from user space
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *tmp_buf;
    const char *newline;
    ssize_t retval = -ENOMEM;
    int result;

    /* Input validation */
    if (!dev || !buf || !count)
    {
        return -EINVAL;
    }

    /* Acquire mutex for thread-safe operation */
    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    /* Allocate temporary buffer for copying from user space */
    tmp_buf = kmalloc(count, GFP_KERNEL);
    if (!tmp_buf)
    {
        mutex_unlock(&dev->lock);
        return -ENOMEM;
    }

    /* Copy data from user space to kernel space */
    if (copy_from_user(tmp_buf, buf, count))
    {
        retval = -EFAULT;
        goto out_free;
    }

    /* Add new data to the device's write buffer */
    result = aesd_handle_write_buffer(dev, tmp_buf, count);
    if (result < 0)
    {
        retval = result;
        goto out_free;
    }

    /* Check if we have a complete command (terminated by newline) */
    newline = memchr(dev->write_buf, '\n', dev->write_buf_size);
    if (newline)
    {
        /* Complete command detected - move to circular buffer */
        aesd_handle_complete_command(dev);
    }

    /* Return number of bytes successfully processed */
    retval = count;

out_free:
    /* Cleanup and release resources */
    kfree(tmp_buf);
    mutex_unlock(&dev->lock);
    return retval;
}

/**
 * @brief File operations structure for AESD character device
 *
 * This structure defines all the file operations supported by the AESD
 * character device. Each field points to a function that handles the
 * corresponding system call:
 *
 * - owner: Module that owns this structure (prevents unloading while in use)
 * - read: Handles read() system calls - reads data from circular buffer
 * - write: Handles write() system calls - accumulates data until newline
 * - open: Handles open() system calls - initializes file context
 * - release: Handles close() system calls - cleanup operations
 * - llseek: Handles lseek() system calls - positioning within buffer data
 * - unlocked_ioctl: Handles ioctl() system calls - advanced seek operations
 *
 * The combination of these operations provides a complete character device
 * interface that applications can use with standard POSIX file operations.
 */
struct file_operations aesd_fops = {
    .owner = THIS_MODULE,                  /* Prevent module unload while device is open */
    .read = aesd_read,                     /* Read data from circular buffer */
    .write = aesd_write,                   /* Write data to device (accumulate until newline) */
    .open = aesd_open,                     /* Open device - setup file context */
    .release = aesd_release,               /* Close device - cleanup operations */
    .llseek = aesd_llseek,                 /* Seek within buffer data */
    .unlocked_ioctl = aesd_unlocked_ioctl, /* Advanced ioctl operations */
};
