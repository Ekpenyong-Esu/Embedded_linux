#include "../include/aesd-char-common.h"
#include "../include/aesdchar.h"

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    size_t entry_offset = 0;
    size_t bytes_to_read;
    size_t total_bytes = 0;
    size_t temp_pos = *f_pos;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    // Keep reading entries until we've satisfied the count or run out of data
    while (total_bytes < count)
    {
        entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, temp_pos, &entry_offset);
        if (!entry)
            break; // No more data to read

        // Calculate how many bytes we can read from this entry
        bytes_to_read = min(count - total_bytes, entry->size - entry_offset);

        if (copy_to_user(buf + total_bytes, entry->buffptr + entry_offset, bytes_to_read))
        {
            retval = -EFAULT;
            goto out;
        }

        total_bytes += bytes_to_read;
        temp_pos += bytes_to_read;

        // If we've read the full entry, move to next one
        if (entry_offset + bytes_to_read >= entry->size)
            temp_pos = total_bytes;
    }

    *f_pos += total_bytes;
    retval = total_bytes;

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

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    tmp_buf = kmalloc(count, GFP_KERNEL);
    if (!tmp_buf)
    {
        mutex_unlock(&dev->lock);
        return -ENOMEM;
    }

    if (copy_from_user(tmp_buf, buf, count))
    {
        retval = -EFAULT;
        goto out;
    }

    result = aesd_handle_write_buffer(dev, tmp_buf, count);
    if (result < 0)
    {
        retval = result;
        goto out;
    }

    newline = memchr(dev->write_buf, '\n', dev->write_buf_size);
    if (newline)
        aesd_handle_complete_command(dev);

    retval = count;

out:
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
