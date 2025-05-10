#define __KERNEL__
#include "../include/aesdchar.h"
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count)
{
    char *new_buf = NULL;
    size_t new_size = 0;

    if (!dev || !new_data)
    {
        return -EINVAL;
    }

    /* Append to existing buffer or create new one */
    if (dev->write_buf)
    {
        new_size = dev->write_buf_size + count;
        new_buf = krealloc(dev->write_buf, new_size + 1, GFP_KERNEL);
        if (!new_buf)
        {
            return -ENOMEM;
        }

        memcpy(new_buf + dev->write_buf_size, new_data, count);
        dev->write_buf = new_buf;
        dev->write_buf_size = new_size;
    }
    else
    {
        dev->write_buf = kmalloc(count + 1, GFP_KERNEL);
        if (!dev->write_buf)
        {
            return -ENOMEM;
        }

        memcpy(dev->write_buf, new_data, count);
        dev->write_buf_size = count;
    }

    /* Ensure null termination */
    dev->write_buf[dev->write_buf_size] = '\0';
    return 0;
}

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
