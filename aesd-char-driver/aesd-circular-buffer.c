#ifdef __KERNEL__
#include <linux/printk.h>
#include <linux/string.h>
#define DEBUG_LOG(fmt, ...) printk(KERN_DEBUG "aesd_circular: " fmt, ##__VA_ARGS__)
#else
#include <string.h>
#define DEBUG_LOG(fmt, ...) /* No logging in user space */
#endif

#include "circular-buffer/include/aesd-circular-buffer-common.h"
#include "circular-buffer/include/aesd-circular-buffer.h"

// Functions are implemented in circular-buffer/src/
extern void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer,
                                           const struct aesd_buffer_entry *add_entry);
extern void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer);
extern void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer);
extern struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                                 size_t char_offset,
                                                                                 size_t *entry_offset_byte_rtn);
