/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 */

#include "aesdchar.h"
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

/* Function prototypes */
static int aesd_setup_cdev(struct aesd_dev *dev);
static void aesd_cleanup_device(struct aesd_dev *dev);
static int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count);
static void aesd_handle_complete_command(struct aesd_dev *dev);

/* File operations prototypes */
static int aesd_open(struct inode *inode, struct file *filp);
static int aesd_release(struct inode *inode, struct file *filp);
static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

/* Module init/exit prototypes */
static int __init aesd_init_module(void);
static void __exit aesd_cleanup_module(void);

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("AESD Char Driver");

struct aesd_dev aesd_device;

static int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count)
{
    char *new_buf;
    size_t new_size;

    if (dev->write_buf)
    {
        new_size = dev->write_buf_size + count;
        new_buf = krealloc(dev->write_buf, new_size, GFP_KERNEL);
        if (!new_buf)
            return -ENOMEM;

        memcpy(new_buf + dev->write_buf_size, new_data, count);
        dev->write_buf = new_buf;
        dev->write_buf_size = new_size;
    }
    else
    {
        dev->write_buf = kmalloc(count, GFP_KERNEL);
        if (!dev->write_buf)
            return -ENOMEM;

        memcpy(dev->write_buf, new_data, count);
        dev->write_buf_size = count;
    }
    return 0;
}

static void aesd_handle_complete_command(struct aesd_dev *dev)
{
    struct aesd_buffer_entry entry;
    struct aesd_buffer_entry *oldest;

    entry.buffptr = dev->write_buf;
    entry.size = dev->write_buf_size;

    // Free oldest entry if buffer is full
    if (dev->buffer.full)
    {
        oldest = &dev->buffer.entry[dev->buffer.out_offs];
        if (oldest->buffptr)
            kfree(oldest->buffptr);
    }

    aesd_circular_buffer_add_entry(&dev->buffer, &entry);
    dev->write_buf = NULL;
    dev->write_buf_size = 0;
}

static int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

static int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    return 0;
}

static ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset = 0;
    struct aesd_buffer_entry *entry;

    PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset);
    if (!entry)
    {
        mutex_unlock(&dev->lock);
        return 0;
    }

    if (entry_offset >= entry->size)
    {
        mutex_unlock(&dev->lock);
        return 0;
    }

    // Determine how many bytes to copy
    count = min(count, entry->size - entry_offset);

    if (copy_to_user(buf, entry->buffptr + entry_offset, count))
    {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

static ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *tmp_buf;
    const char *newline;
    ssize_t retval = -ENOMEM;
    int result;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    // Allocate temporary buffer for user data
    tmp_buf = kmalloc(count, GFP_KERNEL);
    if (!tmp_buf)
        goto out_nomem;

    if (copy_from_user(tmp_buf, buf, count))
    {
        retval = -EFAULT;
        goto out_free;
    }

    // Handle the write buffer
    result = aesd_handle_write_buffer(dev, tmp_buf, count);
    if (result < 0)
    {
        retval = result;
        goto out_free;
    }

    // Check for newline in the complete buffer
    newline = memchr(dev->write_buf, '\n', dev->write_buf_size);
    if (newline)
        aesd_handle_complete_command(dev);

    retval = count;
    kfree(tmp_buf);
    mutex_unlock(&dev->lock);
    return retval;

out_free:
    kfree(tmp_buf);
out_nomem:
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

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

static void aesd_cleanup_device(struct aesd_dev *dev)
{
    uint8_t index;
    struct aesd_buffer_entry *entry;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index)
    {
        if (entry->buffptr)
        {
            kfree(entry->buffptr);
            entry->buffptr = NULL;
        }
    }

    if (dev->write_buf)
    {
        kfree(dev->write_buf);
        dev->write_buf = NULL;
    }
}

static int __init aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device, 0, sizeof(struct aesd_dev));

    // Initialize the circular buffer
    aesd_circular_buffer_init(&aesd_device.buffer);

    // Initialize the mutex
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);
    if (result)
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;
}

static void __exit aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    aesd_cleanup_device(&aesd_device);
    cdev_del(&aesd_device.cdev);
    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
