#define __KERNEL__
#include "../include/aesdchar.h"
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err = 0;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    if (!dev)
    {
        return -EINVAL;
    }

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        pr_err("Error %d adding aesd cdev", err);
    }
    return err;
}

void aesd_cleanup_device(struct aesd_dev *dev)
{
    uint8_t index;
    struct aesd_buffer_entry *entry = NULL;

    if (!dev)
    {
        return;
    }

    /* Free any pending write buffer */
    if (dev->write_buf)
    {
        kfree(dev->write_buf);
        dev->write_buf = NULL;
        dev->write_buf_size = 0;
    }

    /* Free all entries in the circular buffer */
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index)
    {
        if (entry && entry->buffptr)
        {
            kfree((void *)entry->buffptr);
            entry->buffptr = NULL;
            entry->size = 0;
        }
    }
}
