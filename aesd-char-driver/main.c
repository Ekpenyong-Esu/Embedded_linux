#define __KERNEL__
/**
 * @file main.c
 * @brief Main entry point and module initialization for the AESD character driver
 *
 * This file contains the module initialization and cleanup functions for the
 * AESD character driver. It handles:
 * - Dynamic major number allocation
 * - Device structure initialization
 * - Character device registration
 * - Module loading and unloading
 * - Resource cleanup on exit
 *
 * @author Dan Walkes (original), Enhanced by Assignment Team
 * @date Created: Oct 23, 2019, Enhanced: June 7, 2025
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

/* Module metadata */
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Ekpenyong-Esu");
MODULE_DESCRIPTION("AESD Character Driver with llseek and ioctl support");
MODULE_VERSION("1.0");

/* Global variables */
/** @brief Major device number (dynamically allocated) */
int aesd_major = 0;

/** @brief Minor device number (always 0 for single device) */
int aesd_minor = 0;

/** @brief Global device structure instance */
struct aesd_dev aesd_device;

/**
 * @brief Module initialization function
 * @return 0 on success, negative error code on failure
 *
 * This function is called when the module is loaded into the kernel.
 * It performs the following operations:
 * 1. Allocates a dynamic major device number
 * 2. Initializes the device structure and mutex
 * 3. Initializes the circular buffer
 * 4. Sets up the character device and registers it with the kernel
 *
 * If any step fails, it cleans up previously allocated resources.
 */
static int __init aesd_init(void)
{
    dev_t dev = 0;
    int result;

    /* Step 1: Allocate character device region with dynamic major number */
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    if (result < 0)
    {
        pr_err("Could not allocate major number %d\n", aesd_major);
        return result;
    }
    aesd_major = MAJOR(dev);

    /* Step 2: Initialize device structure and synchronization primitives */
    memset(&aesd_device, 0, sizeof(struct aesd_dev));
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);

    /* Step 3: Setup character device and add to kernel */
    result = aesd_setup_cdev(&aesd_device);
    if (result)
    {
        /* Cleanup on failure */
        unregister_chrdev_region(dev, 1);
        mutex_destroy(&aesd_device.lock);
        return result;
    }

    pr_info("AESD character driver loaded successfully with major number %d\n", aesd_major);
    return 0;
}

/**
 * @brief Module cleanup function
 *
 * This function is called when the module is unloaded from the kernel.
 * It performs cleanup in reverse order of initialization:
 * 1. Cleanup device resources and free allocated memory
 * 2. Remove character device from kernel
 * 3. Destroy mutex
 * 4. Unregister device number region
 *
 * This ensures all resources are properly released when the module is removed.
 */
static void __exit aesd_exit(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    pr_info("AESD character driver unloading...\n");

    /* Step 1: Clean up device resources and free allocated memory */
    aesd_cleanup_device(&aesd_device);

    /* Step 2: Remove character device from kernel */
    cdev_del(&aesd_device.cdev);

    /* Step 3: Destroy synchronization primitives */
    mutex_destroy(&aesd_device.lock);

    /* Step 4: Unregister the device number region */
    unregister_chrdev_region(devno, 1);

    pr_info("AESD character driver unloaded successfully\n");
}

module_init(aesd_init);
module_exit(aesd_exit);
