/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to the AESD circular buffer implementation
 *
 * @author Your Name
 * @date Insert Date
 */

#ifdef __KERNEL__
#include <linux/printk.h>
#include <linux/string.h>
#define DEBUG_LOG(fmt, ...) printk(KERN_DEBUG "aesd_circular: " fmt, ##__VA_ARGS__)
#else
#include <string.h>
#define DEBUG_LOG(fmt, ...) /* No logging in user space */
#endif

#include "aesd-circular-buffer.h"

/**
 * Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
 * If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
 * new start location.
 * Any necessary locking must be handled by the caller
 * @param buffer the buffer to add to
 * @param add_entry a pointer to the entry to add
 */
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    if (!buffer || !add_entry || !add_entry->buffptr)
    {
        DEBUG_LOG("Invalid parameters in add_entry\n");
        return;
    }

    DEBUG_LOG("Adding entry of size %zu at position %d\n", add_entry->size, buffer->in_offs);

    // Add the entry at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;

    // If buffer is full, advance out_offs
    if (buffer->full)
    {
        DEBUG_LOG("Buffer full, advancing out_offs from %d\n", buffer->out_offs);
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Advance in_offs and update full flag
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    buffer->full = (buffer->in_offs == buffer->out_offs);

    DEBUG_LOG("Buffer state after add: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}

/**
 * Removes the oldest entry in the circular buffer
 * Any necessary locking must be handled by the caller
 * @param buffer the buffer to remove from
 */
void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer)
{
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in remove_entry\n");
        return;
    }

    // Check if buffer is empty
    if (!buffer->full && (buffer->in_offs == buffer->out_offs))
    {
        DEBUG_LOG("Attempted to remove from empty buffer\n");
        return;
    }

    DEBUG_LOG("Removing entry at position %d\n", buffer->out_offs);

    // Clear the entry
    buffer->entry[buffer->out_offs].buffptr = NULL;
    buffer->entry[buffer->out_offs].size = 0;

    // Advance out_offs and update full flag
    buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    buffer->full = false;

    DEBUG_LOG("Buffer state after remove: in=%d, out=%d, full=%d\n", buffer->in_offs, buffer->out_offs, buffer->full);
}

/**
 * Initializes the circular buffer described by @param buffer to an empty struct
 * @param buffer the buffer to initialize
 */
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    if (!buffer)
    {
        DEBUG_LOG("Invalid buffer parameter in init\n");
        return;
    }

    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
    DEBUG_LOG("Buffer initialized\n");
}

/**
 * Finds the entry corresponding to the char offset @param char_offset in @param buffer
 * Any necessary locking must be handled by the caller
 * @param buffer the buffer to search
 * @param char_offset the position to find in the buffer
 * @param entry_offset_byte_rtn set to the byte of the returned entry corresponding to char_offset
 * @return the buffer entry associated with char_offset, or NULL if no entry found
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
                                                                          size_t char_offset,
                                                                          size_t *entry_offset_byte_rtn)
{
    if (!buffer || !entry_offset_byte_rtn)
    {
        DEBUG_LOG("Invalid parameters in find_entry\n");
        return NULL;
    }

    size_t current_pos = 0;
    uint8_t current_idx = buffer->out_offs;
    uint8_t entries_checked = 0;
    uint8_t total_entries;

    // Calculate total valid entries
    if (buffer->full)
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else if (buffer->in_offs >= buffer->out_offs)
    {
        total_entries = buffer->in_offs - buffer->out_offs;
    }
    else
    {
        total_entries = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }

    DEBUG_LOG("Searching for offset %zu in %d entries\n", char_offset, total_entries);

    // Search through valid entries
    while (entries_checked < total_entries)
    {
        size_t entry_size = buffer->entry[current_idx].size;

        // Check if offset falls within this entry
        if (char_offset < current_pos + entry_size)
        {
            *entry_offset_byte_rtn = char_offset - current_pos;
            DEBUG_LOG("Found offset in entry %d at relative offset %zu\n", current_idx, *entry_offset_byte_rtn);
            return &buffer->entry[current_idx];
        }

        current_pos += entry_size;
        current_idx = (current_idx + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entries_checked++;
    }

    DEBUG_LOG("Offset %zu not found in buffer\n", char_offset);
    return NULL;
}
