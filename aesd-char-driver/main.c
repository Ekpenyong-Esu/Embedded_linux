/**
 * @file main.c
 * @brief Main entry point for the AESD char driver
 */

#include <aesd-char-common.h>
#include <aesdchar.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("AESD Char Driver");

int aesd_major = 0; // Dynamic major by default
int aesd_minor = 0;

struct aesd_dev aesd_device;

// External declarations for functions implemented in other files
extern struct file_operations aesd_fops;
extern int aesd_setup_cdev(struct aesd_dev *dev);
extern void aesd_cleanup_device(struct aesd_dev *dev);

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
    aesd_circular_buffer_init(&aesd_device.buffer);
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
