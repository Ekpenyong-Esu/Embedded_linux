#define __KERNEL__
/**
 * @file main.c
 * @brief Main entry point for the AESD char driver
 */

#include "char-driver/include/aesdchar.h"
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("AESD Char Driver");

int aesd_major = 0; // Dynamic major by default
int aesd_minor = 0;
struct aesd_dev aesd_device;

static int __init aesd_init(void)
{
    dev_t dev = 0;
    int result;

    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    if (result < 0)
    {
        pr_err("Could not allocate major number %d\n", aesd_major);
        return result;
    }
    aesd_major = MAJOR(dev);

    memset(&aesd_device, 0, sizeof(struct aesd_dev));
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);

    result = aesd_setup_cdev(&aesd_device);
    if (result)
    {
        unregister_chrdev_region(dev, 1);
        mutex_destroy(&aesd_device.lock);
        return result;
    }

    return 0;
}

static void __exit aesd_exit(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    aesd_cleanup_device(&aesd_device);
    cdev_del(&aesd_device.cdev);
    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
}

module_init(aesd_init);
module_exit(aesd_exit);
