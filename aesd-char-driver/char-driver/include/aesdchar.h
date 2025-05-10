/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_H
#define AESD_CHAR_DRIVER_H

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "../../circular-buffer/include/aesd-circular-buffer.h"

#define AESD_DEBUG 1 // Comment out this line to disable debug

#undef PDEBUG
#ifdef AESD_DEBUG
#ifdef __KERNEL__
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "aesdchar: " fmt, ##args)
#else
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#endif
#else
#define PDEBUG(fmt, args...)
#endif

struct aesd_dev
{
    struct aesd_circular_buffer buffer;
    struct mutex lock;
    struct cdev cdev;
    char *write_buf;
    size_t write_buf_size;
};

/* Function declarations */
extern int aesd_open(struct inode *inode, struct file *filp);
extern int aesd_release(struct inode *inode, struct file *filp);
extern ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
extern ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

#endif /* AESD_CHAR_DRIVER_H */
