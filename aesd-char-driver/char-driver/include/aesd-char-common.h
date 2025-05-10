#ifndef AESD_CHAR_COMMON_H
#define AESD_CHAR_COMMON_H

#ifdef __KERNEL__
#include <generated/autoconf.h>
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

#include "aesdchar.h"

int aesd_setup_cdev(struct aesd_dev *dev);
void aesd_cleanup_device(struct aesd_dev *dev);
int aesd_handle_write_buffer(struct aesd_dev *dev, const char *new_data, size_t count);
void aesd_handle_complete_command(struct aesd_dev *dev);

#endif /* AESD_CHAR_COMMON_H */
