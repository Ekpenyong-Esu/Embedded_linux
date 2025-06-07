/**
 * @file aesd-char-buffer.c
 * @brief Buffer management functions for AESD character driver
 *
 * This file implements the buffer management functionality for the AESD
 * character driver, including:
 * - Temporary write buffer handling for partial commands
 * - Complete command processing and circular buffer integration
 * - Memory management for dynamic buffer allocation
 *
 * @author Assignment Team
 * @date June 7, 2025
 */

#define __KERNEL__
#include "../include/aesdchar.h"
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

/**
 * @brief Handle incoming write data and manage temporary buffer
 * @param dev Pointer to the AESD device structure
 * @param new_data Pointer to new data to be buffered
 * @param count Number of bytes in new_data
 * @return 0 on success, negative error code on failure
 *
 * This function manages the temporary write buffer that accumulates data
 * until a complete command (terminated by newline) is received. It:
 * 1. Appends new data to existing buffer or creates new buffer
 * 2. Handles memory reallocation as needed
 * 3. Ensures null termination for string operations
 *
 * The temporary buffer allows the driver to handle partial writes and
 * reconstruct complete commands before adding them to the circular buffer.
 */
int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count)
{
    char *new_buf = NULL;
    size_t new_size = 0;

    /* Input validation */
    if (!dev || !new_data)
    {
        return -EINVAL;
    }

    /* Append to existing buffer or create new buffer */
    if (dev->write_buf)
    {
        /* Existing buffer - reallocate to accommodate new data */
        new_size = dev->write_buf_size + count;
        new_buf = krealloc(dev->write_buf, new_size + 1, GFP_KERNEL);
        if (!new_buf)
        {
            return -ENOMEM;
        }

        /* Append new data to existing buffer */
        memcpy(new_buf + dev->write_buf_size, new_data, count);
        dev->write_buf = new_buf;
        dev->write_buf_size = new_size;
    }
    else
    {
        /* No existing buffer - create new one */
        dev->write_buf = kmalloc(count + 1, GFP_KERNEL);
        if (!dev->write_buf)
        {
            return -ENOMEM;
        }

        /* Copy data to new buffer */
        memcpy(dev->write_buf, new_data, count);
        dev->write_buf_size = count;
    }

    /* Ensure null termination for safe string operations */
    dev->write_buf[dev->write_buf_size] = '\0';
    return 0;
}

/**
 * @brief Process a complete command and add it to the circular buffer
 * @param dev Pointer to the AESD device structure
 *
 * This function is called when a complete command (terminated by newline)
 * has been accumulated in the write buffer. It:
 * 1. Creates a buffer entry from the accumulated data
 * 2. Handles buffer overflow by freeing the oldest entry if necessary
 * 3. Adds the new entry to the circular buffer
 * 4. Resets the write buffer for the next command
 *
 * The function ensures that when the circular buffer is full, the oldest
 * entry is properly freed before being overwritten by the new entry.
 */
void aesd_handle_complete_command(struct aesd_dev *dev)
{
    struct aesd_buffer_entry entry = {0};
    struct aesd_buffer_entry *oldest = NULL;

    if (!dev || !dev->write_buf)
    {
        return;
    }

    /* Copy the completed command */
    entry.buffptr = dev->write_buf;
    entry.size = dev->write_buf_size;

    /* If buffer is full, free the oldest entry */
    if (dev->buffer.full)
    {
        oldest = &dev->buffer.entry[dev->buffer.out_offs];
        if (oldest->buffptr)
        {
            kfree((void *)oldest->buffptr);
            oldest->buffptr = NULL;
            oldest->size = 0;
        }
    }

    /* Reset write buffer pointers */
    dev->write_buf = NULL;
    dev->write_buf_size = 0;

    /* Add new entry to circular buffer */
    aesd_circular_buffer_add_entry(&dev->buffer, &entry);
}
