#include "../include/aesd-char-common.h"
#include "../include/aesdchar.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count)
{
    char *new_buf = NULL;
    size_t new_size = 0;

    if (dev->write_buf)
    {
        new_size = dev->write_buf_size + count;
        new_buf = krealloc(dev->write_buf, new_size, GFP_KERNEL);
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
        dev->write_buf = kmalloc(count, GFP_KERNEL);
        if (!dev->write_buf)
        {
            return -ENOMEM;
        }

        memcpy(dev->write_buf, new_data, count);
        dev->write_buf_size = count;
    }
    return 0;
}

void aesd_handle_complete_command(struct aesd_dev *dev)
{
    struct aesd_buffer_entry entry;
    struct aesd_buffer_entry *oldest;
    char *cmd_buf;

    if (!dev || !dev->write_buf)
        return;

    // Save the buffer pointer before adding to circular buffer
    cmd_buf = dev->write_buf;

    entry.buffptr = cmd_buf;
    entry.size = dev->write_buf_size;

    // Clear device write buffer pointers before adding to circular buffer
    dev->write_buf = NULL;
    dev->write_buf_size = 0;

    if (dev->buffer.full)
    {
        oldest = &dev->buffer.entry[dev->buffer.out_offs];
        if (oldest->buffptr)
        {
            kfree(oldest->buffptr);
            oldest->buffptr = NULL;
        }
    }

    aesd_circular_buffer_add_entry(&dev->buffer, &entry);
}
