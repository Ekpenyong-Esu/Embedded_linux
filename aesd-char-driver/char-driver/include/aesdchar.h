/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_H
#define AESD_CHAR_DRIVER_H

#ifdef __KERNEL__
#include "../../circular-buffer/include/aesd-circular-buffer.h"
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

#define PDEBUG(fmt, ...) pr_debug("aesdchar: " fmt, ##__VA_ARGS__)
#else
#define PDEBUG(fmt, ...)
#endif

struct aesd_dev
{
    struct aesd_circular_buffer buffer; /* Circular buffer for commands */
    struct mutex lock;                  /* Mutex for thread safety */
    struct cdev cdev;                   /* Character device structure */
    char *write_buf;                    /* Buffer for partial commands */
    size_t write_buf_size;              /* Size of data in write_buf */
};

/* External declarations */
extern int aesd_major;
extern int aesd_minor;
extern struct file_operations aesd_fops;

/* File operation declarations */
int aesd_open(struct inode *inode, struct file *filp);
int aesd_release(struct inode *inode, struct file *filp);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

/* Device setup and cleanup */
int aesd_setup_cdev(struct aesd_dev *dev);
void aesd_cleanup_device(struct aesd_dev *dev);

/* Buffer handling functions */
int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count);
void aesd_handle_complete_command(struct aesd_dev *dev);

#endif /* AESD_CHAR_DRIVER_H */
