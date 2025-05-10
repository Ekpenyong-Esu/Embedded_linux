#ifndef AESD_CHAR_COMMON_H
#define AESD_CHAR_COMMON_H

#include "aesdchar.h"
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

/* External variables */
extern int aesd_major;
extern int aesd_minor;
extern struct aesd_dev aesd_device;
extern struct file_operations aesd_fops;

/* Function declarations for buffer operations */
int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count);
void aesd_handle_complete_command(struct aesd_dev *dev);

/* Function declarations for device setup */
int aesd_setup_cdev(struct aesd_dev *dev);
void aesd_cleanup_device(struct aesd_dev *dev);

/* Function declarations for file operations */
int aesd_open(struct inode *inode, struct file *filp);
int aesd_release(struct inode *inode, struct file *filp);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);

#endif /* AESD_CHAR_COMMON_H */
