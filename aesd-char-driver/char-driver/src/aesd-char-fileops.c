#define __KERNEL__
#include "../include/aesdchar.h"
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    return 0;
}

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

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *tmp_buf;
    const char *newline;
    ssize_t retval = -ENOMEM;
    int result;

    if (!dev || !buf || !count)
    {
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    tmp_buf = kmalloc(count, GFP_KERNEL);
    if (!tmp_buf)
    {
        mutex_unlock(&dev->lock);
        return -ENOMEM;
    }

    if (copy_from_user(tmp_buf, buf, count))
    {
        retval = -EFAULT;
        goto out_free;
    }

    result = aesd_handle_write_buffer(dev, tmp_buf, count);
    if (result < 0)
    {
        retval = result;
        goto out_free;
    }

    newline = memchr(dev->write_buf, '\n', dev->write_buf_size);
    if (newline)
    {
        aesd_handle_complete_command(dev);
    }

    retval = count;

out_free:
    kfree(tmp_buf);
    mutex_unlock(&dev->lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
};
