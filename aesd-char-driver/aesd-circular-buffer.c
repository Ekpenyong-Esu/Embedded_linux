/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to the AESD circular buffer implementation
 *
 * @author Your Name
 * @date Insert Date
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
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
    /**
     * Implement this function
     */
    if (buffer == NULL || add_entry == NULL)
    {
        return;
    }

    // Add the entry at the current in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;

    // If the buffer is full, we need to advance the out_offs pointer as well
    if (buffer->full)
    {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Advance the in_offs pointer to the next position
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // Check if the buffer is now full (in_offs has caught up with out_offs)
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

/**
 * Removes the oldest entry in the circular buffer
 * Any necessary locking must be handled by the caller
 * @param buffer the buffer to remove from
 */
void aesd_circular_buffer_remove_entry(struct aesd_circular_buffer *buffer)
{
    /**
     * Implement this function
     */
    if (buffer == NULL)
    {
        return;
    }

    // If buffer is empty, nothing to remove
    if ((buffer->in_offs == buffer->out_offs) && !buffer->full)
    {
        return;
    }

    // Clear the entry - not strictly needed but good practice
    buffer->entry[buffer->out_offs].buffptr = NULL;
    buffer->entry[buffer->out_offs].size = 0;

    // Advance the out_offs pointer
    buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // The buffer is no longer full
    buffer->full = false;
}

/**
 * Initializes the circular buffer described by @param buffer to an empty struct
 * @param buffer the buffer to initialize
 */
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer, 0, sizeof(struct aesd_circular_buffer));
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
    /**
     * Implement this function
     */
    uint8_t index;
    size_t position = 0;
    uint8_t entries_traversed = 0;

    if (buffer == NULL || entry_offset_byte_rtn == NULL)
    {
        return NULL;
    }

    // Start from out_offs and go through all valid entries
    index = buffer->out_offs;

    // Calculate how many entries to examine
    uint8_t count = buffer->full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : buffer->in_offs - buffer->out_offs;
    if (buffer->in_offs < buffer->out_offs && !buffer->full)
    {
        count = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - buffer->out_offs + buffer->in_offs;
    }

    // Go through each valid entry
    while (entries_traversed < count)
    {
        // If the char_offset falls within this entry
        if (char_offset < position + buffer->entry[index].size)
        {
            *entry_offset_byte_rtn = char_offset - position;
            return &buffer->entry[index];
        }

        // Advance position by the size of this entry
        position += buffer->entry[index].size;

        // Move to next entry
        index = (index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entries_traversed++;
    }

    // Not found
    return NULL;
}
