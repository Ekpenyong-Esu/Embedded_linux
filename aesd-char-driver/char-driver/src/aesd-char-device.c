/**
 * @file aesd-char-device.c
 * @brief Device setup and cleanup functions for AESD character driver
 *
 * This file implements the device initialization and cleanup functions
 * for the AESD character driver, including:
 * - Character device structure setup and kernel registration
 * - Resource cleanup and memory management
 * - Integration with the kernel's character device framework
 *
 * @author Assignment Team
 * @date June 7, 2025
 */

#define __KERNEL__
#include "../include/aesdchar.h"
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

/**
 * @brief Setup and register the character device with the kernel
 * @param dev Pointer to the AESD device structure
 * @return 0 on success, negative error code on failure
 *
 * This function initializes the character device structure and registers
 * it with the kernel's character device framework. It:
 * 1. Creates a device number from major and minor numbers
 * 2. Initializes the cdev structure with file operations
 * 3. Sets the module owner to prevent premature unloading
 * 4. Adds the device to the kernel's character device registry
 *
 * Once successful, the device will be accessible through the filesystem
 * and applications can open it using standard file operations.
 */
int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err = 0;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    /* Input validation */
    if (!dev)
    {
        return -EINVAL;
    }

    /* Initialize character device structure with file operations */
    cdev_init(&dev->cdev, &aesd_fops);

    /* Set module owner to prevent unloading while device is in use */
    dev->cdev.owner = THIS_MODULE;

    /* Add character device to kernel registry */
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        pr_err("Error %d adding aesd cdev", err);
    }
    return err;
}

/**
 * @brief Cleanup device resources and free allocated memory
 * @param dev Pointer to the AESD device structure
 *
 * This function performs comprehensive cleanup of all device resources:
 * 1. Frees any pending write buffer data
 * 2. Iterates through the circular buffer and frees all stored entries
 * 3. Clears all buffer entry pointers and sizes
 *
 * This function should be called during module unloading to ensure
 * no memory leaks occur. It safely handles the case where some
 * pointers might already be NULL.
 */
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
